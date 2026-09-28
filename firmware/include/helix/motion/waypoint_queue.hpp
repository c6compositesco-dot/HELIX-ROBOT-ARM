#pragma once
#include <optional>
#include "helix/core/types.hpp"

namespace helix {
struct JointWaypoint {
  TimeUs apply_at_us{0};
  JointVector position_rad{};
  JointVector velocity_rad_s{};
};
// Single-owner queue. This is NOT an ISR-safe queue or a trajectory generator.
// Validates points only: segment extrema, acceleration, jerk, stopping horizon,
// collision approval and continuity require a future trajectory validator.
template<std::size_t Capacity>
class WaypointQueue {
  static_assert(Capacity > 0, "Queue capacity must be positive");
 public:
  explicit WaypointQueue(const RobotLimits& limits) noexcept : limits_(limits) {}
  Status push(const JointWaypoint& point, TimeUs now) noexcept {
    if (point.apply_at_us <= now || (last_time_ && point.apply_at_us <= *last_time_))
      return Status::Stale;
    for (std::size_t i = 0; i < kJointCount; ++i) {
      const auto& limit = limits_[i];
      if (!limit.valid() || !std::isfinite(point.position_rad[i])
          || !std::isfinite(point.velocity_rad_s[i])) return Status::Invalid;
      if (point.position_rad[i] < limit.lower_rad || point.position_rad[i] > limit.upper_rad
          || std::abs(point.velocity_rad_s[i]) > limit.max_velocity_rad_s)
        return Status::OutOfRange;
    }
    if (size_ == Capacity) return Status::Full;
    data_[(head_ + size_) % Capacity] = point;
    ++size_;
    last_time_ = point.apply_at_us;
    return Status::Ok;
  }
  std::optional<JointWaypoint> pop() noexcept {
    if (size_ == 0) return std::nullopt;
    const auto result = data_[head_];
    head_ = (head_ + 1) % Capacity;
    --size_;
    return result;
  }
  void clear() noexcept { head_ = 0; size_ = 0; last_time_.reset(); }
  std::size_t size() const noexcept { return size_; }
 private:
  RobotLimits limits_{};
  std::array<JointWaypoint, Capacity> data_{};
  std::size_t head_{0};
  std::size_t size_{0};
  std::optional<TimeUs> last_time_{};
};
} // namespace helix
