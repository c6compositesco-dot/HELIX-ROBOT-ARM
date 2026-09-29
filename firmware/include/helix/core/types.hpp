#pragma once
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace helix {
// This first ABI targets six rotary joints; do not silently change wire axis count.
constexpr std::size_t kJointCount = 6;
using TimeUs = std::uint64_t;
using JointVector = std::array<double, kJointCount>;
enum class Status {
  Ok, Invalid, NotReady, Unsupported, OutOfRange, Stale, Full,
  Faulted, Deadline, NoConvergence, NotImplemented, ModelMismatch
};
struct JointLimits {
  double lower_rad{0.0};
  double upper_rad{0.0};
  double max_velocity_rad_s{0.0};
  bool valid() const noexcept {
    return std::isfinite(lower_rad) && std::isfinite(upper_rad)
        && std::isfinite(max_velocity_rad_s) && lower_rad < upper_rad
        && max_velocity_rad_s > 0.0;
  }
};
using RobotLimits = std::array<JointLimits, kJointCount>;
} // namespace helix
