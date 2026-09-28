# Runtime lifecycle, fault reactions, gravity support and power

Sources: [S10], [S13], [S18], [S19]. Status: specification-only. Issue #9 owns this cross-cutting work. This is not a safety-rated controller design or a claim of standards compliance. ISO's public 2025 scope/edition information is a standards lead, not access to its complete normative requirements.

## Lifecycle and physical reaction

Keep command permission distinct from physical motion and support. Proposed runtime concepts include unconfigured, unreferenced, constrained homing, ready, armed, executing, stopping, holding, fault-latched and recovery. Holding may require energized motors; removing permission does not imply removing torque. A fault may revoke further normal commands while a separate qualified stop/support path remains active.

A reaction matrix is configuration-specific. Do not implement a universal `fault -> disable all` shortcut, a universal `fault -> keep driving` shortcut, or a reset loop that repeatedly re-enables the arm.

| Trigger | Normal permission | Intended physical policy, subject to qualification | Recovery |
|---|---|---|---|
| Host lease expiry with healthy actuation | Revoke new normal motion | Start the approved deceleration/support path before horizon exhaustion | Fresh session, local acknowledgement and reference checks |
| Driver loss or severe electrical fault | Revoke coordinated motion | Available-axis response plus independently engineered gravity/tool support; do not assume the failed drive can brake | Fault investigation and affected references re-established |
| Early thermal warning | Limit new work | Constraint-preserving derating or early controlled stop while support remains available | Hysteresis and verified cooling/limits |
| Severe thermal trip or power loss | Revoke | Qualified electrical/mechanical fallback, not a fictional software stop | Power/thermal inspection and reference recovery |
| Invalid required sensor/interlock | Revoke | Qualified stop/hold or fallback appropriate to remaining actuation | Clear cause and revalidate live inputs |
| Emergency-stop demand | No dependence on agent/host | Independently engineered cell reaction including stored energy and tools | Deliberate local reset; no automatic restart |

No generic timeout, stop distance, brake rating or safety integrity level is guessed. P-HEARTBEAT, P-STOP, P-HORIZON and P-THERMAL are commissioning parameters with release gates.

## Lifecycle and fault requirements

### SAFE-001: Permission is not a physical stop
Phase: R1; Issue: #9; Origin: derived
Requirement: Runtime state shall report motion/support state separately from command permission. A completed software transition shall not claim physical rest or power isolation without suitable evidence.
Acceptance: A fixture with continuing motion after permission revocation remains stopping/unknown rather than stopped; measured stop evidence is recorded separately.

### SAFE-002: Live arming conditions
Phase: R1; Issue: #9; Origin: derived
Requirement: Arming and continued execution shall depend on current configuration, references, required feedback, interlocks, drive state and explicit authorization. A past successful boolean snapshot shall not permanently authorize motion.
Acceptance: Change each condition immediately before and after arming and during execution; the appropriate prevention/reaction occurs within the qualified latency.

### SAFE-003: Explicit modes and ownership
Phase: R1; Issue: #5; Origin: derived
Requirement: Manual jog, homing, task execution, calibration and recovery shall have distinct permitted actions and a single command owner. Mode transitions shall not combine competing command streams.
Acceptance: Concurrent owners and illegal transitions are rejected; switching modes flushes or invalidates incompatible commands without unexpected movement.

### SAFE-004: Latched fault evidence
Phase: R1; Issue: #11; Origin: derived
Requirement: Faults shall retain first cause, subsequent relevant events, controller time, configuration identity and available state. Clearing the displayed fault shall not itself re-arm the controller.
Acceptance: Cascading faults preserve causal ordering; acknowledgement leads to a non-running state and requires fresh authorization for motion.

### SAFE-005: Qualified reaction time and envelope
Phase: R1; Issue: #9; Origin: derived
Requirement: Fault reactions shall account for detection, scheduling/blocking, transport, drive and mechanical response. P-STOP shall be qualified for actual payload, gravity orientation and remaining actuation.
Acceptance: Instrument each contribution and worst-case integrated stopping/support behavior; no percentile timing alone is presented as a hard bound.

### SAFE-006: Reset and reconnect cannot resume
Phase: R1; Issue: #9; Origin: derived
Requirement: Boot, watchdog reset, brownout, firmware update, link reconnect and node discovery shall not automatically arm or replay previous motion. Reference validity shall be reconsidered after each event.
Acceptance: Inject every event at multiple trajectory/task stages and verify disarmed recovery, stale-queue rejection and explicit reference handling.

### SAFE-007: Independent emergency response
Phase: R1; Issue: #9; Origin: derived
Requirement: Cell emergency stopping and gravity/tool retention shall not rely solely on Astra, ordinary USB/CAN delivery or the application firmware. Driver disable and a software stop command shall not be labelled certified STO.
Acceptance: A reviewed external safety/integration design and physical test records establish the actual reaction to processor, communication and power failures.

### SAFE-008: Deliberate recovery
Phase: R1; Issue: #9; Origin: derived
Requirement: Recovery shall require cause resolution, live condition checks, affected reference validation and explicit authorization. Blind catch-up to the old planned pose shall not be used as fault recovery.
Acceptance: Obstruction, encoder mismatch and lost-reference scenarios cannot resume by acknowledgement alone; recovery movement uses its own constrained approved procedure.

### SAFE-009: Guarded commissioning
Phase: R1; Issue: #9; Origin: derived
Requirement: New powered functions shall progress through reviewed low-energy fixtures, supported/unloaded mechanisms and bounded test modes before operation under intended load. Test tooling shall not silently bypass runtime limits.
Acceptance: Commissioning records identify constraints, personnel authorization and external support; sequential joint qualification is preserved before expanding the active system.

### SAFE-010: Capability-scoped hazard review
Phase: R1; Issue: #9; Origin: derived
Requirement: Each new tool, feedback mode, motor type or autonomous workflow shall update the hazard/reaction analysis. Successful simulation or encoder installation shall not be treated as automatic collaborative-robot safety qualification.
Acceptance: Review changed hazards, affected approvals and required physical tests before the capability becomes available in a release profile.

## Electrical, thermal and mechanical support requirements

### PWR-001: Component-specific electrical limits
Phase: R1; Issue: #2; Origin: derived
Requirement: Supply, driver, motor, conductor, connector and slip-ring limits shall be independently identified, with protective devices and grounding/reference strategy appropriate to the configuration.
Acceptance: The lowest applicable component limit governs; voltage/current and fault measurements substantiate the profile rather than relying on board marketing maxima.

### PWR-002: Thermal sensing and plausibility
Phase: R1; Issue: #9; Origin: derived
Requirement: Thermal monitoring shall cover relevant drivers, motors and structural/material limitations, with calibrated sensing and open/short/stale-sensor detection. A driver junction estimate shall not stand in for every mechanical temperature.
Acceptance: Controlled heating and sensor-fault tests verify detection, warning/trip hysteresis and conservative behavior with unavailable temperature data.

### PWR-003: Derating compatible with support
Phase: R1; Issue: #9; Origin: derived
Requirement: Current/thermal derating shall be coordinated with acceleration, holding demand and gravity support. The system shall stop or transfer support before reducing torque below what its qualified envelope requires.
Acceptance: Worst-case gravity/load/temperature fixtures demonstrate a bounded transition; thermal protection does not silently cause an unsupported falling joint.

### PWR-004: Regenerative-energy handling
Phase: R5; Issue: #8; Origin: derived
Requirement: Brushless-drive integration shall qualify deceleration/regeneration against supply absorption, bus voltage, braking components and protective limits. A supply that sources current shall not be assumed to absorb returned energy.
Acceptance: Measure bus behavior during maximum qualified deceleration and load-driven motion, including loss of supply absorption and brake component faults.

### PWR-005: Brake and tool power sequencing
Phase: R1; Issue: #9; Origin: derived
Requirement: Any brake, latch, vacuum or gripping system needed for retention shall have defined power-up/down, feedback, timing and failure behavior. Power removal shall not be assumed to retain a part.
Acceptance: Test transitions and faults with representative loads using external support; retention claims identify the actual mechanism and evidence.

### PWR-006: Configuration requalification
Phase: R5; Issue: #8; Origin: derived
Requirement: Higher-power motors or changed transmissions shall trigger review of supply, thermal, structural, brake, wire/slip-ring and motion limits. High power density shall not be used as evidence of accuracy.
Acceptance: Release records separate continuous/peak thermal capability, dynamic response and measured positioning results for the new configuration.
