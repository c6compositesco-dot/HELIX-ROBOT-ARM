#pragma once
#include "helix/core/types.hpp"

namespace helix {
enum class ExecutionState { Disarmed, Armed, Running, Faulted };
enum class Fault { None, CommunicationsLost, ClockRegression, Interlock, Feedback, Drive, MotionDeadline };
struct ArmChecks {
  bool board_qualified{false};
  bool model_valid{false};
  bool references_valid{false};
  bool required_feedback_valid{false};
  bool interlocks_ok{false};
  bool all_pass() const noexcept {
    return board_qualified && model_valid && references_valid
        && required_feedback_valid && interlocks_ok;
  }
};
// Pure admission-state model. It neither drives GPIO nor performs a physical stop.
// The eventual runtime MUST service it and connect faults to a qualified stop path.
class ExecutionGate {
 public:
  explicit ExecutionGate(TimeUs watchdog_timeout_us) noexcept : timeout_(watchdog_timeout_us) {}
  ExecutionState state() const noexcept { return state_; }
  Fault fault() const noexcept { return fault_; }
  Status arm(const ArmChecks& checks, TimeUs now) noexcept {
    if (state_ == ExecutionState::Faulted) return Status::Faulted;
    if (state_ != ExecutionState::Disarmed || !checks.all_pass() || timeout_ == 0)
      return Status::NotReady;
    last_heartbeat_ = now;
    last_observed_ = now;
    state_ = ExecutionState::Armed;
    return Status::Ok;
  }
  Status start(TimeUs now) noexcept {
    poll(now);
    if (state_ != ExecutionState::Armed) return Status::NotReady;
    state_ = ExecutionState::Running;
    return Status::Ok;
  }
  void poll(TimeUs now) noexcept {
    if (state_ != ExecutionState::Armed && state_ != ExecutionState::Running) return;
    if (now < last_observed_) { trip(Fault::ClockRegression); return; }
    last_observed_ = now;
    if (now - last_heartbeat_ >= timeout_) trip(Fault::CommunicationsLost);
  }
  Status heartbeat(TimeUs now) noexcept {
    poll(now); // A late heartbeat cannot revive an expired permission.
    if (state_ != ExecutionState::Armed && state_ != ExecutionState::Running)
      return Status::NotReady;
    last_heartbeat_ = now;
    return Status::Ok;
  }
  void trip(Fault fault) noexcept {
    if (fault == Fault::None) return;
    if (fault_ == Fault::None) fault_ = fault; // Preserve the first cause.
    state_ = ExecutionState::Faulted;
  }
  void revoke_permission() noexcept {
    if (state_ != ExecutionState::Faulted) state_ = ExecutionState::Disarmed;
  }
  Status acknowledge_fault(bool external_conditions_cleared) noexcept {
    if (state_ != ExecutionState::Faulted || !external_conditions_cleared)
      return Status::NotReady;
    fault_ = Fault::None;
    state_ = ExecutionState::Disarmed; // Explicit new arm/start required.
    return Status::Ok;
  }
 private:
  ExecutionState state_{ExecutionState::Disarmed};
  Fault fault_{Fault::None};
  TimeUs timeout_{0};
  TimeUs last_heartbeat_{0};
  TimeUs last_observed_{0};
};
} // namespace helix
