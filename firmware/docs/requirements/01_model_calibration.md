# Canonical robot model, references and calibration

Sources: [S04], [S05], [S12], [S13]. Status: specification-only. Interfaces: `core/types.hpp`, `joints/transmission.hpp`, `feedback/sample.hpp`, host model adapter and future configuration store. Issue #10 owns the model; #6 supplies measured sensing.

## Design

Use a versioned serial-chain model with explicit parent-to-joint rigid transforms and joint axes. Import DH/URDF/CAD conventions only through tested adapters; do not mix their frame conventions in the core. Model geometry, motor transmission, sensor calibration and tool mounting separately. A fixed signed reduction is the first transmission implementation; future coupled transmissions require an explicit mapping and tests, not an undocumented special case.

The proposed canonical core is right-handed SI: metres, radians, seconds, newton-metres and amperes, with quaternion order xyzw. Unity is an adapter, not the unit authority. Its actual scene scale, basis and rotation convention must be audited. Convert poses, axes, angular velocities and handedness consistently; a simple quaternion-component swap is not a general conversion. Public tests use synthetic dimensions.

Kinematic residual, measured joint accuracy, repeatability, loaded TCP accuracy and compliance are separate quantities. Calibration is an experiment with reference equipment, uncertainty and withheld validation poses, not merely fitting the same samples used to identify offsets.

## Model requirements

### MOD-001: Explicit SI units
Phase: R1; Issue: #10; Origin: derived
Requirement: Every model, command and observation field shall declare units and coordinate side. Public interfaces shall use canonical SI values; display degrees and motor revolutions require explicit conversion.
Acceptance: Unit round trips and deliberately mismatched degree/radian, millimetre/metre and motor/joint fixtures are tested; no implicit unit conversion occurs inside a driver-independent task.

### MOD-002: Frame convention and host conversion
Phase: R1; Issue: #10; Origin: derived
Requirement: The model shall specify transform direction, handedness, joint-axis sign, quaternion order and base/tool frames. Motion Studio conversion shall be versioned and independently tested.
Acceptance: Golden translated/rotated frames, basis vectors and mirrored-coordinate fixtures reproduce the same physical pose; conversions pass inverse and composition checks.

### MOD-003: Model validation
Phase: R1; Issue: #10; Origin: derived
Requirement: Model loading shall reject nonfinite values, invalid rotations, zero-length axes, cyclic or disconnected chains, duplicate joint identifiers and inconsistent joint counts. Normalization is allowed only within a declared input tolerance.
Acceptance: Invalid fixtures fail before admission; near-unit and grossly invalid quaternions exercise distinct paths. Model mutation never alters an active runtime in place.

### MOD-004: Transmission mapping
Phase: R1; Issue: #10; Origin: derived
Requirement: Motor-to-joint mapping shall declare signed ratio, offsets and coupling assumptions; conversions shall reject overflow. Gearing shall not be embedded in task trajectories.
Acceptance: Inverse mappings, sign changes, extreme inputs and synthetic coupled-model rejection are covered; changes invalidate reference/calibration state.

### MOD-005: Joint topology and limits
Phase: R1; Issue: #3; Origin: derived
Requirement: Bounded and continuous joints shall be distinct. Allowed winding/turn counts, cable constraints and forbidden regions shall not be erased by wrapping every angle into a shortest-arc interval.
Acceptance: Wrap-boundary paths, multi-turn references and finite-travel joints choose legal configurations or reject the request; shortest-angle interpolation cannot bypass a mechanical limit.

### MOD-006: Tool and payload identity
Phase: R1; Issue: #10; Origin: derived
Requirement: Tool pose, collision shape, mass/centre-of-mass assumptions and supported IO shall be identified separately from the arm. Payload/limit changes shall invalidate affected approvals.
Acceptance: Swap a synthetic tool or carried part and verify updated TCP, collision envelope and qualification identity before motion is admitted.

### MOD-007: Immutable active configuration
Phase: R1; Issue: #11; Origin: derived
Requirement: An active configuration shall have a schema version and content fingerprint. Updates shall validate a complete candidate, activate only in an allowed non-moving state and preserve recoverability.
Acceptance: Interrupted/corrupted configuration writes never expose a mixed configuration; commands bound to the previous fingerprint are rejected after activation.

### MOD-008: State provenance
Phase: R1; Issue: #6; Origin: derived
Requirement: Commanded, issued-step-estimated, motor-measured and output-measured positions shall remain distinct, with reference provenance and validity for each. No absent measurement shall be filled with a command.
Acceptance: Open-loop fixtures report estimates only; sensor loss invalidates its channel without inventing a replacement measurement.

### MOD-009: Resources are capabilities
Phase: R1; Issue: #5; Origin: derived
Requirement: Axis count, tools, sensors, motion modes, timestamp support and firmware features shall be discoverable. Profiles shall not advertise a capability merely because an interface exists.
Acceptance: Mixed-generation discovery returns only actual qualified capabilities and rejects tasks whose required capabilities are unavailable.

### MOD-010: Configuration dependency graph
Phase: R1; Issue: #10; Origin: derived
Requirement: The model shall identify which references, limits, task resources and preflight approvals depend on each calibration or hardware parameter.
Acceptance: Automated mutation tests show the expected invalidation set for sign, ratio, offset, tool, limit and firmware-format changes without unnecessarily invalidating unrelated read-only metadata.

## Calibration requirements

### CAL-001: Constrained homing authorization
Phase: R1; Issue: #3; Origin: derived
Requirement: Homing shall be a separate explicitly authorized motion mode able to establish an unknown reference without bypassing travel, speed, timeout, switch-plausibility and interlock constraints.
Acceptance: Test already-active, stuck, bouncing, missing and reversed switches, bounded backoff/reapproach and timeout; homing failure never declares a reference valid.

### CAL-002: Reference invalidation
Phase: R1; Issue: #10; Origin: derived
Requirement: Reset, power loss, lost motion, manual movement, changed mapping or untrusted sensor state shall invalidate affected references unless the selected sensing system can independently re-establish them.
Acceptance: Each event is injected and subsequent motion is refused until the relevant reference procedure succeeds; restarting a process cannot restore validity from stale RAM.

### CAL-003: Motor absolute-angle ambiguity
Phase: R2; Issue: #6; Origin: derived
Requirement: Single-turn motor angle behind a reduction shall not be treated as absolute joint angle over multiple motor turns. Turn tracking and startup ambiguity resolution shall be explicitly specified.
Acceptance: Distinct legal joint poses yielding identical motor single-turn readings are tested; ambiguous startup remains unreferenced.

### CAL-004: Calibration data and uncertainty
Phase: R1; Issue: #10; Origin: derived
Requirement: Calibration records shall identify equipment, procedure, configuration, timestamp, repeat observations, fitted parameters and uncertainty. Numerical fit residual shall not stand in for independent accuracy evidence.
Acceptance: Validate on withheld poses and both approach directions, with relevant loads and temperatures; report repeatability and systematic error separately using a declared test method.

### CAL-005: Controlled calibration movement
Phase: R2; Issue: #6; Origin: derived
Requirement: Encoder/servo calibration procedures shall declare required freedom of movement, current/velocity limits, support for gravity loads and abort behavior. Vendor automatic startup calibration shall not be enabled blindly.
Acceptance: A procedure requiring unloaded rotor motion is rejected in an incompatible assembled-joint configuration; abort preserves or explicitly invalidates references.

### CAL-006: Restore and migration
Phase: R1; Issue: #11; Origin: derived
Requirement: Calibration backup/restore shall be versioned and configuration-bound. Firmware or hardware migrations shall reject incompatible records and require review before applying transformed data.
Acceptance: Wrong-board, wrong-encoder, partial, corrupt and old-schema records fail predictably; successful restore still requires appropriate physical reference checks.
