#pragma once
#include "helix/core/types.hpp"

namespace helix {
enum class SampleSource { Unavailable, StepEstimate, MotorEncoder, OutputEncoder };
struct PositionSample {
  double radians{0.0};
  TimeUs sampled_at_us{0}; // Common controller timebase after adapter conversion.
  SampleSource source{SampleSource::Unavailable};
  bool valid{false};
  bool referenced{false};
};
inline bool fresh(const PositionSample& sample, TimeUs now, TimeUs max_age) noexcept {
  return sample.valid && sample.referenced
      && sample.source != SampleSource::Unavailable && std::isfinite(sample.radians)
      && now >= sample.sampled_at_us && now - sample.sampled_at_us <= max_age;
}
struct JointFeedback {
  PositionSample step_estimate{};       // Joint-side estimate, not measured output.
  PositionSample motor_position{};      // Motor-side radians; map explicitly.
  PositionSample output_position{};     // Joint-side measured radians.
};
// A motor encoder alone does not establish actual link position.
inline bool has_fresh_output(const JointFeedback& feedback, TimeUs now, TimeUs age) noexcept {
  return feedback.output_position.source == SampleSource::OutputEncoder
      && fresh(feedback.output_position, now, age);
}
} // namespace helix
