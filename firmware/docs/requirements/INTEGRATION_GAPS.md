# Existing foundation versus required behavior

Inspected baseline: e1bad2563d84d7434b9e87e39283f2e9714a71ec. This requirements change does not claim these gaps are fixed.

| Existing file/section | Present behavior | Required next change and owning issue |
|---|---|---|
| `core/types.hpp` | Six-axis double vector, position/velocity point limits | Add validated configuration and acceleration/jerk/topology semantics; no guessed hardware values. #3/#10 |
| `joints/transmission.hpp` | Independent signed fixed-ratio mapping | Preserve mapping contract, add calibration provenance and explicitly reject unsupported coupling. #10 |
| `feedback/sample.hpp` | Separate source/validity/reference/age fields | Add sensor identity, sample/receipt time uncertainty, acquisition, plausibility and qualified loop integration. #6 |
| `motion/waypoint_queue.hpp` | Single-owner point queue | Continuous segment format, interior extrema, continuity, stopping horizon and reviewed concurrency exchange. #3 |
| `runtime/execution_gate.hpp` | Permission state and heartbeat fault latch | Live checks, separate stopping/holding/physical evidence, homing/calibration modes and external reaction integration. #3/#9 |
| `drives/drive.hpp` | Basic capabilities, submit interface, void stop request | Actual adapters, command-mode semantics, drive lifecycle, stop acknowledgement/evidence and loop ownership. #3/#7/#8 |
| `kinematics/solver.hpp` | Request/result and unavailable stub | Real FK/Jacobian/solver, scaled tolerances, residual diagnostics, clock-budget implementation and conformance. #4 |
| `protocol/admission.hpp` | In-memory session/model/sequence check consuming accepted sequence | Atomic queue reservation/commit/ack/retry integration; complete codec, boot epochs, full configuration fingerprint and bounded time mapping. #5 |
| `tasks/program.hpp` | Syntax validation and End marker | Expected IO values, step results, finite branches/retries, resource ownership, interpreter and tool retention. #5/#9 |
| `platform/clock.hpp` | Abstract time source | Coherent hardware monotonic time, wrap handling and measured ISR access. #2/#3 |
| Board/driver/transport README files | Explicit unavailable boundaries | Qualified BSP, pulse engine, sensing, USB/CAN and drive implementations. #2/#3/#6/#7 |
| Existing host tests | Contract behavior and failure placeholders | Deeper numeric/property/fuzz/concurrency tests, target timing, independent physical evidence. #11 |

## Integration rules

Do not wire the current `ExecutionGate` directly to a universal driver-disable action. Do not let `CommandAdmission` consume a sequence and then lose the command on queue failure. Do not treat a valid queue point, successful IK result, End marker or sent gripper command as a complete physical outcome. These are separate layers with separate acceptance gates.

A document checker passing means IDs, references and metadata are coherent. It does not close any implementation issue. The original source/binary provenance and verified board configuration remain unresolved prerequisites for powered deployment.
