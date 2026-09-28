# Octopus board support and real-time execution

Sources: [S01], [S02], [S03], [S10], [S23]. Status: specification-only. Interfaces: `boards/octopus_pro_v1_1`, `platform/clock.hpp`, future scheduler, pulse engine and peripheral HAL.

## Design

Keep a compact portable control core and a pinned board-support layer. Verify the actual MCU, clock, bootloader and V1.1 routing instead of choosing settings by board-family name. ST's H723ZE specification is a resource reference, not evidence of the fitted component or usable DMA/pin combinations [S01]. The official schematic remains an audit input, not a qualified wiring plan [S02].

Use hardware-timed pulse execution with bounded interrupt work. A cooperative/background numerical worker is the first architecture to evaluate; an RTOS is allowed only with explicit priority, blocking and ownership analysis. Neither an RTOS nor DMA automatically guarantees deadlines. Prefer static allocation after startup and a single owner per peripheral/queue, with an explicit ISR-to-worker exchange.

H7 cache/DMA integration must use memory reachable by each master and either qualified cache maintenance or deliberately noncacheable buffers. Alignment and barriers are part of the driver design; do not copy memory addresses from a different H7 part [S03].

## Board requirements

### BSP-001: Physical target identity
Phase: R1; Issue: #2; Origin: derived
Requirement: Board startup/build shall be bound to verified MCU marking, silicon revision, flash/RAM allocation, oscillator/clock plan and bootloader offset. Unknown audit fields shall prevent powered release.
Acceptance: Compare the built target and runtime identity against the physical audit; deliberately mismatched targets are rejected by release tooling.

### BSP-002: Reproducible baseline and recovery
Phase: R1; Issue: #2; Origin: derived
Requirement: Preserve the existing image/settings and establish their provenance where possible before replacing them. A tested recovery route shall exist independently of successful application startup.
Acceptance: Record image hashes, source/toolchain/configuration when recovered, and demonstrate recovery from a nonbooting test image using a reviewed bench procedure.

### BSP-003: Reviewed pin and electrical map
Phase: R1; Issue: #2; Origin: derived
Requirement: GPIO functions, voltage domains, pull states, enable polarity, driver sockets, endstops and tool outputs shall come from the exact revision and physical wiring audit, not guessed defaults.
Acceptance: Non-actuating continuity/logic checks match the reviewed map; incorrect or duplicate allocations fail configuration validation.

### BSP-004: Reset and boot output behavior
Phase: R1; Issue: #9; Origin: derived
Requirement: Boot, reset, brownout, bootloader entry and firmware faults shall have qualified output states that account for gravity and tool retention. Disarmed software shall not be equated with safe electrical behavior.
Acceptance: Observe outputs through each transition with external instruments and verify the separately engineered mechanical/electrical reaction.

### BSP-005: Peripheral ownership
Phase: R1; Issue: #2; Origin: derived
Requirement: Timers, DMA streams, interrupts, buses and GPIO ports shall have an explicit allocation/priority table. Encoder and bus additions shall recheck conflicts with stepping, timekeeping and USB.
Acceptance: Build-time/configuration conflict checks and worst-case concurrent bench tests cover all enabled peripherals.

### BSP-006: Memory and coherency
Phase: R1; Issue: #2; Origin: derived
Requirement: Linker placement shall distinguish instruction/data/stack/DMA regions, document cache policy and protect DMA ownership transitions. No resource estimate shall treat aggregate RAM as one unrestricted arena.
Acceptance: Inspect map files and instrument stack high-water marks; cache-on stress tests with adversarial DMA lengths/alignment show no stale or corrupted data.

### BSP-007: Monotonic time
Phase: R1; Issue: #3; Origin: derived
Requirement: The runtime shall expose a coherent 64-bit monotonic controller timebase with documented wrap extension and ISR access. Wall-clock updates shall not change motion time.
Acceptance: Test wrap boundaries, torn-read attempts, clock regression, reset epochs and simultaneous captures; future commands from an old epoch are rejected.

### BSP-008: Independent watchdog design
Phase: R1; Issue: #9; Origin: derived
Requirement: A hardware watchdog, where supported, shall be fed only by a qualified health mechanism reflecting critical task progress, not unconditionally by the stalled subsystem's timer interrupt.
Acceptance: Freeze each critical task while other interrupts continue; the watchdog/reaction occurs as specified and reset cannot automatically resume motion.

### BSP-009: Persistent storage restrictions
Phase: R1; Issue: #11; Origin: derived
Requirement: Flash/SD writes, erase operations and configuration activation shall not block time-critical execution. Persistent data shall use integrity/version checks and an interruption-tolerant update procedure.
Acceptance: Inject power interruptions throughout writes and stress logging requests during motion; timing bounds and last-valid configuration behavior remain satisfied.

### BSP-010: Hardware image release gate
Phase: R1; Issue: #2; Origin: derived
Requirement: Removing the current board-image block shall require a reviewed BSP change with verified startup, recovery, output behavior and test evidence. A successful host build alone shall never remove this gate.
Acceptance: CI rejects incomplete board profiles and records which audit/qualification artifacts permit each released target.

## Real-time requirements

### RT-001: Workload separation
Phase: R1; Issue: #3; Origin: derived
Requirement: Pulse scheduling, trajectory evaluation, fault detection, transport parsing, diagnostics and IK shall have separate bounded execution responsibilities. USB arrival or Unity frame timing shall not directly schedule pulses.
Acceptance: Saturate transport and numerical workloads while measuring pulse timing and fault latency; the qualified bounds remain met.

### RT-002: Deadline and blocking budget
Phase: R1; Issue: #3; Origin: derived
Requirement: Each periodic/event workload shall declare period, deadline, maximum blocking, priority, execution budget and overload policy. Proposed P-TICK and P-IK values are not release evidence.
Acceptance: Combine measured worst cases with interrupt/blocking analysis; validate cold/hot-cache and concurrent-IO cases, not average timing alone.

### RT-003: Bounded storage and work
Phase: R1; Issue: #11; Origin: derived
Requirement: Time-critical paths shall use bounded storage/work and avoid allocation, blocking filesystem/serial calls, uncontrolled recursion and unbounded retries. Library internals shall be audited as well as our call sites.
Acceptance: Allocation hooks, stress fixtures, static review and runtime timing instrumentation detect any prohibited activity after initialization.

### RT-004: Explicit concurrency contract
Phase: R1; Issue: #3; Origin: derived
Requirement: Every shared queue and state snapshot shall declare single-owner, atomic or bounded-critical-section access. Multiword data shall not be presumed atomic because the processor is 32-bit.
Acceptance: Host concurrency fixtures and target interrupt-injection tests verify coherent snapshots and absence of use-after-publish mutation.

### RT-005: Numerical isolation
Phase: R1-IK; Issue: #4; Origin: derived
Requirement: The solver shall yield or terminate at bounded computation boundaries and never delay the pulse/fault path. A deadline shall include elapsed preemption time, not only solver CPU cycles.
Acceptance: Interrupt the solve at every testable boundary; timeout returns no executable candidate and motion continues only through already admitted execution/stop data.

### RT-006: Overload is a fault condition
Phase: R1; Issue: #9; Origin: derived
Requirement: Missed execution deadlines, event-buffer underrun and invalid time state shall trigger documented local reactions. The controller shall not conceal overload by issuing an unsafe burst of catch-up motion.
Acceptance: Induce each overload and verify event ordering, bounded reaction and latched diagnostic evidence; no stale segment is executed merely to empty a queue.

### RT-007: Measured headroom
Phase: R1; Issue: #11; Origin: derived
Requirement: Release evidence shall report flash, stack, RAM-region usage, interrupt load and timing headroom for the enabled configuration. Added features shall rerun the combined workload qualification.
Acceptance: Compare baseline and encoder/network/IK-enabled profiles using the same measurement campaign and reject configurations that exceed their approved budgets.
