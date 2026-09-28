#pragma once
#include <optional>
#include "helix/core/types.hpp"

namespace helix {
// First adapter only: independent fixed-ratio transmission. Not a coupled model.
struct FixedRatioTransmission {
  double motor_per_joint{0.0};
  int direction{1};
  double motor_zero_rad{0.0};
  double joint_zero_rad{0.0};
  bool valid() const noexcept {
    return std::isfinite(motor_per_joint) && motor_per_joint > 0.0
        && (direction == 1 || direction == -1)
        && std::isfinite(motor_zero_rad) && std::isfinite(joint_zero_rad);
  }
  std::optional<double> to_joint(double motor_rad) const noexcept {
    if (!valid() || !std::isfinite(motor_rad)) return std::nullopt;
    const double value = joint_zero_rad
        + direction * ((motor_rad - motor_zero_rad) / motor_per_joint);
    return std::isfinite(value) ? std::optional<double>{value} : std::nullopt;
  }
  std::optional<double> to_motor(double joint_rad) const noexcept {
    if (!valid() || !std::isfinite(joint_rad)) return std::nullopt;
    const double value = motor_zero_rad
        + direction * (joint_rad - joint_zero_rad) * motor_per_joint;
    return std::isfinite(value) ? std::optional<double>{value} : std::nullopt;
  }
};
} // namespace helix
