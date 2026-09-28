# Feedback, loop ownership and interchangeable drive backends

Sources: [S10], [S11], [S12], [S13], [S15]. Status: specification-only. Issues #6 and #8. No feedback hardware or corrective controller is implemented by the current contracts.

## Selected architecture

Acquire motor encoders in observation-only mode first. Establish scale, direction, reference, delay and noise before permitting feedback to change commands. Current TMC2209 systems remain step-commanded power stages; attaching an encoder does not automatically provide motor-current field-oriented control or joint torque measurement.

Maintain separate motor and output channels. A fast drive-local motor loop and a slower output-position correction loop are candidates, not universal gains or fixed bandwidth ratios. Output-side velocity feedback across compliant/backlash transmissions can destabilize control; the selected topology must be justified through measured dynamics [S13]. Align sample timestamps before calculating transmission discrepancy. This discrepancy is not automatically torque and output angle is not global TCP position.

A backend advertises actual modes, state, sensors, timing and limits. The same robot-facing contract may be implemented by local STEP/DIR or a remote servo, but electrical capabilities, units, tuning and fault reactions differ. Servo commutation/current loops stay in appropriate local drives [S11]. CiA 402 supplies useful state/mode vocabulary [S15]; copying vocabulary is not a compliance claim.

## Feedback requirements

### FB-001: Timestamped observation identity
Phase: R2; Issue: #6; Origin: derived
Requirement: Measurements shall identify sensor, coordinate side, units, sample time, receipt time where relevant, validity, reference and timestamp uncertainty. Remote samples shall be mapped to controller time explicitly.
Acceptance: Delayed/reordered remote samples cannot masquerade as fresh measurements; tests distinguish sample age from packet arrival age.

### FB-002: Scale, wrap and reference
Phase: R2; Issue: #6; Origin: derived
Requirement: Encoder configuration shall distinguish pulses, decoded counts, revolutions, direction and single/multi-turn reference. Unwrapping shall enforce a motion/sampling ambiguity bound and handle overflow.
Acceptance: PPR-versus-quadrature-count, sign, wrap, skipped-sample and maximum-speed fixtures produce correct values or explicit invalidity, never silent aliasing.

### FB-003: Plausibility and stale-data detection
Phase: R2; Issue: #6; Origin: derived
Requirement: Required feedback shall enforce P-FEEDBACK freshness, plausible transitions and interface error checks. Frozen-sensor diagnosis shall consider commanded/physical conditions rather than flag legitimate stationary readings as failure.
Acceptance: Disconnect, corrupt, freeze, reverse and delay sensors across stationary and moving cases; the detector's limitations and false-positive behavior are characterized.

### FB-004: Observation before correction
Phase: R2; Issue: #6; Origin: derived
Requirement: Motor-feedback bring-up shall first log command/estimate/measurement relationships without modifying actuation, then permit correction only under a separately qualified configuration.
Acceptance: Observation mode is instrumented to prove unchanged output; correction cannot be enabled until scale/reference/delay/following-error tests pass.

### FB-005: Bounded correction and anti-windup
Phase: R2; Issue: #6; Origin: derived
Requirement: Corrective control shall respect position, speed, acceleration, current and following-error limits, with saturation handling and bounded integral state where applicable. Large error shall fault rather than command blind catch-up.
Acceptance: Saturation, obstruction and prolonged error tests demonstrate bounded correction and recovery without integrator-driven jumps.

### FB-006: Bumpless mode transition
Phase: R2; Issue: #6; Origin: derived
Requirement: Open-loop, observation and closed-loop transitions shall initialize controller state from coherent valid observations and obey the configured motion/support policy.
Acceptance: Transitions at representative positions/loads produce no unapproved discontinuity; missing references or invalid feedback prevent transition.

### FB-007: Independent output feedback
Phase: R4; Issue: #6; Origin: derived
Requirement: Adding a joint-output encoder shall retain the motor observation as a distinct channel. Output feedback shall be calibrated to the physical link and not silently substituted for motor commutation feedback.
Acceptance: Simulated transmission slip/compliance changes the appropriate discrepancy and reported joint state while preserving both original measurements.

### FB-008: Qualified dual-loop stability
Phase: R4; Issue: #6; Origin: derived
Requirement: Dual-loop topology, bandwidth, filtering and gains shall be selected from measured transmission dynamics, sensor latency and load response, with one owner per loop.
Acceptance: Frequency/step-response and disturbance tests across the qualified load/temperature envelope establish stability and performance margins; no generic gain set is released for all arms.

### FB-009: Honest force and accuracy claims
Phase: R4; Issue: #6; Origin: derived
Requirement: Encoder difference, commanded current and measured motor current shall not be labelled measured joint torque without a validated conversion/model and uncertainty. Output sensing shall not imply measured TCP accuracy.
Acceptance: Telemetry and API tests preserve source/estimate labels; independent metrology validates any advertised torque/position performance.

### FB-010: Feedback fault propagation
Phase: R2; Issue: #9; Origin: derived
Requirement: Loss of required feedback shall revoke incompatible control modes and invoke the qualified reaction. Falling back to open-loop motion shall require an explicit separately approved policy, not an automatic convenience fallback.
Acceptance: Sensor failure in each operating mode produces the specified local/system response and cannot silently retain a closed-loop capability flag.

## Drive requirements

### DRV-001: Capability-negotiated command modes
Phase: R1; Issue: #3; Origin: derived
Requirement: Each backend shall advertise implemented/qualified position, velocity, torque, feedforward, timestamp, sensor and stop capabilities. Unsupported modes shall be rejected before side effects.
Acceptance: Position-only fixtures reject torque/velocity modes and missing timestamp support; discovered capabilities match the release configuration.

### DRV-002: Single control-loop owner
Phase: R3; Issue: #7; Origin: derived
Requirement: Every control loop shall have a documented owner and setpoint interpretation. Central and drive-local position controllers shall not independently fight over the same state without a designed cascade.
Acceptance: Review and test mode handshakes and step/disturbance responses; changing a drive mode invalidates incompatible controller state.

### DRV-003: Explicit units and conversions
Phase: R3; Issue: #7; Origin: derived
Requirement: Adapters shall convert robot-side units to vendor units explicitly, including gear/sign/current/torque conventions and saturations. Estimated conversions shall remain labelled estimates.
Acceptance: Reference packets and synthetic ratios verify bidirectional conversions, boundary saturation and nonfinite rejection.

### DRV-004: Drive lifecycle and stop evidence
Phase: R3; Issue: #7; Origin: derived
Requirement: Enable, ready, executing, stopping, stopped, fault and unavailable states shall be observable. A submitted stop request shall not be treated as confirmed physical stop without the required evidence.
Acceptance: Lost acknowledgements, late feedback and drive faults preserve uncertainty and system reaction; the controller never assumes a failed drive obeyed a command.

### DRV-005: Mixed-generation coordination
Phase: R3; Issue: #7; Origin: derived
Requirement: The coordinator shall support qualified mixtures of local stepper and networked drives under one trajectory and fault policy without exposing backend differences to task definitions.
Acceptance: A mixed fixture meets P-SKEW and feedback-age limits and reacts correctly to one-axis failure before full-arm rollout.

### DRV-006: Local brushless motor loops
Phase: R5; Issue: #8; Origin: derived
Requirement: Brushless commutation, current measurement/regulation and immediate drive electrical protection shall run in an appropriate local servo drive. Existing TMC2209 channels shall not be treated as three-phase inverters.
Acceptance: Integration documentation and tests show no per-PWM-cycle dependency on central bus messages and qualify the selected drive's electrical behavior.

### DRV-007: Backend conformance and persistence
Phase: R3; Issue: #7; Origin: derived
Requirement: A backend shall pass the shared command/feedback/fault conformance suite and identify its firmware/protocol/configuration. Drive reboot shall invalidate stale state and require fresh qualification checks before resumption.
Acceptance: The same synthetic contract tests run against local/remote adapters; reboot, wrong protocol version and changed gains/limits cannot silently inherit active approvals.
