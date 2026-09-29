# Host USB transport

STATUS: no driver or packet codec implemented. `protocol/admission.hpp` is an in-memory, single-owner admission model only.

Implement bounded framing, lengths, endianness, integrity checking, version negotiation, acknowledgements and backpressure. Never transmit C++ object layouts directly. Validate complete frames before admission; define retry/duplicate response semantics. A new connection requires a new session and explicit runtime authorization. Fault/disconnect must flush old commands in the eventual integration.

USB carries plans and state; arrival timing does not schedule motor pulses. USB disconnect is not an emergency-stop channel. No network authentication or security boundary is claimed by sequence checking.
