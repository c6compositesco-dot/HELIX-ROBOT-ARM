# Helix firmware foundation

**Host-testable setup, not runnable robot firmware. No hardware image is produced.** The existing root `firmware.bin`, root README and licence are untouched. Its corresponding source is not established by this work.

Our portable robot-control core targets the Octopus Pro as the retained central controller. Drive and feedback adapters allow open-loop steps, motor sensing, networked drives, joint-output sensing and later suitable brushless drives without changing task-level joint semantics. This is an architecture contract, not hardware compatibility certification.

## Sections and present status

| Section | Present in this revision | Not yet implemented |
|---|---|---|
| `include/helix/core`, `joints` | SI-unit types, fixed-ratio mapping with invalid/overflow rejection | Real robot geometry/calibration, coupled transmissions |
| `include/helix/feedback` | Separate estimate/motor/output samples, validity and age checks | Sensor acquisition and correction loops |
| `include/helix/motion` | Fixed-capacity, single-owner waypoint queue and point checks | Interpolation, whole-segment limits, stopping horizon, pulse output |
| `include/helix/runtime` | Disarmed-by-default permission state, latched faults, watchdog model | Physical stop execution, homing and complete runtime wiring |
| `include/helix/drives`, `drivers` | Capability checks and explicit unavailable adapter | STEP/DIR, encoder and vendor servo adapters |
| `include/helix/kinematics` | Bounded request/result interface, unavailable solver | FK, Jacobian, DLS, target-hardware timing qualification |
| `include/helix/protocol`, `transport` | Version/session/sequence/model/expiry admission | USB/CAN drivers, binary codec, synchronization |
| `include/helix/tasks` | Bounded program-description validation | Interpreter, tool and sensor bindings |
| `boards/octopus_pro_v1_1` | Audit checklist, blocked board-image build | Verified BSP, startup/linker/HAL/pin mapping |
| `tests`, GitHub Actions | Eight host contract suites, GCC/Clang, optional ASan/UBSan | MCU timing and hardware-in-the-loop evidence |

## Run on a development computer

```sh
cmake -S firmware -B firmware/build -DCMAKE_BUILD_TYPE=Debug
cmake --build firmware/build --parallel
ctest --test-dir firmware/build --output-on-failure
```

Sanitizers: add `-DHELIX_SANITIZERS=ON` with GCC/Clang. Tests use synthetic fixtures, never real joint calibration. Tests use checks that remain enabled in Release builds. The header-only core has fixed-capacity storage and no explicit dynamic allocation, exceptions or RTTI; this does not establish a worst-case MCU execution bound.

See [implementation gates](docs/IMPLEMENTATION.md) and [contributor rules](AGENTS.md). No supported path arms physical hardware in this revision. Do not flash the old binary as though this branch rebuilt it.
