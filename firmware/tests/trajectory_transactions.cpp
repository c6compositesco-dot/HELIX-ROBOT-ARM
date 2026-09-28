#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <new>
#include "helix/protocol/trajectory_inbox.hpp"

static bool allocation_forbidden = false;
void* operator new(std::size_t n) {
  if (allocation_forbidden) std::abort();
  void* p = std::malloc(n == 0 ? 1 : n);
  if (!p) std::abort();
  return p;
}
void* operator new[](std::size_t n) { return ::operator new(n); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }

using namespace helix;
using namespace helix::motion;
using namespace helix::protocol;
class FixedClock final : public MonotonicClock {
 public:
  explicit FixedClock(TimeUs now) : now_(now) {}
  TimeUs now_us() const noexcept override { return now_; }
 private:
  TimeUs now_;
};
class AdvancingClock final : public MonotonicClock {
 public:
  AdvancingClock(TimeUs before, TimeUs after) : before_(before), after_(after) {}
  TimeUs now_us() const noexcept override { return calls_++ == 0 ? before_ : after_; }
 private:
  TimeUs before_, after_;
  mutable unsigned calls_{0};
};
static std::size_t checks = 0;
static void check(bool passed, const char* text, int line) {
  ++checks;
  if (!passed) { std::fprintf(stderr, "FAIL line %d: %s\n", line, text); std::exit(1); }
}
#define CHECK(...) check(static_cast<bool>((__VA_ARGS__)), #__VA_ARGS__, __LINE__)
static MotionEnvelope envelope(double q = 1, double v = 100, double a = 100, double j = 100) {
  MotionEnvelope e{}; for (auto& x : e) x = {-q, q, v, a, j}; return e;
}
static CubicSegment curve(double b0, double b1, double b2, double b3) {
  CubicSegment s{}; s.start_us = 1000000; s.duration_us = 1000000;
  s.control_rad[0][0] = b0; s.control_rad[1][0] = b1;
  s.control_rad[2][0] = b2; s.control_rad[3][0] = b3;
  return s;
}
static bool certified(const CubicSegment& s, const MotionEnvelope& e) {
  return validate(s, e, {8, 12264}).status == BoundStatus::Certified;
}
static CubicSegment from_state(TimeUs start, const KinematicState& x, double jerk) {
  // Synthetic 0.25 s constant-jerk fixture. Independent power-to-Bernstein conversion.
  CubicSegment s{}; s.start_us = start; s.duration_us = 250000;
  const double t = 0.25;
  for (std::size_t i = 0; i < kJointCount; ++i) {
    const double j = i == 0 ? jerk : 0;
    s.control_rad[0][i] = x.q[i];
    s.control_rad[1][i] = x.q[i] + x.v[i]*t/3;
    s.control_rad[2][i] = x.q[i] + 2*x.v[i]*t/3 + x.a[i]*t*t/6;
    s.control_rad[3][i] = x.q[i] + x.v[i]*t + x.a[i]*t*t/2 + j*t*t*t/6;
  }
  return s;
}
static void bounds_suite() {
  const auto e = envelope(); const BoundBudget budget{8, 12264};
  CHECK(validate({}, e, budget).status == BoundStatus::Invalid);
  auto s = curve(0, 0, 0, 0); CHECK(certified(s, e));
  auto edge = curve(-1, -1, -1, -1); CHECK(certified(edge, e));
  s.format = 2; CHECK(validate(s, e, budget).status == BoundStatus::Unsupported);
  s = curve(0, 0, 0, 0); s.duration_us = 0; CHECK(!valid_shape(s));
  s.duration_us = (TimeUs{1} << 53) + 1; CHECK(!valid_shape(s));
  s.duration_us = 2; s.start_us = std::numeric_limits<TimeUs>::max() - 1; CHECK(!valid_shape(s));
  for (unsigned point = 0; point < 4; ++point) for (std::size_t axis = 0; axis < kJointCount; ++axis) {
    s = curve(0, 0, 0, 0); s.control_rad[point][axis] = std::numeric_limits<double>::quiet_NaN();
    CHECK(!certified(s, e)); s.control_rad[point][axis] = std::numeric_limits<double>::infinity();
    CHECK(!evaluate(s, s.start_us));
  }
  s = curve(0, 0, 0, 0);
  CHECK(validate(s, e, {8, 0}).status == BoundStatus::Invalid);
  CHECK(validate(s, e, {9, 100}).status == BoundStatus::Invalid);
  CHECK(validate(s, e, {8, 12265}).status == BoundStatus::Invalid);
  auto r = validate(s, e, {8, 23}); CHECK(r.status == BoundStatus::Uncertified && r.nodes == 23);
  CHECK(validate(s, e, {0, 24}).status == BoundStatus::Certified);
  auto invalid = e; invalid[5].acceleration_rad_s2 = 0; CHECK(!certified(s, invalid));
  invalid = e; invalid[5].upper_rad = invalid[5].lower_rad; CHECK(!certified(s, invalid));
  invalid = e; invalid[5].jerk_rad_s3 = std::numeric_limits<double>::infinity(); CHECK(!certified(s, invalid));

  s = curve(0, 2, 2, 0); // Legal endpoint positions, illegal interior peak at t=0.5.
  r = validate(s, e, budget); CHECK(r.status == BoundStatus::Violation && r.derivative == 0);
  s = curve(0, 1.2, 1.2, 0); // Inner vertices outside, actual peak only 0.9.
  CHECK(validate(s, e, {0, 100}).status == BoundStatus::Uncertified);
  CHECK(certified(s, e)); // Subdivision must not treat inner vertices as actual positions.
  s = curve(0, 0, 1, 1); // Endpoint velocities zero; interior velocity is 1.5.
  r = validate(s, envelope(2, 1, 100, 100), budget);
  CHECK(r.status == BoundStatus::Violation && r.derivative == 1);
  s = curve(0, 0, 0, 1);
  r = validate(s, envelope(2, 10, 3, 100), budget);
  CHECK(r.status == BoundStatus::Violation && r.derivative == 2);
  r = validate(s, envelope(2, 10, 10, 5), budget);
  CHECK(r.status == BoundStatus::Violation && r.derivative == 3);
  const double huge = std::numeric_limits<double>::max();
  CHECK(validate(curve(-huge, huge, -huge, huge), envelope(huge, huge, huge, huge), budget).status == BoundStatus::Invalid);

  s = curve(0, 0, 0, 1);
  const auto begin = evaluate(s, s.start_us); const auto end = evaluate(s, s.start_us+s.duration_us);
  const auto middle = evaluate(s, s.start_us+s.duration_us/2);
  CHECK(begin && end && middle);
  CHECK(begin->q[0] == 0 && end->q[0] == 1);
  CHECK(std::abs(middle->q[0] - 0.125) < 1e-14);
  CHECK(std::abs(middle->v[0] - 0.75) < 1e-14);
  CHECK(std::abs(middle->a[0] - 3.0) < 1e-14);
  CHECK(!evaluate(s, s.start_us-1)); CHECK(!evaluate(s, s.start_us+s.duration_us+1));
  s.start_us = std::numeric_limits<TimeUs>::max() - s.duration_us;
  CHECK(evaluate(s, std::numeric_limits<TimeUs>::max())->q[0] == 1);
  CHECK(certified(curve(0, 1e-310, -1e-310, 0), e));
}
static InboxConfig config() {
  InboxConfig c{}; c.limits = envelope(10, 10, 10, 10); c.budget = {8, 12264};
  c.continuity = {1e-10, 1e-10, 1e-10}; c.max_future_us = 10000000; c.lease_us = 5000000;
  return c;
}
static ArmChecks qualified() { return {true, true, true, true, true}; }
static TrajectoryRequest request(SessionKey key, std::uint64_t seq, CubicSegment s) {
  return {1, key, seq, 42, s.start_us, s};
}
static void transactions_suite() {
  const auto c = config();
  TrajectoryInbox<1, 2> inbox(91, c);
  CHECK(inbox.state() == ExecutionState::Disarmed);
  auto key = inbox.new_session(42, 0); CHECK(key); CHECK(inbox.queued() == 0);
  auto s1 = from_state(1000000, {}, 1);
  auto s2 = from_state(1250000, *evaluate(s1, 1250000), -1);
  auto s3 = from_state(1500000, *evaluate(s2, 1500000), -1);
  auto s4 = from_state(1750000, *evaluate(s3, 1750000), 1);
  CHECK(std::abs(evaluate(s4, 2000000)->v[0]) < 1e-14);
  CHECK(std::abs(evaluate(s4, 2000000)->a[0]) < 1e-14);
  auto one = request(*key, 1, s1); auto two = request(*key, 2, s2);
  CHECK(inbox.submit(one, FixedClock{0}).code == AdmissionCode::NotReady);
  CHECK(inbox.arm(*key, {}, 1000000, {}, 0) == Status::NotReady);
  CHECK(inbox.arm(*key, qualified(), 1000000, {}, 0) == Status::Ok);
  auto bad = one; bad.segment.control_rad[3][0] = 100;
  CHECK(inbox.submit(bad, FixedClock{0}).code == AdmissionCode::CurveRejected);
  CHECK(inbox.queued() == 0);
  CHECK(inbox.submit(one, FixedClock{0}).code == AdmissionCode::Accepted); // Rejection did not consume ID.
  CHECK(inbox.submit(one, FixedClock{1}).code == AdmissionCode::Duplicate && inbox.queued() == 1);
  bad = one; bad.expires_us -= 1;
  CHECK(inbox.submit(bad, FixedClock{1}).code == AdmissionCode::Conflict);
  bad = one; bad.segment.control_rad[2][2] += 0.1;
  CHECK(inbox.submit(bad, FixedClock{1}).code == AdmissionCode::Conflict);
  CHECK(inbox.submit(two, FixedClock{1}).code == AdmissionCode::Full);
  CHECK(inbox.take_for_preparation(1)->sequence == 1);
  CHECK(inbox.submit(one, FixedClock{1}).delivery == Delivery::Prepared);
  CHECK(inbox.submit(two, FixedClock{1}).code == AdmissionCode::Accepted); // Full did not consume ID or tail.
  CHECK(inbox.queued() == 1);
  CHECK(inbox.take_for_preparation(1)->sequence == 2);
  auto three = request(*key, 3, s3);
  bad = three; bad.segment.start_us += 1;
  CHECK(inbox.submit(bad, FixedClock{1}).code == AdmissionCode::Discontinuous);
  bad = three; for (auto& p : bad.segment.control_rad) p[0] += 0.01;
  CHECK(inbox.submit(bad, FixedClock{1}).code == AdmissionCode::Discontinuous);
  bad = three; bad.segment.control_rad[1][0] += 0.0001;
  CHECK(inbox.submit(bad, FixedClock{1}).code == AdmissionCode::Discontinuous);
  bad = three; bad.segment.control_rad[2][0] += 0.0001;
  CHECK(inbox.submit(bad, FixedClock{1}).code == AdmissionCode::Discontinuous);
  CHECK(inbox.submit(three, FixedClock{1}).code == AdmissionCode::Accepted);
  CHECK(inbox.submit(one, FixedClock{1}).code == AdmissionCode::OutcomeUnknown); // Receipt evicted, never replayed.
  CHECK(inbox.take_for_preparation(1)->sequence == 3);
  auto four = request(*key, 4, s4); CHECK(inbox.submit(four, FixedClock{1}).code == AdmissionCode::Accepted);
  inbox.trip(Fault::Interlock);
  CHECK(inbox.state() == ExecutionState::Faulted && inbox.queued() == 0);
  CHECK(!inbox.take_for_preparation(2));
  CHECK(inbox.submit(four, FixedClock{2}).delivery == Delivery::Cancelled);
  CHECK(inbox.submit(three, FixedClock{2}).delivery == Delivery::OutcomeUnknown);
  CHECK(inbox.acknowledge_fault(false) == Status::NotReady);
  CHECK(inbox.acknowledge_fault(true) == Status::Ok);
  CHECK(inbox.state() == ExecutionState::Disarmed);
  CHECK(inbox.arm(*key, qualified(), 3000000, {}, 3) == Status::NotReady);
  CHECK(inbox.submit(four, FixedClock{2000001}).code == AdmissionCode::Duplicate); // Historical receipt, not expiry revival.
  auto next_key = inbox.new_session(42, 2000001); CHECK(next_key && next_key->session > key->session);
  CHECK(inbox.submit(one, FixedClock{2000001}).code == AdmissionCode::OldSession);
  CHECK(inbox.state() == ExecutionState::Disarmed);

  TrajectoryInbox<1, 2> limits(91, c); key = limits.new_session(42, 0);
  CHECK(limits.arm(*key, qualified(), 1000000, {}, 0) == Status::Ok);
  one = request(*key, 1, s1);
  bad = one; bad.sequence = 0; CHECK(limits.submit(bad, FixedClock{0}).code == AdmissionCode::Invalid);
  bad = one; bad.version = 2; CHECK(limits.submit(bad, FixedClock{0}).code == AdmissionCode::Unsupported);
  bad = one; bad.model_revision = 43; CHECK(limits.submit(bad, FixedClock{0}).code == AdmissionCode::ModelMismatch);
  bad = one; bad.owner.boot = 92; CHECK(limits.submit(bad, FixedClock{0}).code == AdmissionCode::OldSession);
  bad = one; bad.expires_us = 0; CHECK(limits.submit(bad, FixedClock{0}).code == AdmissionCode::Expired);
  bad = one; bad.expires_us = bad.segment.start_us+1; CHECK(limits.submit(bad, FixedClock{0}).code == AdmissionCode::OutsideHorizon);
  bad = one; bad.segment.start_us = c.max_future_us;
  CHECK(limits.submit(bad, FixedClock{0}).code == AdmissionCode::OutsideHorizon);
  bad = one; bad.segment.format = 99; CHECK(limits.submit(bad, FixedClock{0}).code == AdmissionCode::Unsupported);
  CHECK(limits.submit(one, FixedClock{0}).code == AdmissionCode::Accepted);
  limits.disarm(0); CHECK(limits.queued() == 0 && limits.state() == ExecutionState::Disarmed);
  CHECK(limits.arm(*key, qualified(), 1000000, {}, 1) == Status::NotReady);
  CHECK(limits.submit(one, FixedClock{1}).delivery == Delivery::Cancelled);

  auto short_lease = c; short_lease.lease_us = 100;
  TrajectoryInbox<1, 2> lease(91, short_lease); key = lease.new_session(42, 0);
  CHECK(lease.arm(*key, qualified(), 1000000, {}, 0) == Status::Ok);
  one = request(*key, 1, s1); CHECK(lease.submit(one, FixedClock{0}).code == AdmissionCode::Accepted);
  CHECK(lease.heartbeat(*key, 1, 1000, 10) == Status::Ok);
  CHECK(lease.heartbeat(*key, 1, 1000, 90) == Status::Stale); // Replay cannot extend lease.
  bad = one; bad.version = 2; CHECK(lease.submit(bad, FixedClock{109}).code == AdmissionCode::Unsupported);
  CHECK(lease.heartbeat(*key, 2, 1000, 110) == Status::NotReady);
  CHECK(lease.fault() == Fault::CommunicationsLost && lease.queued() == 0);
  CHECK(lease.submit(one, FixedClock{111}).delivery == Delivery::Cancelled);
  CHECK(lease.new_session(42, 111)); CHECK(lease.state() == ExecutionState::Faulted);

  TrajectoryInbox<1, 2> regressed(91, c); key = regressed.new_session(42, 0);
  CHECK(regressed.arm(*key, qualified(), 1000000, {}, 10) == Status::Ok);
  regressed.service(20); regressed.service(19); CHECK(regressed.fault() == Fault::ClockRegression);
  TrajectoryInbox<1, 2> late(91, c); key = late.new_session(42, 0);
  CHECK(late.arm(*key, qualified(), 1000000, {}, 0) == Status::Ok);
  CHECK(late.submit(request(*key, 1, s1), FixedClock{0}).code == AdmissionCode::Accepted);
  CHECK(!late.take_for_preparation(1000001)); CHECK(late.fault() == Fault::MotionDeadline);

  TrajectoryInbox<1, 2> invalid_config(91, {}); CHECK(!invalid_config.new_session(42, 0));
  TrajectoryInbox<1, 2> invalid_boot(0, c); CHECK(!invalid_boot.new_session(42, 0));
  TrajectoryInbox<1, 2> reference(91, c); key = reference.new_session(42, 0);
  KinematicState moving{}; moving.v[1] = 0.01;
  CHECK(reference.arm(*key, qualified(), 1000000, moving, 0) == Status::Invalid);
  moving = {}; moving.a[1] = std::numeric_limits<double>::quiet_NaN();
  CHECK(reference.arm(*key, qualified(), 1000000, moving, 0) == Status::Invalid);
  CHECK(reference.arm(*key, qualified(), 0, {}, 0) == Status::Invalid);

  // Ownership transfer must service an already-expired lease before disarming.
  TrajectoryInbox<1, 2> transfer(91, short_lease); key = transfer.new_session(42, 0);
  CHECK(transfer.arm(*key, qualified(), 1000000, {}, 0) == Status::Ok);
  CHECK(transfer.new_session(42, 100));
  CHECK(transfer.fault() == Fault::CommunicationsLost);

  // Expiry, motion deadline and lease must be rechecked AFTER expensive validation.
  TrajectoryInbox<1, 2> expired_during_check(91, c); key = expired_during_check.new_session(42, 0);
  CHECK(expired_during_check.arm(*key, qualified(), 1000000, {}, 0) == Status::Ok);
  one = request(*key, 1, s1); one.expires_us = 100;
  AdvancingClock expiry_clock(0, 101);
  CHECK(expired_during_check.submit(one, expiry_clock).code == AdmissionCode::Expired);
  CHECK(expired_during_check.queued() == 0);
  one.expires_us = 1000000;
  CHECK(expired_during_check.submit(one, FixedClock{102}).code == AdmissionCode::Accepted);
  TrajectoryInbox<1, 2> lease_during_check(91, short_lease); key = lease_during_check.new_session(42, 0);
  CHECK(lease_during_check.arm(*key, qualified(), 1000000, {}, 0) == Status::Ok);
  AdvancingClock lease_clock(0, 100);
  CHECK(lease_during_check.submit(request(*key, 1, s1), lease_clock).code == AdmissionCode::Faulted);
  CHECK(lease_during_check.fault() == Fault::CommunicationsLost && lease_during_check.queued() == 0);
  TrajectoryInbox<1, 2> backwards_during_check(91, c); key = backwards_during_check.new_session(42, 0);
  CHECK(backwards_during_check.arm(*key, qualified(), 1000000, {}, 0) == Status::Ok);
  AdvancingClock backwards_clock(10, 9);
  CHECK(backwards_during_check.submit(request(*key, 1, s1), backwards_clock).code == AdmissionCode::Faulted);
  CHECK(backwards_during_check.fault() == Fault::ClockRegression);
  TrajectoryInbox<1, 2> late_during_check(91, c); key = late_during_check.new_session(42, 0);
  CHECK(late_during_check.arm(*key, qualified(), 1000000, {}, 0) == Status::Ok);
  AdvancingClock late_clock(0, 1000000);
  CHECK(late_during_check.submit(request(*key, 1, s1), late_clock).code == AdmissionCode::Expired);
  CHECK(late_during_check.queued() == 0);

  // Repeated ring wrap, discarded acknowledgements, and out-of-window replay.
  TrajectoryInbox<2, 4> ring(91, c); key = ring.new_session(42, 0);
  CHECK(ring.arm(*key, qualified(), 1000000, {}, 0) == Status::Ok);
  for (std::uint64_t seq = 1; seq <= 512; ++seq) {
    auto hold = curve(0, 0, 0, 0); hold.duration_us = 1000;
    hold.start_us = 1000000 + (seq-1)*1000;
    const auto cmd = request(*key, seq, hold);
    CHECK(ring.submit(cmd, FixedClock{0}).code == AdmissionCode::Accepted);
    CHECK(ring.submit(cmd, FixedClock{0}).code == AdmissionCode::Duplicate);
    CHECK(ring.queued() == 1);
    CHECK(ring.take_for_preparation(0)->sequence == seq);
    CHECK(ring.submit(cmd, FixedClock{0}).delivery == Delivery::Prepared);
  }
  auto hold = curve(0, 0, 0, 0); hold.start_us = 1512000; hold.duration_us = 1000;
  CHECK(ring.submit(request(*key, std::numeric_limits<std::uint64_t>::max(), hold), FixedClock{0}).code == AdmissionCode::Accepted);
  CHECK(ring.submit(request(*key, 1, hold), FixedClock{0}).code == AdmissionCode::OutcomeUnknown);
  CHECK(ring.submit(request(*key, 0, hold), FixedClock{0}).code == AdmissionCode::Invalid);
  ring.trip(Fault::None); CHECK(ring.queued() == 1);

  // Fixed-storage paths must not call C++ allocation on the tested host.
  allocation_forbidden = true;
  TrajectoryInbox<2, 4> no_heap(99, c); const auto nk = no_heap.new_session(42, 0);
  CHECK(no_heap.arm(*nk, qualified(), 1000000, {}, 0) == Status::Ok);
  CHECK(no_heap.submit(request(*nk, 1, s1), FixedClock{0}).code == AdmissionCode::Accepted);
  CHECK(no_heap.take_for_preparation(0)); no_heap.trip(Fault::Drive);
  allocation_forbidden = false;
}
static std::uint64_t rng_state = 0x73a56c129edULL;
static double random_signed() {
  rng_state = rng_state * 6364136223846793005ULL + 1442695040888963407ULL;
  return static_cast<double>((rng_state >> 32) % 2000001) / 1000000.0 - 1.0;
}
static void properties_suite() {
  const auto e = envelope(1, 10, 50, 100);
  unsigned accepted = 0, rejected = 0;
  for (unsigned trial = 0; trial < 128; ++trial) {
    auto s = curve(0, 0, 0, 0);
    s.duration_us = 200000 + static_cast<TimeUs>((random_signed()+1) * 1000000);
    for (auto& p : s.control_rad) for (double& q : p) q = random_signed();
    const auto result = validate(s, e, {8, 12264});
    CHECK(result.nodes <= 12264);
    if (result.status != BoundStatus::Certified) { ++rejected; continue; }
    ++accepted;
    const long double t = static_cast<long double>(s.duration_us) / 1000000.L;
    for (unsigned k = 0; k <= 100; ++k) {
      const TimeUs at = s.start_us + s.duration_us*k/100;
      const long double u = static_cast<long double>(at-s.start_us)/s.duration_us, z = 1-u;
      const auto sampled = evaluate(s, at); CHECK(sampled);
      for (std::size_t axis = 0; axis < kJointCount; ++axis) {
        const long double b0=s.control_rad[0][axis], b1=s.control_rad[1][axis];
        const long double b2=s.control_rad[2][axis], b3=s.control_rad[3][axis];
        // Independent explicit long-double polynomial; sampling is a test oracle,
        // not the continuous acceptance algorithm.
        const long double q=z*z*z*b0+3*z*z*u*b1+3*z*u*u*b2+u*u*u*b3;
        const long double v=3*(z*z*(b1-b0)+2*z*u*(b2-b1)+u*u*(b3-b2))/t;
        const long double a=6*(z*(b2-2*b1+b0)+u*(b3-2*b2+b1))/(t*t);
        const long double j=6*(b3-3*b2+3*b1-b0)/(t*t*t);
        CHECK(q >= -1 && q <= 1); CHECK(std::abs(v) <= 10);
        CHECK(std::abs(a) <= 50); CHECK(std::abs(j) <= 100);
        CHECK(std::abs(q-sampled->q[axis]) < 1e-12L);
        CHECK(std::abs(v-sampled->v[axis]) < 1e-11L);
        CHECK(std::abs(a-sampled->a[axis]) < 1e-10L);
      }
    }
  }
  CHECK(accepted > 0 && rejected > 0);
  std::printf("Property corpus: %u certified / %u rejected\n", accepted, rejected);
}
int main(int argc, char** argv) {
  if (argc != 2) return 2;
  if (std::strcmp(argv[1], "trajectory_bounds") == 0) bounds_suite();
  else if (std::strcmp(argv[1], "trajectory_transactions") == 0) transactions_suite();
  else if (std::strcmp(argv[1], "trajectory_properties") == 0) properties_suite();
  else return 2;
  std::printf("PASS %s: %zu checks\n", argv[1], checks);
}
