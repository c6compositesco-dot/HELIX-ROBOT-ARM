#pragma once
#include "helix/core/types.hpp"

namespace helix {
struct ToolPose {
  std::array<double, 3> position_m{};
  std::array<double, 4> quaternion_xyzw{{0.0, 0.0, 0.0, 1.0}};
};
struct IkBudget {
  std::uint32_t max_iterations{0};
  TimeUs deadline_us{0}; // Absolute monotonic deadline; iteration bound alone is insufficient.
};
struct IkRequest {
  ToolPose target{};
  JointVector seed_rad{};
  RobotLimits limits{};
  IkBudget budget{};
  std::uint64_t model_revision{0};
};
struct IkResult {
  Status status{Status::NotImplemented};
  JointVector candidate_rad{}; // Never executable unless status == Ok and separately admitted.
  std::uint32_t iterations{0};
};
class Kinematics {
 public:
  virtual ~Kinematics() = default;
  virtual IkResult inverse(const IkRequest& request) noexcept = 0;
};
class UnavailableKinematics final : public Kinematics {
 public:
  IkResult inverse(const IkRequest&) noexcept override { return {}; }
};
} // namespace helix
