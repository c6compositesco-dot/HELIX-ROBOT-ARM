# Helix firmware requirements baseline

Revision 0.2 / 2026-09-28. **Specification, not implemented behavior or hardware approval.** This baseline expands draft PR #1. The original foundation remains host-only and the board-image build stays blocked.

## Product intent and authority

Build our own reusable robot-control system. Retain the Octopus Pro as the central controller through open-loop steppers, motor encoders, networked drives, output encoders and external brushless servos. Astra/Fleet supervises tasks; deterministic local execution and fault reaction do not depend on AI. Simulate and validate the motion that will actually execute.

`Origin: user` identifies an explicit product decision from the project discussion. `Origin: derived` identifies an engineering requirement proposed to realize it, not a claim that the user selected a particular algorithm. Every Requirement is normative for the stated phase/capability. Every Acceptance is work still to perform unless a separately reviewed evidence record establishes otherwise. A source demonstrates a method or interface, not that Helix meets it. There is no certification claim.

## Section map

| Specification | Responsibility | Implementation issues |
|---|---|---|
| [Model and calibration](01_model_calibration.md) | Units, frames, geometry, transmission, references and calibration | #10 |
| [Board and real time](02_board_realtime.md) | Octopus BSP, clocks, memory, peripherals, scheduling | #2, #3 |
| [Kinematics](03_kinematics.md) | FK, Jacobian, bounded full-pose IK and numerical qualification | #4 |
| [Motion and STEP/DIR](04_motion_stepdir.md) | Trajectory validation, timing, stepping and driver setup | #3 |
| [Runtime, faults and power](05_runtime_safety_power.md) | Lifecycle, stopping, gravity support, thermal/electrical behavior | #9 |
| [Feedback and drives](06_feedback_drives.md) | Motor/output sensing, loop ownership and interchangeable actuators | #6, #8 |
| [Host protocol and network](07_protocol_network.md) | USB framing, transactional admission, CAN and synchronization | #5, #7 |
| [Tasks, tools and simulation](08_tasks_tools_sim.md) | Bounded task execution, physical evidence and sim-to-real binding | #5, #10 |
| [Release and observability](09_release_observability.md) | Evidence, telemetry, tests, updates and trust boundaries | #11 |

[Methods and alternatives](METHODS.md) records why techniques were selected or deferred. [Sources](sources.json) distinguishes papers, deployed-system documentation and vendor specifications. [Parameters](parameters.json) records unresolved measurements and proposed timing targets. [Integration gaps](INTEGRATION_GAPS.md) maps the existing code to missing behavior. [Qualification plan](QUALIFICATION.md) defines release gates and test campaigns.

## Staging: future features must not block the first useful controller

| Phase | Deliverable |
|---|---|
| R0 | Requirements, host fixtures, reproducible software checks; no hardware image |
| R1 | Qualified open-loop execution, references/homing, host protocol, bounded tasks and diagnostics |
| R1-IK | Optional onboard Cartesian/FK/IK feature, enabled only after its numerical and loaded-MCU gates |
| R2 | Motor-encoder observation, then separately qualified bounded correction |
| R3 | One networked drive, then coordinated mixed local/networked operation |
| R4 | Independent joint-output feedback and qualified dual-loop behavior |
| R5 | Qualified external brushless drives using the same joint/task contracts |

Requirements introduced in a phase continue to apply to later compatible configurations. Optional advertised capabilities must meet their own requirements; `unsupported` is acceptable, fabricated capability is not. Host-planned R1 operation does not depend on R1-IK. Development can proceed in parallel; powered deployment cannot skip its hardware gates.

## Performance policy

Proposed study points, not measurements: 1 kHz central trajectory evaluation; 100 Hz onboard IK with a 2 ms elapsed solve budget and up to 10 iterations; investigate 250-500 Hz drive exchange. Pulse rates are derived from the actual motors, ratios, microstep settings and motion envelope. Do not translate desktop-library benchmarks into MCU guarantees. Freeze numerical tolerances, watchdogs, stopping margins and feedback-age limits only through the parameter register and evidence. No arbitrary global 200 ms timeout or factory joint gains are supplied.

## System requirements

### SYS-001: Own robot-control core
Phase: R1; Issue: #2; Origin: user
Requirement: Our portable control core shall own robot-coordinate semantics, coordinated execution and system fault policy. Third-party HALs or qualified numerical components shall not replace the product's control architecture.
Acceptance: Review dependency boundaries and run the same joint/task contract against independent simulated and physical-adapter fixtures without vendor registers in task code.

### SYS-002: Retain the Octopus coordinator
Phase: R1; Issue: #2; Origin: user
Requirement: The architecture shall retain the Octopus Pro across all planned actuator generations, adding qualified interfaces and external drives where necessary. Replacement is a separately justified decision based on a measured unmet requirement.
Acceptance: Review each R2-R5 design for central-board retention and record any unmet resource or interface requirement rather than assuming an upgrade requires replacement.

### SYS-003: Hardware-independent task meaning
Phase: R1; Issue: #5; Origin: user
Requirement: Tasks shall request robot-side actions and capabilities rather than motor pulses, encoder counts or brand-specific registers. Six rotary joints are the first supported profile, not a claim of arbitrary-machine compatibility.
Acceptance: Execute identical synthetic task definitions against open-loop and network-drive mocks; reject unsupported capabilities before execution.

### SYS-004: Bounded AI authority
Phase: R1; Issue: #5; Origin: user
Requirement: Astra/Fleet may select tasks and propose variable actions, but shall not bypass local admission, limits, execution timing, operator permissions or fault policy. No AI response is required to initiate local fault reaction.
Acceptance: Disconnect or delay the supervisor and attempt invalid proposals; local behavior remains within the qualified policy and no proposal writes directly to actuator outputs.

### SYS-005: Local execution without a cloud dependency
Phase: R1; Issue: #5; Origin: derived
Requirement: Approved deterministic tasks shall not require internet services to execute. Loss of the local supervisor shall follow the selected qualified mode; unattended continuation is not the default and requires all dependencies to be local.
Acceptance: Block internet access and verify normal local operation; separately remove the host link and verify the documented stop/continuation policy for that mode.

### SYS-006: Preserve working hardware and private configuration
Phase: R0; Issue: #2; Origin: derived
Requirement: Development shall preserve the existing root binary and recovery information, and keep private geometry/calibration outside public fixtures. New images shall not be flashed automatically by CI or repository tooling.
Acceptance: Compare root-file hashes against the foundation parent; inspect fixture data and CI permissions; require a reviewed recovery route before powered deployment.

### SYS-007: Honest capability and completion reporting
Phase: R1; Issue: #5; Origin: derived
Requirement: The system shall distinguish specified, implemented, software-tested and hardware-qualified features, and distinguish command completion from measured physical success.
Acceptance: An unavailable encoder, solver or drive never returns operational success; status responses expose supported modes and the evidence basis for completion.

### SYS-008: Configuration-scoped qualification
Phase: R1; Issue: #10; Origin: derived
Requirement: Qualification shall identify board, firmware, geometry, transmissions, tools, payload assumptions, limits and feedback configuration. A material change shall invalidate affected approvals rather than inherit them silently.
Acceptance: Mutate each configuration category and verify that incompatible motion/task approvals are rejected until revalidated.

## Machine-readable extraction

Run `python3 firmware/tools/check_requirements.py` to validate structure and references, or add `--export /tmp/helix-requirements.json` to obtain the traceability registry. This checks documentation completeness only. Exported requirements remain `specified`; no checker may mark physical or software behavior verified automatically.
