# Reusable tasks, tool IO and simulation-before-execution

Sources: [S09], [S18]. Status: specification-only. Issues #5 and #10. The current task container is a syntactic description, not an operational interpreter.

## Task contract

A task/skill declares required devices, references, tools, sensing, approved trajectory resources, expected inputs, parameters, success evidence, timeouts and recovery permissions. The first interpreter is a bounded state machine over approved actions, not a general scripting VM. Support finite conditional branches and bounded retries with a total execution deadline. AI may propose a replacement action at an allowed boundary; it does not own the interpreter's timing or safety decisions.

For the first print-cell task: printer-ready-for-access signal -> approved approach -> grip action -> required grip evidence -> approved withdrawal with attached part -> placement -> release -> verify clearance/completion. The part must be released or mechanically releasable through an explicitly qualified operation. Grasp planning does not solve arbitrary bed adhesion. The printer/arm/tool handshake is part of the cell contract; unsupported printers are not silently treated as compatible.

## Task requirements

### TASK-001: Declarative skill prerequisites
Phase: R1; Issue: #5; Origin: user
Requirement: A reusable task shall declare its required capabilities, resources, configuration, references and operating conditions before execution. Robot-independent intent shall be separated from qualified cell-specific trajectories.
Acceptance: The same task template accepts compatible mock cells and rejects missing grippers, references, sensors or unsupported printer states before motion.

### TASK-002: Bounded interpreter
Phase: R1; Issue: #5; Origin: derived
Requirement: Task programs shall have bounded instructions, state storage, branch/retry counts and total wall-clock duration. No unbounded recursion, loop or arbitrary code execution shall enter the real-time runtime.
Acceptance: Static validation and execution tests reject cycles without bounds, invalid resources and exhausted budgets; interpreter work per tick remains bounded.

### TASK-003: Explicit step semantics
Phase: R1; Issue: #5; Origin: derived
Requirement: Each action shall define entry conditions, desired IO value or trajectory identity, completion evidence, timeout and permitted failure transition. Program End shall not override unfinished or failed actions.
Acceptance: Tests cover every action outcome, including a valid syntax sequence that fails physically; End never reports success after a lost grip or aborted motion.

### TASK-004: Resource ownership and cell handshake
Phase: R1; Issue: #5; Origin: derived
Requirement: Tasks shall acquire exclusive control of conflicting cell resources and respect printer/tool readiness and access permissions. An arm shall not enter a printer merely because a nominal print timer elapsed.
Acceptance: Simulate printing, cooling, door/bed motion, stale readiness and competing tasks; conflicting or unverified access is refused.

### TASK-005: Bounded recovery
Phase: R1; Issue: #5; Origin: derived
Requirement: Recovery shall use explicitly approved actions with retry/deadline limits and fresh observation. A failed step shall not trigger unplanned improvisation or automatic replay of a potentially completed physical action.
Acceptance: Ambiguous pickup, lost acknowledgement and obstacle scenarios either select an approved recovery or request intervention while maintaining the defined support state.

### TASK-006: Cancellation and interruption
Phase: R1; Issue: #9; Origin: derived
Requirement: Pause/cancel/fault shall propagate through motion and tool state using qualified stop/retention behavior. Cancellation shall not immediately release a held object or mark unfinished work successful.
Acceptance: Interrupt every task state and verify resulting motion, tool, completion and restart semantics.

### TASK-007: Task data without firmware reflashing
Phase: R1; Issue: #5; Origin: derived
Requirement: New tasks using supported primitives shall be uploaded as versioned validated data. Changes requiring new primitive behavior shall be explicit firmware capability changes.
Acceptance: Load two distinct bounded tasks without reflashing; incompatible versions/resources are rejected and task identity is recorded in telemetry.

## Tool and IO requirements

### TOOL-001: Typed tool resources
Phase: R1; Issue: #5; Origin: derived
Requirement: Tools shall declare electrical/logical interface, command units, legal states, limits and available feedback. A generic output bit shall not imply a universal gripper interface.
Acceptance: Mock digital, servo and sensor-confirmed tools expose only their supported actions; wrong tool identity or illegal command values fail admission.

### TOOL-002: Observed grip success
Phase: R1; Issue: #5; Origin: derived
Requirement: Grasp success shall use the evidence required by the task, such as validated position, force/current interpretation, vacuum or vision feedback. Sending a close command alone shall not imply retained payload.
Acceptance: Empty grasp, partial grip and missing feedback cases remain failed/unverified and prevent dependent withdrawal when the task requires retention evidence.

### TOOL-003: Safe release authorization
Phase: R1; Issue: #9; Origin: derived
Requirement: Tool release shall require the appropriate task/mode/location conditions and shall not occur automatically on host disconnect or arbitrary task cancellation.
Acceptance: Disconnect/cancel tests at each carrying stage preserve the qualified retention/fallback policy; deliberate release remains possible only through authorized actions.

### TOOL-004: IO filtering and fault plausibility
Phase: R1; Issue: #5; Origin: derived
Requirement: Input filtering/debounce, electrical fault detection and signal age shall be specified per signal. Filtering shall not hide a critical event beyond its reaction budget.
Acceptance: Bounce, stuck levels, open circuits and delayed samples exercise both normal and fault paths with measured latency.

### TOOL-005: Task-specific mechanical release
Phase: R1; Issue: #10; Origin: derived
Requirement: Printer-part removal shall declare bed release/cooling/access conditions and any qualified detachment mechanism. Arbitrary part geometry and adhesion shall not be assumed solved by grasp selection.
Acceptance: The first demo uses a defined releasable part/cell and verifies withdrawal clearance with the part attached; incompatible jobs are rejected before the task starts.

## Simulation and approval requirements

### SIM-001: Configuration-bound preflight
Phase: R1; Issue: #10; Origin: user
Requirement: Preflight approval shall bind model, calibration, tool/carried-object geometry, limits, trajectory semantics and relevant cell/scene state. Approval shall not be reused after an incompatible change.
Acceptance: Mutate each bound property and confirm invalidation; recorded approvals identify exactly what was checked rather than merely a generic simulation success flag.

### SIM-002: Validate the actual swept motion
Phase: R1; Issue: #10; Origin: derived
Requirement: Host preflight shall evaluate the actually interpolated path, approach/withdrawal, carried part and qualified stopping envelope with uncertainty margins. Endpoint reachability alone shall not constitute approval.
Acceptance: Fixtures with clear endpoints but colliding links/parts/interior paths or invalid stopping space fail preflight.

### SIM-003: Shared semantics, independent checks
Phase: R1; Issue: #10; Origin: derived
Requirement: Simulation and firmware shall use the same core/model conventions where practical, with independent reference checks to detect common-mode errors. Pulse quantization and backend timing shall be represented to the required fidelity.
Acceptance: Golden traces compare host evaluation, firmware output and measured fixture motion; an intentionally wrong shared convention is detected by an independent oracle.

### SIM-004: Observe before continuing
Phase: R1; Issue: #5; Origin: derived
Requirement: Physical task execution shall verify required outcomes and scene assumptions before proceeding. Simulation success shall not replace sensing of grip, clearance or device readiness where required.
Acceptance: Change the physical/mock outcome while retaining the same simulated plan; the task stops or recovers rather than declaring the simulated result physically achieved.

### SIM-005: Bounded dynamic AI proposals
Phase: R1; Issue: #5; Origin: user
Requirement: AI-generated grasps or movements shall enter through the same configuration, path and execution admission gates as other commands. Initial dynamic replacement shall occur at defined supported boundaries, not arbitrary motor-loop writes.
Acceptance: Adversarial/unreachable/colliding/stale proposals are rejected without disturbing admitted stop/support behavior; delayed AI responses cannot keep a dead lease alive.

### SIM-006: Progressive cell qualification
Phase: R1; Issue: #10; Origin: derived
Requirement: Qualification shall progress from fixed simulated geometry to instrumented fixtures and repeatable cell tasks, varying one defined uncertainty at a time. A few successful cycles shall not be advertised as general unattended reliability.
Acceptance: Record failures, intervention rates, cycle counts and operating conditions against P-ENDURANCE; each expansion of geometry/load/environment has a declared evidence gate.
