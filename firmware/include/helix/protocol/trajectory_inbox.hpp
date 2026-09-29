#pragma once
#include "helix/motion/cubic_segment.hpp"
#include "helix/runtime/execution_gate.hpp"
#include "helix/platform/clock.hpp"

namespace helix::protocol {
struct SessionKey { std::uint64_t boot{0}, session{0}; };
inline bool same_key(SessionKey a, SessionKey b) noexcept {
  return a.boot == b.boot && a.session == b.session;
}
struct TrajectoryRequest {
  std::uint16_t version{1};
  SessionKey owner{};
  std::uint64_t sequence{0}, model_revision{0};
  TimeUs expires_us{0};
  motion::CubicSegment segment{};
};
// Typed semantic equality, never memcmp(padding) or a collision-prone payload hash.
inline bool same_request(const TrajectoryRequest& a, const TrajectoryRequest& b) noexcept {
  return a.version == b.version && same_key(a.owner, b.owner) && a.sequence == b.sequence
      && a.model_revision == b.model_revision && a.expires_us == b.expires_us
      && a.segment.format == b.segment.format && a.segment.start_us == b.segment.start_us
      && a.segment.duration_us == b.segment.duration_us && a.segment.control_rad == b.segment.control_rad;
}
struct ContinuityTolerance {
  double position_rad{0}, velocity_rad_s{0}, acceleration_rad_s2{0};
  bool valid() const noexcept {
    return std::isfinite(position_rad) && position_rad >= 0
        && std::isfinite(velocity_rad_s) && velocity_rad_s >= 0
        && std::isfinite(acceleration_rad_s2) && acceleration_rad_s2 >= 0;
  }
};
struct InboxConfig {
  motion::MotionEnvelope limits{};
  motion::BoundBudget budget{};
  ContinuityTolerance continuity{};
  TimeUs max_future_us{0}, lease_us{0};
  // Required reserve AFTER preparation checks, not a stopping horizon. No target default.
  TimeUs preparation_lead_us{0};
  bool valid() const noexcept {
    if (!budget.valid() || !continuity.valid() || max_future_us == 0 || lease_us == 0
        || preparation_lead_us == 0 || preparation_lead_us >= max_future_us) return false;
    for (const auto& l : limits) if (!l.valid()) return false;
    return true;
  }
};
enum class AdmissionCode {
  Accepted, Duplicate, Conflict, OutcomeUnknown, Invalid, Unsupported,
  OldSession, ModelMismatch, NotReady, Faulted, Expired, OutsideHorizon,
  Full, Discontinuous, CurveRejected
};
enum class Delivery { None, Queued, Prepared, Cancelled, OutcomeUnknown };
struct AdmissionReply {
  AdmissionCode code{AdmissionCode::Invalid};
  Delivery delivery{Delivery::None};
  motion::BoundReport bounds{};
};

// Admission/preparation only: NO motor execution, stop generation, collision approval,
// codec or authentication. One serialized owner; NOT an ISR/concurrent queue.
// All fallible work precedes the fixed-storage commit. Acknowledgements describe
// acceptance, never physical completion. Boot ID uniqueness is an integration gate.
template<std::size_t QueueCapacity, std::size_t ReceiptCapacity>
class TrajectoryInbox {
  static_assert(QueueCapacity > 0 && ReceiptCapacity >= QueueCapacity, "Invalid inbox capacities");
 public:
  TrajectoryInbox(std::uint64_t boot_id, const InboxConfig& config) noexcept
      : boot_(boot_id), config_(config), gate_(config.lease_us) {}
  std::optional<SessionKey> new_session(std::uint64_t model, TimeUs now) noexcept {
    disarm(now); // Ownership transfer invalidates old pending work even on setup failure.
    active_ = false;
    if (!config_.valid() || boot_ == 0 || model == 0 || session_ == std::numeric_limits<std::uint64_t>::max())
      return std::nullopt;
    ++session_; model_ = model; active_ = true; arm_used_ = false;
    receipts_ = {}; receipt_head_ = 0; last_sequence_ = 0; heartbeat_sequence_ = 0;
    return SessionKey{boot_, session_}; // Does not clear a latched fault or arm.
  }
  Status arm(SessionKey key, const ArmChecks& checks, TimeUs first_start,
             const motion::KinematicState& reference, TimeUs now) noexcept {
    service(now);
    if (!current(key)) return Status::Stale;
    if (gate_.state() == ExecutionState::Faulted) return Status::Faulted;
    if (arm_used_) return Status::NotReady; // A revoked stream requires a fresh session.
    if (!preparation_time_available(first_start, now)
        || first_start - now > config_.max_future_us) return Status::Invalid;
    // This slice admits a new stream only from a caller-qualified stationary reference.
    for (std::size_t i = 0; i < kJointCount; ++i)
      if (!std::isfinite(reference.q[i]) || reference.q[i] < config_.limits[i].lower_rad
          || reference.q[i] > config_.limits[i].upper_rad || reference.v[i] != 0 || reference.a[i] != 0)
        return Status::Invalid;
    const Status status = gate_.arm(checks, now);
    if (status == Status::Ok) { arm_used_ = true; tail_ = reference; tail_time_ = first_start; have_tail_ = true; }
    return status;
  }
  void service(TimeUs now) noexcept {
    gate_.poll(now);
    // Detect a missed queued handoff even if the consumer never asks for work.
    if (permitted() && size_ != 0
        && !preparation_time_available(queue_[head_].segment.start_us, now))
      gate_.trip(Fault::MotionDeadline);
    if (gate_.state() == ExecutionState::Faulted) invalidate_pending();
  }
  Status heartbeat(SessionKey key, std::uint64_t sequence, TimeUs expires, TimeUs now) noexcept {
    service(now);
    if (!current(key) || sequence == 0 || sequence <= heartbeat_sequence_
        || expires <= now || expires - now > config_.max_future_us) return Status::Stale;
    const Status status = gate_.heartbeat(now);
    if (status == Status::Ok) heartbeat_sequence_ = sequence;
    return status;
  }
  void disarm(TimeUs now) noexcept { service(now); gate_.revoke_permission(); invalidate_pending(); }
  void trip(Fault fault) noexcept {
    gate_.trip(fault);
    if (gate_.state() == ExecutionState::Faulted) invalidate_pending();
  }
  Status acknowledge_fault(bool cleared) noexcept { return gate_.acknowledge_fault(cleared); }
  ExecutionState state() const noexcept { return gate_.state(); }
  Fault fault() const noexcept { return gate_.fault(); }
  std::size_t queued() const noexcept { return size_; }

  AdmissionReply submit(const TrajectoryRequest& request, const MonotonicClock& clock) noexcept {
    const TimeUs now = clock.now_us();
    service(now); // Permission may expire even when the request itself is invalid.
    if (request.version != 1) return {AdmissionCode::Unsupported};
    if (!current(request.owner)) return {AdmissionCode::OldSession};
    if (request.model_revision != model_) return {AdmissionCode::ModelMismatch};
    if (request.sequence == 0) return {AdmissionCode::Invalid};
    for (const auto& receipt : receipts_) if (receipt.used && receipt.request.sequence == request.sequence) {
      if (!same_request(receipt.request, request)) return {AdmissionCode::Conflict};
      // Retry returns historical disposition even after expiry/fault. Never re-enqueues.
      return {AdmissionCode::Duplicate, receipt.delivery, receipt.bounds};
    }
    if (request.sequence <= last_sequence_) return {AdmissionCode::OutcomeUnknown};
    if (gate_.state() == ExecutionState::Faulted) return {AdmissionCode::Faulted};
    if (!permitted() || !have_tail_) return {AdmissionCode::NotReady};
    const auto& s = request.segment;
    if (!motion::valid_shape(s)) return {s.format == 1 ? AdmissionCode::Invalid : AdmissionCode::Unsupported};
    if (request.expires_us <= now || !preparation_time_available(s.start_us, now))
      return {AdmissionCode::Expired};
    if (request.expires_us > s.start_us || s.start_us + s.duration_us - now > config_.max_future_us)
      return {AdmissionCode::OutsideHorizon};
    if (size_ == QueueCapacity) return {AdmissionCode::Full};
    const auto bounds = motion::validate(s, config_.limits, config_.budget);
    if (bounds.status != motion::BoundStatus::Certified) return {AdmissionCode::CurveRejected, Delivery::None, bounds};
    const auto start = motion::evaluate(s, s.start_us);
    const auto end = motion::evaluate(s, s.start_us + s.duration_us);
    if (!start || !end) return {AdmissionCode::Invalid};
    if (s.start_us != tail_time_ || !continuous(*start, tail_)) return {AdmissionCode::Discontinuous};

    // Validation is bounded but not instantaneous. Recheck real time before commit.
    const TimeUs commit_time = clock.now_us();
    service(commit_time);
    if (gate_.state() == ExecutionState::Faulted) return {AdmissionCode::Faulted};
    if (!permitted() || !have_tail_) return {AdmissionCode::NotReady};
    if (request.expires_us <= commit_time || !preparation_time_available(s.start_us, commit_time))
      return {AdmissionCode::Expired};

    // Transaction commit: no callbacks, I/O, allocation, yield or possible capacity failure.
    queue_[(head_ + size_) % QueueCapacity] = request;
    receipts_[receipt_head_] = {true, request, Delivery::Queued, bounds};
    receipt_head_ = (receipt_head_ + 1) % ReceiptCapacity;
    tail_ = *end; tail_time_ = s.start_us + s.duration_us;
    last_sequence_ = request.sequence; ++size_;
    return {AdmissionCode::Accepted, Delivery::Queued, bounds};
  }

  // Handoff to a FUTURE qualified preparation/execution layer, not timed actuation.
  // A returned request is only a preparation ticket. Consumers MUST call
  // check_preparation after expensive work and immediately before publishing it.
  std::optional<TrajectoryRequest> take_for_preparation(TimeUs now) noexcept {
    service(now);
    if (!permitted() || size_ == 0) return std::nullopt;
    const auto request = queue_[head_];
    head_ = (head_ + 1) % QueueCapacity; --size_;
    for (auto& receipt : receipts_) if (receipt.used && receipt.request.sequence == request.sequence)
      receipt.delivery = Delivery::Prepared;
    return request;
  }
  // Point-in-time preparation permission only, NEVER an execution permit. Same
  // serialized owner as all other calls; do not cache Ok or share across an ISR.
  // Retained exact request + Prepared disposition is the bounded ticket registry.
  // Receipt eviction fails closed, so consumers must finish within that window.
  Status check_preparation(const TrajectoryRequest& ticket, const MonotonicClock& clock) noexcept {
    const TimeUs now = clock.now_us();
    service(now);
    if (!current(ticket.owner)) return Status::Stale;
    if (ticket.model_revision != model_) return Status::ModelMismatch;
    if (gate_.state() == ExecutionState::Faulted) return Status::Faulted;
    if (!permitted()) return Status::NotReady;
    for (const auto& receipt : receipts_) {
      if (!receipt.used || receipt.request.sequence != ticket.sequence) continue;
      if (receipt.delivery != Delivery::Prepared || !same_request(receipt.request, ticket))
        return Status::Stale;
      const TimeUs checked_at = clock.now_us();
      service(checked_at);
      if (gate_.state() == ExecutionState::Faulted) return Status::Faulted;
      if (!permitted()) return Status::NotReady;
      if (!preparation_time_available(ticket.segment.start_us, checked_at)) {
        trip(Fault::MotionDeadline);
        return Status::Deadline;
      }
      return Status::Ok;
    }
    return Status::Stale;
  }
  bool current(SessionKey key) const noexcept {
    return active_ && key.boot == boot_ && key.session == session_;
  }
 private:
  bool preparation_time_available(TimeUs start, TimeUs now) const noexcept {
    // Subtract only after ordering: no wrap at epoch zero or UINT64_MAX.
    return start > now && start - now > config_.preparation_lead_us;
  }
  struct Receipt {
    bool used{false}; TrajectoryRequest request{};
    Delivery delivery{Delivery::None}; motion::BoundReport bounds{};
  };
  bool permitted() const noexcept {
    return gate_.state() == ExecutionState::Armed || gate_.state() == ExecutionState::Running;
  }
  bool continuous(const motion::KinematicState& a, const motion::KinematicState& b) const noexcept {
    for (std::size_t i = 0; i < kJointCount; ++i)
      if (!(std::abs(a.q[i]-b.q[i]) <= config_.continuity.position_rad)
          || !(std::abs(a.v[i]-b.v[i]) <= config_.continuity.velocity_rad_s)
          || !(std::abs(a.a[i]-b.a[i]) <= config_.continuity.acceleration_rad_s2)) return false;
    return true;
  }
  void invalidate_pending() noexcept {
    for (auto& receipt : receipts_) if (receipt.used) {
      if (receipt.delivery == Delivery::Queued) receipt.delivery = Delivery::Cancelled;
      else if (receipt.delivery == Delivery::Prepared) receipt.delivery = Delivery::OutcomeUnknown;
    }
    head_ = 0; size_ = 0; have_tail_ = false;
  }
  std::uint64_t boot_{0}, session_{0}, model_{0}, last_sequence_{0}, heartbeat_sequence_{0};
  bool active_{false}, have_tail_{false}, arm_used_{false};
  InboxConfig config_{};
  ExecutionGate gate_;
  motion::KinematicState tail_{};
  TimeUs tail_time_{0};
  std::array<TrajectoryRequest, QueueCapacity> queue_{};
  std::array<Receipt, ReceiptCapacity> receipts_{};
  std::size_t head_{0}, size_{0}, receipt_head_{0};
};
} // namespace helix::protocol
