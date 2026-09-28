# Continuous trajectory validation and transactional admission

Implementation slice after requirements baseline `3f68253`. Host-tested C++17 only. This is **not flashable firmware**, a motor executor, a physical stop system or a collision planner. Retain the Octopus coordinator and the existing root binary.

## Implemented data path

A caller establishes a fresh boot/session/model context, supplies a qualified stationary start reference and explicitly arms the software admission gate. A typed request is then processed as:

1. Service the existing lease/fault model; reject incompatible ownership, version or model.
2. Resolve retained duplicate receipts without enqueuing again. Reject changed content under the same ID.
3. Check expiry, required positive preparation lead, bounded scheduling horizon and queue capacity.
4. Certify continuous joint position, velocity, acceleration and jerk bounds; check start time and C2 boundary continuity.
5. Read the injected controller clock again; reject a lease, command, start time or preparation lead that expired during validation.
6. Commit the complete six-axis request, receipt, sequence and planned tail in fixed storage; return an admission receipt.

Only a single serialized owner may call the inbox. There are no callbacks, waits, allocations or fallible operations inside commit. This is a logical transaction boundary, **not** a CPU-atomic/ISR-safe or crash-persistent transaction. A board port must implement and qualify its producer/consumer exchange separately.

## Motion format

`motion::CubicSegment` format 1 stores four joint-side Bezier control positions in radians, a controller start time and a positive duration in integer microseconds. All six joints share the same time interval. For each joint:

    q(u) = (1-u)^3*b0 + 3*(1-u)^2*u*b1 + 3*(1-u)*u^2*b2 + u^3*b3
    u = (controller_time - start_time) / duration

The segment has constant jerk. Successive segments may change jerk but must match position, velocity and acceleration within explicitly supplied numerical tolerances. This first representation is not a general quintic, spline fitter, retimer or optimal-motion generator. Nonfinite input, unknown format, zero duration, integer timestamp overflow and durations above 2^53 microseconds are rejected. Evaluation includes endpoints and refuses extrapolation.

## Continuous bounds, not a sampling test

Each scalar curve is converted to Bernstein form; its derivatives are Bernstein curves of lower degree. The convex hull contains the complete polynomial. The validator uses outward-rounded intervals for derivative conversion and de Casteljau bisection, accepting only when the entire hull is inside the configured bounds.

An out-of-range inner control vertex is not itself proof of a limit violation. Subdivision can establish a tighter bound. A provably out-of-range curve endpoint (including subdivision endpoints) returns `Violation`. Exhausted depth/work budget returns `Uncertified`, which admission rejects. Overflow or invalid arithmetic also rejects. Consequently, a valid near-limit curve may be conservatively rejected; acceptance is not promised for every mathematically valid input.

The implementation assumes IEEE-754 binary64 with strict arithmetic and gradual underflow, without FTZ/DAZ or unsafe finite/fast-math assumptions. GCC/Clang builds disable contraction and fast math; negative compiler tests check the explicit header guard. Runtime floating-point configuration on the actual MCU remains a board qualification gate.

The certificate covers the represented mathematical joint polynomial. Evaluation roundoff, pulse quantization, mechanical tracking, collision clearance and stopping space require separately established margins/validation before actuation. No numerical certificate is a functional-safety claim.

Primary mathematical references (method basis, not Helix verification):
- [MIT: Bezier definition, derivatives and convex-hull property](https://web.mit.edu/hyperbook/Patrikalakis-Maekawa-Cho/node12.html).
- [MIT: Bernstein bounds and subdivision](https://web.mit.edu/hyperbook/Patrikalakis-Maekawa-Cho/node42.html).

No source code is copied from these references. The interval implementation and tests are original to this slice; independent review remains required.

## Replay, cancellation and uncertain outcomes

The receipt window is compile-time bounded and stores the typed request, avoiding raw-struct padding comparisons and payload-hash collisions. Identical requests in the retained window return the recorded disposition even after command expiry. They never re-enter the queue. IDs older than the window return `OutcomeUnknown` rather than being executed again. A full queue or failed validation does not consume an ID or change the planned tail, so the unchanged command can be retried.

`Queued` means admitted. `Prepared` means removed for a future preparation layer, not executed. On disarm/fault, queued receipts become `Cancelled`; already prepared work becomes `OutcomeUnknown`. There is no physical completion state in this slice. New sessions clear old pending data and do not clear latched faults. A stream may arm only once per session; re-arming after revoked permission requires a new session, so a previous prepared ticket cannot become valid merely because the same owner re-arms.

Submission reads a `MonotonicClock` at entry and again immediately before the non-failing commit. Scripted-clock tests cover expiry and clock regression during validation. A positive `preparation_lead_us` is mandatory and checked at both points and during preparation handoff. It still requires a measured target-specific budget; synthetic host fixture values are not board settings. See [prepared-motion authorization](PREPARED_MOTION.md) for the live predicates and remaining stopping-horizon boundary.

The caller must provision a unique boot identity and route actual controller time; this module does not invent entropy, authentication or a persistent recovery log. Old-boot/session rejection does not establish whether an earlier physical operation happened. No exactly-once guarantee is made across reset, power loss or an external actuator.

Only valid owner health messages with fresh health-sequence numbers and unexpired timestamps refresh the lease. Ordinary commands, duplicates and invalid traffic do not. Lease expiry and clock regression latch faults and invalidate pending data. A preparation request arriving after its scheduled start latches `MotionDeadline` instead of producing catch-up motion. These are software permission changes, not physical stopping.

## Verification and scope

New tests: `trajectory_bounds`, `trajectory_transactions`, `trajectory_properties`, `trajectory_fast_math_guard`, and `trajectory_demo`. The original eight contract suites remain.

The three new C++ suites execute 432823 checks per configuration, mostly deterministic property-sample assertions, **not 432823 independent scenarios**. They cover invalid inputs, each dynamic limit, legal endpoints with illegal interiors, conservative uncertainty, 512 queue/receipt wraps, retry/changed-ID behavior, receipt eviction, session changes, sequence exhaustion, continuity, deadlines and fault cancellation. The deterministic property corpus has 128 curves, with 99 certified and 29 rejected; accepted curves are compared at 101 timestamps on all six axes against an independent explicit long-double polynomial. Sampling is a regression oracle, not the certificate.

Locally these suites passed GCC 14.2 Debug/Release, Clang 17 Debug/Release and Clang ASan/UBSan. A scoped C++ allocation trap exercised admission, validation and cancellation. The standalone four-segment host demo also passed with sanitizers. The negative floating-point compilation checks passed with both compilers. Full-repository CMake/requirements and old-suite regression are checked by hosted CI; consult the PR's exact revision checks rather than inferring them from these local runs.

Run after checkout:

```sh
cmake -S firmware -B firmware/build -DCMAKE_BUILD_TYPE=Debug
cmake --build firmware/build --parallel
ctest --test-dir firmware/build --output-on-failure
./firmware/build/helix_admission_demo
```

## Requirements contribution, not blanket closure

| Requirements | Contribution in this slice | Still required |
|---|---|---|
| MOT-001/002/004/010/011 | Typed cubic format, conservative continuous bounds, stationary start/C2 checks, bounded admission queue | Wire ABI, calibrated tolerances, concurrency and target qualification |
| API-003/004 | Single-owner commit, retained duplicate receipts, conflict/eviction handling and retry after full/rejection | Actual packet codec, ACK transport, fault/power interruption integration |
| API-005/006/007/009 | Boot/session identity, bounded future scheduling, fresh health messages and honest admission dispositions | Authenticated gateway, hardware clock/time mapping, physical completion evidence |
| MOT-005/006/009 | Late preparation rejected; no extrapolating evaluator | Stop-horizon planning, timely producer-starvation detection and qualified physical stop/support |

Do not close issues #3 or #5 from this slice alone. Next actuator-facing work is the qualified stopping/preparation horizon and timed event engine, alongside the board/source audit (#2). Actual FK/IK (#4), canonical model/calibration (#10), USB/CAN, sensing, tools and task interpretation remain separate implementation work.
