# Helix Robot Arm

Helix is C6's six-axis, printed-structure, belt-driven robot-arm project. The first intended task is repeatable camera motion and product filming for makers and creators who are willing to build and calibrate their equipment.

**Development stage, 28 September 2026:** individual-joint testing and assembly. A coordinated camera task, independent customer build, payload rating and repeatability figure have not yet been qualified. The first customer release is not ready. This repository is a control-software development workspace, not a complete build kit or a supported hardware-flashing package.

## What is available here

| Component | Current evidence | Not yet established |
|---|---|---|
| Portable C++17 control core | Fixed-size six-joint types, transmission checks, permission/fault model and explicit unsupported adapters | Calibrated machine configuration and physical execution |
| Continuous trajectory validation | Host-tested cubic evaluation and conservative position/velocity/acceleration/jerk bounds | Collision clearance, physical tracking and target numeric/timing qualification |
| Transactional trajectory inbox | Host-tested ownership, sequence/receipt handling, preparation deadlines and live prepared-request authorization | Wire transport, concurrent runtime integration, pulse output and stopping/holding |
| Kinematics, feedback and drives | Interfaces and capability/validity checks | Operational IK solver, acquired encoder measurements and qualified hardware backends |
| Requirements | 153 staged requirements with owning issues, acceptance procedures and unresolved parameters | Evidence that every requirement has been implemented or accepted |
| Original root `firmware.bin` | Preserved historical binary | Its corresponding source/configuration and a verified recovery/flash procedure |

Start with the [firmware overview](firmware/README.md), [trajectory/admission behavior](firmware/docs/TRAJECTORY_ADMISSION.md), [prepared-motion boundaries](firmware/docs/PREPARED_MOTION.md) and [implementation gaps](firmware/docs/requirements/INTEGRATION_GAPS.md). Host checks do not demonstrate physical motion or safety.

## Run the host checks

Use a C++17 compiler, CMake and Python. These commands build desktop test programs only:

```sh
cmake -S firmware -B firmware/build -DCMAKE_BUILD_TYPE=Debug
cmake --build firmware/build --config Debug --parallel
ctest --test-dir firmware/build -C Debug --output-on-failure
python3 -m unittest discover -s firmware/tools -p test_requirements_checker.py -v
python3 firmware/tools/check_requirements.py
```

Repeat in Release for a code change. GCC/Clang builds support `-DHELIX_SANITIZERS=ON`. Fixtures use synthetic values; their geometry, limits, timing and permissions are not robot settings. The requirements checker verifies specification structure, not implemented or physical behavior.

There is no supported command to flash or operate the arm from this revision. The board-image build remains blocked until its required evidence is supplied. Do not use the historical binary as an inferred match for the new core.

## First release and performance

The [public programme page](https://helix-print-as-you-go.william-thom-6102.chatgpt.site/build) describes the proposed six-release digital build programme and its launch conditions. Customers would separately source components and print/assemble the arm. That offer is not an available hardware kit or a completed release.

An authoritative bill of materials, print/material settings, wiring instructions, calibration procedure and measured camera demonstration are still required. Private mechanical sources and calibration are not included in this public firmware repository.

Earlier README figures of **2 kg payload**, **≤0.2 mm repeatability** and **under US$400** were unverified targets, not measured specifications or a current delivered-price commitment. They must not be used as product performance claims. No payload, reach, speed, repeatability or complete build-cost rating is established here.

## Development gates

1. Preserve the existing machine and recovery information; identify the exact board, driver and mechanical configuration.
2. Complete host-level behavior and fault tests within the selected R1 scope.
3. Qualify target clocks, numerical behavior, pulse timing, interfaces and load-support response before powered deployment.
4. Validate the full camera task and an independent build against dated, configuration-specific measurements.
5. Release supported files, documentation and operating limits only after those results exist.

The Octopus Pro remains the planned central coordinator. Later feedback, networked drives and onboard IK have separate gates and are not implemented here. Host-planned R1 operation does not depend on onboard IK. Supervisor or AI availability cannot replace local motion limits and fault handling. See the [requirements phases](firmware/docs/requirements/README.md), [qualification plan](firmware/docs/requirements/QUALIFICATION.md) and [contributor rules](firmware/AGENTS.md).

Normal permission revocation is not a physical stopping solution. Stopping, gravity support, power loss and tool retention remain unresolved physical obligations. A passing software test is not permission to remove motor power under load or operate an unqualified arm.

## License and contributions

The repository contains an [MIT license](LICENSE). Preserve its notice. That file does not establish rights to unpublished CAD, third-party assets or the historical binary's unknown source provenance. Keep private geometry, calibration, credentials and commercial documents out of public fixtures.

Contributions should identify the relevant requirement, describe implemented versus unavailable behavior, and include evidence appropriate to the changed scope. Do not close physical qualification work using host tests alone.
