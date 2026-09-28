#pragma once
#include "helix/core/types.hpp"
namespace helix {
class MonotonicClock {
 public:
  virtual ~MonotonicClock() = default;
  virtual TimeUs now_us() const noexcept = 0;
};
// Board implementation must extend timer wrap coherently and document ISR access.
} // namespace helix
