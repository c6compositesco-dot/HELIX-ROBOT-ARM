# Trajectories, stopping horizons and local STEP/DIR

Sources: [S07], [S08], [S09], [S10]. Status: specification-only. Issue #3. Current `WaypointQueue` checks points; it does not implement the requirements below.

## Selected motion architecture

The first release executes host-prepared, validated joint trajectories against controller time. Prefer a bounded piecewise-polynomial representation carrying position, velocity and acceleration boundary state, with an explicit interpolation version. A useful candidate is piecewise constant jerk: q(t)=q0+v0*t+a0*t^2/2+j*t^3/6. Check interior extrema by derivative roots as well as boundaries; quintics require the corresponding higher-order root checks or a proven conservative bound. Finite sampling alone is not a proof of continuous limits.

Evaluate Ruckig's jerk-limited state-to-state method as a host reference and possible isolated numerical component [S07, S08], not as a replacement robot controller. Current upstream CMake identifies 0.19.6, requires C++20 and enables a cloud client by default. Community intermediate-waypoint calculation uses that cloud service. The existing core is C++17: dependency/version/toolchain choice needs its own review. No cloud path is permitted in the firmware, and no desktop timing claim becomes an MCU deadline guarantee. Do not assume intermediate-waypoint timing preserves a previously approved geometric path [S09].

Pulse generation should use a qualified hardware-timed event engine with integer/fractional accumulation, preserving residual steps across segments. A timer-compare ISR or DMA implementation must fit actual GPIO routing and measured load; not every STEP pin is necessarily a convenient timer output. The execution engine owns pulse timing; a separate trajectory worker prepares a bounded future horizon. Maintain sufficient validated stopping data before the normal queue becomes empty.

## Motion requirements

### MOT-001: Defined continuous trajectory semantics
Phase: R1; Issue: #3; Origin: derived
Requirement: Every trajectory shall declare timebase, interpolation format, joint ordering, start state and position/velocity/acceleration limits, with jerk limits where supported. Unknown formats shall be rejected.
Acceptance: Host and controller evaluate identical synthetic segments at endpoints and interior timestamps within P-NUMERIC tolerances; mismatched versions fail admission.

### MOT-002: Whole-segment limit validation
Phase: R1; Issue: #3; Origin: derived
Requirement: Validation shall cover interior position, velocity, acceleration and jerk extrema, not only waypoints. Numerical root uncertainty shall produce conservative bounds or rejection.
Acceptance: Adversarial segments with legal endpoints and illegal interior extrema are rejected; analytical and independent dense-reference checks agree without treating sampling as proof.

### MOT-003: Coordinated joint execution
Phase: R1; Issue: #3; Origin: derived
Requirement: All enabled joints shall execute against one trajectory timebase with bounded skew. Per-axis speed limits shall produce coordinated retiming rather than silent desynchronization.
Acceptance: Measure multi-axis event timing and final phase alignment under maximum intended concurrency; any exceeded P-SKEW budget is detected.

### MOT-004: Start-state agreement and continuity
Phase: R1; Issue: #3; Origin: derived
Requirement: A trajectory shall match the trusted current/planned state within configured tolerances and preserve required boundary continuity. Open-loop estimates shall remain labelled estimates.
Acceptance: Wrong-start, nonfinite, position-jump and derivative-discontinuity fixtures are rejected; measured feedback disagreement follows the configured fault policy rather than snapping to a path.

### MOT-005: Validated stopping horizon
Phase: R1; Issue: #9; Origin: derived
Requirement: Execution shall retain sufficient data and resources to initiate the qualified stop before usable trajectory data expires. P-HORIZON shall account for detection, computation, actuation and mechanical stopping needs.
Acceptance: Inject progressively earlier/later producer stalls and verify that stop initiation occurs within the validated envelope, never after the last usable sample has already been consumed.

### MOT-006: No unsafe extrapolation or catch-up
Phase: R1; Issue: #3; Origin: derived
Requirement: Queue starvation, missed deadlines and late segments shall not cause unrestricted extrapolation, instantaneous position correction or pulse bursts intended merely to catch up with wall time.
Acceptance: Timestamp gaps, expired segments and intentional runtime stalls exercise documented rejection/stop paths without violating pulse or motion constraints.

### MOT-007: Controlled speed changes
Phase: R1; Issue: #3; Origin: derived
Requirement: Speed override, pause, resume and jog changes shall use constraint-preserving transition trajectories. Instant multiplication of velocity shall not bypass acceleration or jerk constraints.
Acceptance: Step changes of requested speed and repeated pause/resume actions remain within the configured dynamic envelope or are refused.

### MOT-008: Path approval after transformation
Phase: R1; Issue: #10; Origin: derived
Requirement: Retiming, smoothing, IK branch changes and pulse quantization shall be included in the executed-path validation model. Any geometric change outside approved tolerance shall require renewed preflight.
Acceptance: Construct a collision-free waypoint set whose interpolated/smoothed path collides; admission rejects it unless the actual transformed path passes validation.

### MOT-009: Reachable stop policy
Phase: R1; Issue: #9; Origin: derived
Requirement: Each allowed operating envelope shall have a qualified stopping strategy accounting for available actuation, gravity, payload, limits and relevant collision clearance. An unavailable drive shall not be assumed capable of commanded deceleration.
Acceptance: Test boundary cases in simulation and reviewed physical fixtures; impossible controlled-stop conditions select the independently engineered fallback rather than report a successful stop.

### MOT-010: Deterministic buffering and backpressure
Phase: R1; Issue: #5; Origin: derived
Requirement: Queue capacity, minimum usable horizon and admission reservations shall be bounded and observable. Full queues shall reject or defer commands without losing sequence/acknowledgement consistency.
Acceptance: Fill, drain, wrap and interrupt producer/consumer operations; no partial segment or partially admitted multi-axis command is executed.

### MOT-011: Local numerical generation is optional
Phase: R1; Issue: #3; Origin: derived
Requirement: Host-generated trajectory playback shall not depend on an unqualified onboard generator. Any later local generator shall meet the same format, continuous-limit, stop and timing requirements.
Acceptance: Execute the same reference motion from offline data and a candidate generator; compare actual path interpretation, failure behavior and resource bounds before enabling the candidate.

## STEP/DIR and driver requirements

### STEP-001: Derived event-rate budget
Phase: R1; Issue: #3; Origin: derived
Requirement: Maximum pulse demand shall be derived from motor full steps, input microsteps, transmission mapping and output speed for every enabled axis. Internal driver interpolation shall not be counted as MCU-generated input pulses.
Acceptance: Calculate and measure worst-case simultaneous demand with verified configuration; refuse profiles exceeding the qualified event budget.

### STEP-002: Electrical timing compliance
Phase: R1; Issue: #3; Origin: derived
Requirement: STEP width, DIR setup/hold, enable timing and polarity shall meet the actual driver/module requirements at the selected clock and event load.
Acceptance: Logic-analyser measurements include direction reversals, shortest intervals, coincident axis events, startup, reset and overload; no GPIO assumptions come from an unrelated board revision.

### STEP-003: Quantization and residual preservation
Phase: R1; Issue: #3; Origin: derived
Requirement: The step accumulator shall bound quantization error, preserve fractional residuals across trajectory segments and handle signed direction changes without cumulative rounding drift or integer overflow.
Acceptance: Long sequences of sub-step moves, reversals and varied segment boundaries agree with an independent reference within the declared quantization bound.

### STEP-004: Verified driver configuration
Phase: R1; Issue: #2; Origin: derived
Requirement: Current, microstepping and driver operating mode shall be explicitly configured and read back where supported. Power/voltage limits shall follow fitted components, not the Octopus maximum input rating alone.
Acceptance: Corrupt or unavailable driver configuration is detected before arming; mode/current changes during operation follow a reviewed transition and thermal qualification.

### STEP-005: Honest issued-step accounting
Phase: R1; Issue: #6; Origin: derived
Requirement: Position estimates shall advance from pulses actually issued by the event engine, not merely queued requests. Stall/load diagnostics shall not be reported as independent joint-position feedback.
Acceptance: Cancelled/aborted/underrun sequences report their issued-step history correctly and invalidate trust when physical tracking is unknown.

### STEP-006: Driver faults and holding behavior
Phase: R1; Issue: #9; Origin: derived
Requirement: Driver fault, thermal warning, power loss and enable/disable transitions shall integrate with the robot's gravity-support and tool policy. Disabling a driver is not universally the safe response.
Acceptance: Low-energy fault injection establishes actual holding/release behavior and the external support response; software state alone is insufficient evidence.
