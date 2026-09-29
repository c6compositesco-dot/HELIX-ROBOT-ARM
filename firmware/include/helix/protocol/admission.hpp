#pragma once
#include "helix/core/types.hpp"

namespace helix {
// In-memory contract, NOT a packed wire struct. No encoder/decoder is provided yet.
struct CommandEnvelope {
  std::uint16_t major_version{1};
  std::uint64_t session_id{0};
  std::uint64_t sequence{0};
  std::uint64_t model_revision{0};
  TimeUs expires_at_us{0};
};
// Stateful, single-owner admission. Sequence zero is reserved. Reconnect must use
// a fresh nonzero session and revoke runtime permission separately. This is not authentication.
class CommandAdmission {
 public:
  CommandAdmission(std::uint64_t session, std::uint64_t model) noexcept
      : session_(session), model_(model) {}
  Status admit(const CommandEnvelope& envelope, TimeUs now) noexcept {
    if (session_ == 0 || model_ == 0) return Status::NotReady;
    if (envelope.major_version != 1) return Status::Unsupported;
    if (envelope.session_id != session_) return Status::Stale;
    if (envelope.model_revision != model_) return Status::ModelMismatch;
    if (envelope.expires_at_us <= now || envelope.sequence == 0
        || envelope.sequence <= last_sequence_) return Status::Stale;
    last_sequence_ = envelope.sequence;
    return Status::Ok;
  }
 private:
  std::uint64_t session_{0};
  std::uint64_t model_{0};
  std::uint64_t last_sequence_{0};
};
} // namespace helix
