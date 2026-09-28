#pragma once
#include "helix/core/types.hpp"

namespace helix {
enum class TaskOp { MoveTrajectory, WaitInput, SetTool, End };
struct TaskInstruction {
  TaskOp op{TaskOp::End};
  std::uint32_t resource_id{0};
  TimeUs timeout_us{0};
};
// Bounded description only. No task executor, resources or hardware bindings yet.
// No unbounded loops or jumps in the initial program shape.
template<std::size_t Capacity>
class TaskProgram {
  static_assert(Capacity > 0, "Task capacity must be positive");
 public:
  Status append(const TaskInstruction& instruction) noexcept {
    if (ended_) return Status::Invalid;
    switch (instruction.op) {
      case TaskOp::MoveTrajectory:
      case TaskOp::WaitInput:
      case TaskOp::SetTool:
        if (instruction.resource_id == 0 || instruction.timeout_us == 0) return Status::Invalid;
        break;
      case TaskOp::End: break;
      default: return Status::Invalid;
    }
    if (size_ == Capacity) return Status::Full;
    instructions_[size_++] = instruction;
    ended_ = instruction.op == TaskOp::End;
    return Status::Ok;
  }
  bool complete() const noexcept { return ended_; } // Syntax only, not execution approval.
  std::size_t size() const noexcept { return size_; }
 private:
  std::array<TaskInstruction, Capacity> instructions_{};
  std::size_t size_{0};
  bool ended_{false};
};
} // namespace helix
