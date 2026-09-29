#include <cstdio>
#include "helix/protocol/trajectory_inbox.hpp"
using namespace helix;
using namespace helix::motion;
using namespace helix::protocol;
class DemoClock final : public MonotonicClock {
 public:
  TimeUs now_us() const noexcept override { return 0; }
};
int main() {
  const DemoClock clock;
  // Synthetic host-only fixture. None of these values is Helix calibration.
  InboxConfig c{};
  for (auto& axis : c.limits) axis = {-10, 10, 10, 10, 10};
  c.budget = {8, 12264}; c.continuity = {1e-10, 1e-10, 1e-10};
  c.max_future_us = 10000000; c.lease_us = 1000;
  c.preparation_lead_us = 100; // Synthetic reserve, not target qualification.
  TrajectoryInbox<1, 4> inbox(123, c);
  const auto key = inbox.new_session(42, 0);
  if (!key || inbox.arm(*key, {true,true,true,true,true}, 1000000, {}, 0) != Status::Ok) return 1;
  KinematicState state{};
  for (std::uint64_t sequence = 1; sequence <= 4; ++sequence) {
    const double j = sequence == 1 || sequence == 4 ? 1.0 : -1.0;
    CubicSegment segment{}; segment.start_us = 1000000 + (sequence-1)*250000; segment.duration_us = 250000;
    constexpr double t = 0.25;
    for (std::size_t axis = 0; axis < kJointCount; ++axis) {
      const double jerk = axis == 0 ? j : 0;
      segment.control_rad[0][axis] = state.q[axis];
      segment.control_rad[1][axis] = state.q[axis] + state.v[axis]*t/3;
      segment.control_rad[2][axis] = state.q[axis] + 2*state.v[axis]*t/3 + state.a[axis]*t*t/6;
      segment.control_rad[3][axis] = state.q[axis] + state.v[axis]*t + state.a[axis]*t*t/2 + jerk*t*t*t/6;
    }
    const TrajectoryRequest command{1, *key, sequence, 42, segment.start_us, segment};
    if (inbox.submit(command, clock).code != AdmissionCode::Accepted) return 2;
    if (inbox.submit(command, clock).code != AdmissionCode::Duplicate || inbox.queued() != 1) return 3;
    const auto ticket = inbox.take_for_preparation(clock.now_us());
    if (!ticket) return 4;
    const auto end = evaluate(segment, segment.start_us+segment.duration_us);
    if (!end) return 5;
    if (inbox.check_preparation(*ticket, clock) != Status::Ok) return 7;
    state = *end;
    std::printf("Segment %llu: bounds accepted, duplicate suppressed, preparation permission rechecked\n",
                static_cast<unsigned long long>(sequence));
  }
  inbox.service(1000);
  if (inbox.fault() != Fault::CommunicationsLost || inbox.queued() != 0) return 6;
  std::printf("Final commanded reference: q=%.8f rad, v=%.8f rad/s, a=%.8f rad/s^2\n", state.q[0], state.v[0], state.a[0]);
  std::puts("Lease expiry latched. Host preparation only: no motion executed and no motor outputs enabled.");
}
