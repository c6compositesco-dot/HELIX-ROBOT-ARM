# Existing implementation versus required behavior

The original foundation is e1bad256; the requirements baseline is 3f68253. The subsequent [trajectory/admission slice](../TRAJECTORY_ADMISSION.md) adds real host-tested behavior, but not hardware execution. No implementation issue is closed by documentation or passing host tests alone.

| File/section | Present behavior | Remaining change and owning issue |
|---|---|---|
| `core/types.hpp` | Six-axis vector and point limits | Canonical calibrated configuration/topology and evidence. #3/#10 |
| `joints/transmission.hpp` | Checked signed fixed-ratio mapping | Calibration provenance and explicit coupled-model handling. #10 |
| `feedback/sample.hpp` | Distinct estimate/motor/output validity/reference/age | Acquisition, identity/time uncertainty, plausibility and qualified loops. #6 |
| `motion/waypoint_queue.hpp` | Original single-owner point queue, unchanged | Do not use as full-path approval. #3 |
| `motion/cubic_segment.hpp` | Cubic evaluation, outward-rounded continuous bounds with bounded subdivision | Actual target numeric qualification, collision/quantization margins, stopping horizon and event engine. #3/#9 |
| `runtime/execution_gate.hpp` | Permission/lease faults plus motion-deadline fault code | Live checks, physical stopping/holding, homing and external reaction. #3/#9 |
| `protocol/trajectory_inbox.hpp` | Serialized commit, C2/start and preparation-lead checks, idempotent receipts, live prepared-request authorization and boot/session/lease invalidation | Wire ABI, ACK transport, concurrency, measured target lead, real timebase and actuator integration. #5 |
| `protocol/admission.hpp` | Original standalone sequence check, unchanged | Do not compose it before a fallible queue insert; use the transaction boundary instead. #5 |
| `drives/drive.hpp` | Capabilities and unavailable backend | Actual adapters, mode semantics, stop evidence and loop ownership. #3/#7/#8 |
| `kinematics/solver.hpp` | Interface/unavailable solver | Real FK/Jacobian/IK with timing and shared model conformance. #4 |
| `tasks/program.hpp` | Syntax/End marker | Interpreter, observed IO, bounded branches and tool retention. #5/#9 |
| `platform/clock.hpp` | Abstract source | Hardware monotonic epoch/wrap and concurrency qualification. #2/#3 |
| Board/driver/transport sections | Unavailable hardware boundaries | BSP, pulses, sensors, USB/CAN and actual drives. #2/#3/#6/#7 |
| Tests | Original contracts plus bounds, transactions, deterministic properties, compiler guards and host demo | MCU measurements, concurrency/fuzz transport tests and physical evidence. #11 |

## Integration rules

Cubic certification concerns the mathematical joint trajectory, not collisions or physical tracking. A `Prepared` receipt is not execution or completion. The inbox has no qualified stop generator: it invalidates pending data and faults rather than solving gravity support. Its lease must be serviced by the eventual runtime. Re-arming a previously authorized stream requires a fresh session; old prepared work must not be reused.

Do not wire permission revocation to a universal disable-all policy, or mistake the maximum future-time bound for a validated stopping horizon. The local transaction has one serialized owner; it is not an ISR-safe queue or exactly-once guarantee across power loss.

The root firmware provenance, physical board configuration, canonical model and independent safety/support behavior remain prerequisites for powered deployment. Unimplemented capabilities still fail or remain unavailable. The [prepared-motion increment](../PREPARED_MOTION.md) supplies live software predicates and a stop-horizon design, not an implemented or measured physical stop.
