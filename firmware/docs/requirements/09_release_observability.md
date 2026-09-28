# Observability, verification, release and trust boundaries

Sources: [S18], [S20], [S23]. Status: specification-only. Issue #11. A passing requirements linter verifies structure/references, not that these behaviors exist.

## Evidence policy

Separate four states: specified, implemented, software-verified and hardware-qualified. Evidence records identify exact source/build/configuration, test method, pass criteria, raw results and limitations. Keep numerical correctness, worst-case timing, physical stopping, electrical/thermal endurance and accuracy claims separate. Host unit-test counts are not a percentage of robot safety or product completion.

Use deterministic host tests, randomized/property tests with saved seeds, malformed-input fuzzing, sanitizers and independent numerical references; then cross-compiled target tests and hardware-in-the-loop fault/timing campaigns. Pin all dependencies and inspect licensing before importing code. The original binary's unknown corresponding source remains separate from the new MIT-licensed foundation.

NIST firmware-resilience guidance supplies a protect/detect/recover pattern [S20], not a certification for this board. Design updates around actual bootloader/flash resources: do not promise dual-bank rollback merely from aggregate flash size. Keep the MCU off the public internet; any later remote agent access belongs behind an authenticated host gateway and explicit local machine authorization.

## Observability requirements

### OBS-001: Bounded event and fault records
Phase: R1; Issue: #11; Origin: derived
Requirement: Runtime events shall record monotonic time, boot/session/configuration identity, command/task ID, relevant state and first fault cause using bounded nonblocking storage.
Acceptance: Saturated logging cannot delay motion/fault deadlines; records retain ordering and expose dropped-event counts.

### OBS-002: Honest state telemetry
Phase: R1; Issue: #11; Origin: derived
Requirement: Telemetry shall label command/estimate/measurement source, validity, reference, age and units, plus actual drive/motion/permission states. Unavailable values shall not be rendered as plausible zero measurements.
Acceptance: Invalid/stale/absent sensor and interrupted-motion fixtures produce unambiguous host displays and machine-readable statuses.

### OBS-003: Timing and resource instrumentation
Phase: R1; Issue: #11; Origin: derived
Requirement: Diagnostic builds shall measure deadlines, blocking, queue horizons, errors, memory/stack usage and reset causes with a documented instrumentation cost.
Acceptance: Compare instrumented and release profiles; measurements cover concurrent worst-case workloads without using average timing as a hard guarantee.

### OBS-004: Versioned trace replay
Phase: R1; Issue: #10; Origin: derived
Requirement: Captured traces shall identify formats and configurations and support deterministic replay in host fixtures without actuating hardware by default.
Acceptance: Reproduce an injected failure from a saved trace; incompatible replay formats fail and replay tools cannot accidentally open live actuation channels.

### OBS-005: Privacy and diagnostic scope
Phase: R0; Issue: #11; Origin: derived
Requirement: Public tests and issue reports shall exclude private CAD, calibration, credentials and unnecessary user data. Operational logging/exports shall expose their scope and remain under user control.
Acceptance: Review repository fixtures and release artifacts; synthetic data suffices for public regression cases and secrets are absent.

## Release requirements

### REL-001: Traceable requirement evidence
Phase: R0; Issue: #11; Origin: derived
Requirement: Each requirement shall have a unique ID, introduction phase, owning issue, origin and acceptance procedure. Verification claims shall link a reviewed evidence record rather than derive from document presence.
Acceptance: CI rejects missing/duplicate/malformed metadata and unknown references; exported records remain specified until separate evidence is attached.

### REL-002: Reproducible dependency set
Phase: R1; Issue: #2; Origin: derived
Requirement: Builds shall pin compatible toolchain, CMSIS/device/HAL and numerical dependencies, with licence/provenance records and reproducible configuration/image identifiers.
Acceptance: A clean build reproduces declared outputs or documents unavoidable variance; dependency changes rerun applicable conformance/timing tests.

### REL-003: Host verification matrix
Phase: R0; Issue: #11; Origin: derived
Requirement: Host checks shall run in Debug/Release with supported compilers, finite-input protections and sanitizers where available. Tests shall remain active in Release and include negative cases.
Acceptance: CI runs the matrix and fails deliberately injected regressions; passing tests are reported by exact revision without implying MCU qualification.

### REL-004: Target and physical evidence
Phase: R1; Issue: #11; Origin: derived
Requirement: Enabled hardware capabilities shall pass target timing, IO/fault and physical qualification campaigns against frozen configuration-specific criteria.
Acceptance: Release records include raw timing/stop/thermal/reference evidence and unresolved limitations; simulation or host checks alone cannot mark hardware qualified.

### REL-005: Capability-scoped release gates
Phase: R1; Issue: #11; Origin: derived
Requirement: A release shall enable only qualified capabilities. Future encoder/network/brushless requirements shall not block a qualified open-loop release, but shall block advertising their own unsupported functions.
Acceptance: Release manifests and discovery responses agree with test evidence; incomplete hardware profiles cannot generate an approved powered image.

### REL-006: Controlled firmware update and recovery
Phase: R1; Issue: #11; Origin: derived
Requirement: Updates shall require authorized non-moving/support conditions, image identity/integrity checks and a tested recovery procedure consistent with actual flash/bootloader resources. Production authenticity requirements shall be resolved in the threat model, not replaced by a CRC.
Acceptance: Wrong-target, corrupt and interrupted updates are tested on a recoverable fixture; reboot stays disarmed and incompatible state/calibration is not restored silently.

### REL-007: Regression and claim discipline
Phase: R1; Issue: #11; Origin: derived
Requirement: Every defect shall gain a regression case where feasible; performance and accuracy claims shall identify configuration, method, uncertainty and limitations. Changes to motion/safety-related code shall invalidate affected qualification evidence.
Acceptance: Review release diffs against the dependency/evidence map; claims cannot survive a relevant untested change solely because the version number is newer.

## Security and authority requirements

### SEC-001: Explicit trust boundary
Phase: R1; Issue: #11; Origin: derived
Requirement: The MCU shall not be exposed as a public internet agent endpoint. Any future remote service shall authenticate/authorize users at a host gateway and retain explicit local machine-enable policy.
Acceptance: Threat-model tests show remote identity alone cannot bypass local permission, limits, fault handling or configuration approval.

### SEC-002: Integrity is not authorization
Phase: R1; Issue: #5; Origin: derived
Requirement: CRC, command sequence and session matching shall not be described as cryptographic authentication. The selected deployment shall define who can open a session, change limits and update firmware.
Acceptance: Replay, tampering and unauthorized configuration tests are evaluated against the declared boundary; documentation makes no unsupported security guarantee.

### SEC-003: Bounded untrusted inputs
Phase: R1; Issue: #5; Origin: derived
Requirement: Task/model/trajectory/protocol data shall be treated as untrusted, with bounded parsing, validation and least-privilege operations. No payload may directly write arbitrary registers or execute native code.
Acceptance: Fuzz malformed and resource-exhaustion inputs under sanitizers and on target parser fixtures; invalid data produces no actuator side effects.

### SEC-004: Auditable privileged changes
Phase: R1; Issue: #11; Origin: derived
Requirement: Changes to calibration, motion limits, gains, supported tools, firmware and authorization policy shall be explicit, auditable and allowed only in qualified states. AI task authority shall not imply unrestricted maintenance authority.
Acceptance: Attempt privileged changes from task-only sessions and while moving; reject them and record the event without disrupting the qualified execution path.
