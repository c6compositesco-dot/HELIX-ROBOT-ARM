# Foundation validation record

Run locally on 2026-09-28. This is host contract testing, not hardware qualification. Test source Git blob: `d9a7ba42df58b922a30ec1caeb13fb28bc4af21c`. The tested source and CMake bytes match the foundation Git objects; documentation additions do not alter those bytes.

## Results

| Configuration | Result |
|---|---|
| GCC 14.2.0 / Debug | 8 of 8 suites passed |
| GCC 14.2.0 / Release | 8 of 8 suites passed |
| Clang 17.0.0 / Debug | 8 of 8 suites passed |
| Clang 17.0.0 / Release | 8 of 8 suites passed |
| Clang 17.0.0 / Debug / AddressSanitizer + UndefinedBehaviorSanitizer | 8 of 8 suites passed; no sanitizer reports |
| Configure with HELIX_BUILD_BOARD_IMAGE=ON | Rejected with the intended board-image gate message |

CMake: 3.31.6. Warnings treated as errors on tested GCC/Clang builds. Checks remain active in Release.

Suites and executed checks per configuration: transmission 31; feedback 11; motion 118; runtime 27; protocol 10; drives 13; kinematics 3; tasks 14. Total: 227 checks per configuration. Repeated fixtures are counted as executions, not 227 independently distinct test cases.

## Scope of evidence

- Invalid and nonfinite values, transmission overflow, missing/stale feedback, queue bounds/order, startup permissions, fault latching, heartbeat expiry and time regression are covered by synthetic cases.
- Unsupported drive modes and unavailable hardware/IK adapters are checked for explicit failure.
- Protocol checks cover version, session, sequence, model revision and expiry, not wire framing or authentication.
- Task tests validate bounded descriptions, not physical task execution.
- No embedded toolchain build, MCU runtime measurement, ISR load measurement, GPIO action, USB/CAN communication, encoder acquisition, kinematic solve or real motor test has been performed.
- No hardware image is generated and no device was flashed. The legacy root firmware.bin is unchanged.
- The GitHub Actions workflow is configured separately; this record does not claim a hosted CI result. Consult the actual run status for that evidence.

## Reproduce

```sh
cmake -S firmware -B firmware/build -DCMAKE_CXX_COMPILER=g++ -DCMAKE_BUILD_TYPE=Debug
cmake --build firmware/build --parallel
ctest --test-dir firmware/build --output-on-failure
```

Repeat with Release and/or clang++. For the sanitizer run use clang++, Debug, and `-DHELIX_SANITIZERS=ON`. The next gate is source/hardware baseline recovery and non-actuating board support, not motor enablement.
