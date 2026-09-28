# Implementation gates

This branch starts the software structure, not a replacement firmware deployment. Order work by evidence, not by advertised feature count.

## 1. Reproducible baseline and board support
Recover exact firmware source/configuration and preserve a tested recovery image. Verify chip, bootloader, pin map, supply/driver limits, timers, DMA and memory placement. Reconcile host units and command semantics. Gate: reproducible build and non-actuating bring-up with known reset/enable states. No motor outputs before review.

## 2. Execution runtime and current stepper backend
Integrate the permission model with actual local fault reaction, homing authorization, a qualified trajectory generator and hardware-timed pulse output. Add whole-segment velocity/acceleration/jerk checks and stopping horizon. The waypoint queue validates points only; never mistake that for a validated path. Gate: unloaded single-channel measurements, then coordinated timing and fault tests. Stopping under gravity requires a physical design, not a state enum.

## 3. Shared kinematics
Port the actual numerical conventions after auditing Motion Studio. Validate FK/Jacobian/DLS against shared reference vectors, branch continuity, units, tools and joint limits. Honor elapsed deadlines as well as iteration count. Never execute partial or failed solutions. Initial 100 Hz/2 ms ideas are unmeasured targets. Gate: host conformance then worst-case loaded-board measurements; preserve simulated branch/configuration approval.

## 4. Encoder feedback
Acquire one motor encoder in observation-only mode. Qualify polarity, scale, reference, timestamps, age, noise and following-error reporting before enabling bounded correction. Add separate output sensing later. Gate: injected faults are detected; no false physical-position claims. Tune any dual-loop design against measured transmission dynamics.

## 5. Host and network integration
Implement versioned bounded USB framing and protocol dispatch. Bind session invalidation to runtime permission revocation and queue clearing. Qualify CAN electrical interface and one drive protocol before scaling. Mixed local/remote actuation needs measured application skew and local watchdogs. Gate: malformed/stale/duplicate packets, link loss, node reboot and bus faults do not cause automatic re-arming or stale replay.

## 6. Tasks and later servo backends
Bind bounded task descriptions to validated trajectories and observed IO, with deadlines and explicit failure states. New tasks are data, not firmware reflashes. Qualify brushless local drives separately; preserve joint units and capability negotiation. Gate: the same host workflow works against a simulation fixture and qualified physical backend with honest completion evidence.

## Integration obligations not solved by the initial contracts

- `ExecutionGate` does not read interlocks or stop motors. The caller must supply current checks and invoke a qualified physical reaction.
- `CommandAdmission` is not a byte parser, retry protocol, authentication layer or trajectory approval. Queue failure/ack/retry behavior needs an atomic dispatch design.
- `WaypointQueue` is single-owner and not safe for concurrent ISR access without an explicitly designed exchange.
- No measured feedback is created by the placeholder drive. No solver or task executor is operational.
- GPIO reset behavior, brakes, emergency stopping, thermal/electrical protection, calibration and timing remain hardware gates.

## Primary implementation references

No third-party control code is copied in this foundation. Recheck and pin any dependency before importing it.

- Board vendor: https://github.com/bigtreetech/BIGTREETECH-OCTOPUS-Pro
- MCU vendor: https://www.st.com/en/microcontrollers-microprocessors/stm32h723ze.html
- STM32 HAL: https://github.com/STMicroelectronics/stm32h7xx-hal-driver
- CMake C++ standard handling: https://cmake.org/cmake/help/latest/prop_tgt/CXX_STANDARD.html
- Checkout action release pin: https://github.com/actions/checkout/releases/tag/v7.0.1

Source licence of the legacy binary remains a separate provenance question; this work does not relicense it.
