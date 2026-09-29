# Helix firmware foundation

**Host-testable control code, not runnable robot firmware. No hardware image is produced.** The existing root `firmware.bin`, root README and licence are untouched. Its corresponding source is not established by this work.

Our portable robot-control core retains the Octopus Pro as central controller. Drive and feedback adapters support the planned progression from open-loop steps to motor sensing, networked drives, joint-output sensing and external brushless drives. This is an architecture target, not hardware certification.

## Implemented next slice

[Continuous trajectory validation and transactional admission](docs/TRAJECTORY_ADMISSION.md) now provides a six-axis cubic/constant-jerk representation, a bounded conservative continuous-limit validator, an evaluator and a fixed-storage transactional inbox. Failed/full submissions do not consume command IDs; retained duplicates do not enqueue twice. Session changes, lease expiry and faults invalidate pending work without claiming a physical stop. A host-only four-segment demo exercises the path.

## Requirements and researched design

Start with the [requirements baseline](docs/requirements/README.md), [methods and alternatives](docs/requirements/METHODS.md), [implementation gaps](docs/requirements/INTEGRATION_GAPS.md) and [qualification plan](docs/requirements/QUALIFICATION.md). Each requirement has an ID, phase, owning issue and acceptance procedure. [Sources](docs/requirements/sources.json) identify evidence and limitations; [parameters](docs/requirements/parameters.json) separate study targets from measurements. Specification is not implementation evidence.

## Sections and present status

| Section | Present | Remaining |
|---|---|---|
| `core`, `joints` | SI-unit types and checked fixed-ratio mapping | Real geometry, calibration and coupled transmissions |
| `feedback` | Separate estimate/motor/output samples with validity/age checks | Acquisition, plausibility and correction loops |
| `motion` | Point queue; cubic evaluation and continuous position/velocity/acceleration/jerk certification | Timed playback, stop horizon, pulse output and collision/quantization margins |
| `runtime` | Permission state, fault latch and lease model | Live interlocks, physical stopping/support and homing |
| `drives` | Capability checks and explicitly unavailable adapters | Qualified STEP/DIR, network and servo backends |
| `kinematics` | Bounded request/result interface and unavailable solver | Actual FK/Jacobian/IK and MCU timing |
| `protocol` | Single-owner trajectory transaction, receipt window, session/lease integration | Wire codec, USB/CAN, authentication and clock mapping |
| `tasks` | Bounded program-description syntax | Interpreter, tools and sensor bindings |
| `boards/octopus_pro_v1_1` | Audit checklist and blocked board image | Verified BSP, startup/linker/clocks/pins/peripherals |
| Tests/CI | Eight original suites plus five new test entries on GCC/Clang | Hardware timing, electrical and physical qualification |

## Run on a development computer

```sh
cmake -S firmware -B firmware/build -DCMAKE_BUILD_TYPE=Debug
cmake --build firmware/build --parallel
ctest --test-dir firmware/build --output-on-failure
./firmware/build/helix_admission_demo
python3 -m unittest discover -s firmware/tools -p test_requirements_checker.py -v
python3 firmware/tools/check_requirements.py
```

Add `-DHELIX_SANITIZERS=ON` for GCC/Clang. Fixtures are synthetic, not machine settings. Assertions remain enabled in Release. The trajectory validator requires strict binary64 arithmetic with gradual underflow; the Octopus floating-point configuration and loaded timing are unverified.

See [implementation gates](docs/IMPLEMENTATION.md) and [contributor rules](AGENTS.md). No supported path enables physical hardware in this revision. Do not treat a passing host demo as permission to flash or drive the arm.
