#pragma once
#include <algorithm>
#include <limits>
#include <optional>
#include "helix/core/types.hpp"

#if defined(__FAST_MATH__) || (defined(__FINITE_MATH_ONLY__) && __FINITE_MATH_ONLY__ > 0)
#error "Continuous interval validation requires strict floating-point semantics"
#endif

namespace helix::motion {
static_assert(std::numeric_limits<double>::is_iec559
              && std::numeric_limits<double>::digits == 53,
              "This validation implementation requires IEEE-754 binary64");

// Local C++ representation, NOT a wire struct. Cubic Bezier control positions,
// normalized over [start_us, start_us + duration_us], in joint-side radians.
// Each cubic has constant jerk. Adjacent segments require a separate C2 check.
struct CubicSegment {
  std::uint16_t format{1};
  TimeUs start_us{0};
  TimeUs duration_us{0};
  std::array<JointVector, 4> control_rad{};
};
struct AxisEnvelope {
  double lower_rad{0}, upper_rad{0};
  double velocity_rad_s{0}, acceleration_rad_s2{0}, jerk_rad_s3{0};
  bool valid() const noexcept {
    return std::isfinite(lower_rad) && std::isfinite(upper_rad)
        && lower_rad < upper_rad && std::isfinite(velocity_rad_s)
        && velocity_rad_s > 0 && std::isfinite(acceleration_rad_s2)
        && acceleration_rad_s2 > 0 && std::isfinite(jerk_rad_s3) && jerk_rad_s3 > 0;
  }
};
using MotionEnvelope = std::array<AxisEnvelope, kJointCount>;
struct KinematicState { JointVector q{}, v{}, a{}; };
enum class BoundStatus { Certified, Invalid, Unsupported, Violation, Uncertified };
struct BoundReport {
  BoundStatus status{BoundStatus::Invalid};
  std::size_t axis{0};
  unsigned derivative{0}; // 0 position, 1 velocity, 2 acceleration, 3 jerk.
  std::uint32_t nodes{0};
};
struct BoundBudget {
  unsigned max_depth{0};
  std::uint32_t max_nodes{0}; // Total across all six axes and all four quantities.
  bool valid() const noexcept { return max_depth <= 8 && max_nodes > 0 && max_nodes <= 12264; }
};

inline bool valid_shape(const CubicSegment& s) noexcept {
  constexpr TimeUs max_exact_integer = TimeUs{1} << 53;
  if (s.format != 1 || s.duration_us == 0 || s.duration_us > max_exact_integer
      || s.start_us > std::numeric_limits<TimeUs>::max() - s.duration_us) return false;
  for (const auto& point : s.control_rad)
    for (double q : point) if (!std::isfinite(q)) return false;
  return true;
}

namespace detail {
struct Interval { double lo{0}, hi{0}; };
inline bool finite(Interval x) noexcept {
  return std::isfinite(x.lo) && std::isfinite(x.hi) && x.lo <= x.hi;
}
inline Interval outward(double lo, double hi) noexcept {
  const double inf = std::numeric_limits<double>::infinity();
  return {std::nextafter(lo, -inf), std::nextafter(hi, inf)};
}
inline Interval add(Interval a, Interval b) noexcept {
  if (a.lo == 0 && a.hi == 0) return b;
  if (b.lo == 0 && b.hi == 0) return a;
  return outward(a.lo + b.lo, a.hi + b.hi);
}
inline Interval subtract(Interval a, Interval b) noexcept {
  if (a.lo == a.hi && b.lo == b.hi && a.lo == b.lo) return {};
  return add(a, {-b.hi, -b.lo});
}
inline Interval multiply(Interval a, Interval b) noexcept {
  if ((a.lo == 0 && a.hi == 0) || (b.lo == 0 && b.hi == 0)) return {};
  const std::array<double, 4> p{{a.lo*b.lo, a.lo*b.hi, a.hi*b.lo, a.hi*b.hi}};
  for (double x : p) if (!std::isfinite(x)) {
    const double nan = std::numeric_limits<double>::quiet_NaN();
    return {nan, nan};
  }
  return outward(*std::min_element(p.begin(), p.end()), *std::max_element(p.begin(), p.end()));
}
using Controls = std::array<Interval, 4>;
inline void split(const Controls& input, unsigned degree, Controls& left, Controls& right) noexcept {
  auto row = input;
  left[0] = row[0]; right[degree] = row[degree];
  for (unsigned level = 1; level <= degree; ++level) {
    for (unsigned i = 0; i <= degree - level; ++i)
      row[i] = add(multiply(row[i], {0.5, 0.5}), multiply(row[i+1], {0.5, 0.5}));
    left[level] = row[0]; right[degree-level] = row[degree-level];
  }
}
inline BoundStatus certify(Controls points, unsigned degree, double lo, double hi,
                           BoundBudget budget, std::uint32_t& visited) noexcept {
  struct Node { Controls p{}; unsigned depth{0}; };
  std::array<Node, 9> stack{}; // Depth-first traversal, fixed maximum depth + 1.
  std::size_t count = 1;
  stack[0] = {points, 0};
  while (count != 0) {
    if (visited >= budget.max_nodes) return BoundStatus::Uncertified;
    ++visited;
    const Node node = stack[--count];
    bool contained = true;
    for (unsigned i = 0; i <= degree; ++i) {
      if (!finite(node.p[i])) return BoundStatus::Invalid;
      contained = contained && node.p[i].lo >= lo && node.p[i].hi <= hi;
    }
    if (contained) continue;
    // Only actual endpoints, not out-of-range inner control vertices, prove violation.
    for (unsigned i : {0U, degree})
      if (node.p[i].hi < lo || node.p[i].lo > hi) return BoundStatus::Violation;
    if (node.depth >= budget.max_depth) return BoundStatus::Uncertified;
    Controls left{}, right{};
    split(node.p, degree, left, right);
    stack[count++] = {right, node.depth + 1};
    stack[count++] = {left, node.depth + 1};
  }
  return BoundStatus::Certified;
}
inline double bezier(std::array<double, 4> p, unsigned degree, double u) noexcept {
  if (u == 0) return p[0];
  if (u == 1) return p[degree];
  for (unsigned n = degree; n > 0; --n)
    for (unsigned i = 0; i < n; ++i) p[i] = (1-u)*p[i] + u*p[i+1];
  return p[0];
}
} // namespace detail

// Certifies the mathematical polynomial via outward-rounded Bernstein hulls.
// Requires strict IEEE arithmetic with gradual underflow (no FTZ/DAZ/fast-math).
// No sampling proof, no heap, bounded subdivision; uncertainty rejects admission.
// This is NOT collision validation, a stop planner or MCU timing qualification.
inline BoundReport validate(const CubicSegment& s, const MotionEnvelope& limits,
                            BoundBudget budget) noexcept {
  BoundReport report{};
  if (s.format != 1) { report.status = BoundStatus::Unsupported; return report; }
  if (!valid_shape(s) || !budget.valid()) return report;
  for (const auto& limit : limits) if (!limit.valid()) return report;
  // duration_us <= 2^53 ensures exact integer conversion. Division is rounded outward.
  const auto inv_t = detail::outward(1000000.0 / static_cast<double>(s.duration_us),
                                    1000000.0 / static_cast<double>(s.duration_us));
  for (std::size_t axis = 0; axis < kJointCount; ++axis) {
    report.axis = axis;
    detail::Controls controls{};
    for (unsigned i = 0; i < 4; ++i) controls[i] = {s.control_rad[i][axis], s.control_rad[i][axis]};
    const auto& l = limits[axis];
    const std::array<double, 4> low{{l.lower_rad, -l.velocity_rad_s, -l.acceleration_rad_s2, -l.jerk_rad_s3}};
    const std::array<double, 4> high{{l.upper_rad, l.velocity_rad_s, l.acceleration_rad_s2, l.jerk_rad_s3}};
    for (unsigned d = 0; d <= 3; ++d) {
      report.derivative = d;
      report.status = detail::certify(controls, 3-d, low[d], high[d], budget, report.nodes);
      if (report.status != BoundStatus::Certified) return report;
      if (d != 3) {
        const auto scale = detail::multiply(inv_t, {static_cast<double>(3-d), static_cast<double>(3-d)});
        for (unsigned i = 0; i < 3-d; ++i)
          controls[i] = detail::multiply(detail::subtract(controls[i+1], controls[i]), scale);
      }
    }
  }
  return report;
}

// Numerical evaluator for host/firmware conformance. Inclusive endpoints; no extrapolation.
// Applied-pulse quantization and floating evaluation error need separate runtime margins.
inline std::optional<KinematicState> evaluate(const CubicSegment& s, TimeUs at) noexcept {
  if (!valid_shape(s) || at < s.start_us || at - s.start_us > s.duration_us) return std::nullopt;
  KinematicState out{};
  const double inv_t = 1000000.0 / static_cast<double>(s.duration_us);
  const double u = static_cast<double>(at - s.start_us) / static_cast<double>(s.duration_us);
  for (std::size_t axis = 0; axis < kJointCount; ++axis) {
    std::array<double, 4> p{};
    for (unsigned i = 0; i < 4; ++i) p[i] = s.control_rad[i][axis];
    out.q[axis] = detail::bezier(p, 3, u);
    for (unsigned i = 0; i < 3; ++i) p[i] = (p[i+1] - p[i]) * 3 * inv_t;
    out.v[axis] = detail::bezier(p, 2, u);
    for (unsigned i = 0; i < 2; ++i) p[i] = (p[i+1] - p[i]) * 2 * inv_t;
    out.a[axis] = detail::bezier(p, 1, u);
    if (!std::isfinite(out.q[axis]) || !std::isfinite(out.v[axis]) || !std::isfinite(out.a[axis]))
      return std::nullopt;
  }
  return out;
}
} // namespace helix::motion
