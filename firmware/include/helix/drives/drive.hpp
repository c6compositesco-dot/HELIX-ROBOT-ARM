#pragma once
#include "helix/core/types.hpp"
#include "helix/feedback/sample.hpp"

namespace helix {
enum class DriveMode { Position, Velocity, Torque };
struct DriveCapabilities {
  bool operational{false};
  bool position{false};
  bool velocity{false};
  bool torque{false};
  bool motor_encoder{false};
  bool output_encoder{false};
  bool timestamped_commands{false};
};
struct JointCommand {
  DriveMode mode{DriveMode::Position};
  double position_rad{0.0};
  double velocity_rad_s{0.0};
  double torque_nm{0.0};
  TimeUs apply_at_us{0};
};
// Robot-side units. Each adapter owns conversion to its motor/vendor units.
inline Status validate_command(const JointCommand& command, const DriveCapabilities& caps,
                               TimeUs now) noexcept {
  if (!caps.operational) return Status::NotReady;
  if (!std::isfinite(command.position_rad) || !std::isfinite(command.velocity_rad_s)
      || !std::isfinite(command.torque_nm)) return Status::Invalid;
  switch (command.mode) {
    case DriveMode::Position: if (!caps.position) return Status::Unsupported; break;
    case DriveMode::Velocity: if (!caps.velocity) return Status::Unsupported; break;
    case DriveMode::Torque: if (!caps.torque) return Status::Unsupported; break;
    default: return Status::Invalid;
  }
  if (command.apply_at_us != 0) {
    if (!caps.timestamped_commands) return Status::Unsupported;
    if (command.apply_at_us <= now) return Status::Stale;
  }
  return Status::Ok;
}
class Drive {
 public:
  virtual ~Drive() = default;
  virtual DriveCapabilities capabilities() const noexcept = 0;
  virtual Status enable() noexcept = 0;
  virtual Status submit(const JointCommand& command, TimeUs now) noexcept = 0;
  virtual void request_stop() noexcept = 0;
  virtual JointFeedback feedback() const noexcept = 0;
};
// Explicit fail-closed placeholder, never a simulated-success hardware driver.
class UnavailableDrive final : public Drive {
 public:
  DriveCapabilities capabilities() const noexcept override { return {}; }
  Status enable() noexcept override { return Status::NotImplemented; }
  Status submit(const JointCommand&, TimeUs) noexcept override { return Status::NotImplemented; }
  void request_stop() noexcept override {} // No hardware exists in this adapter.
  JointFeedback feedback() const noexcept override { return {}; }
};
} // namespace helix
