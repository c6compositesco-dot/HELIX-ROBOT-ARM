# Methods, alternatives and evidence strength

This is a design decision record, not a list of universally best algorithms. Mature implementation patterns, newer peer-reviewed candidates and vendor capability documentation have different evidential weight. None has been benchmarked on Helix by this requirements expansion. Sources resolve through sources.json.

| Area | Selected direction | Alternatives and reason | Evidence and remaining gate |
|---|---|---|---|
| Local six-axis IK | Own fixed-size FK/geometric Jacobian plus scaled bounded LM/DLS; factorization rather than explicit inverse | TRAC-IK/SQP as host comparison, not concurrent variable-work MCU search | KDL and Pinocchio implementation patterns [S04, S05], TRAC-IK [S06]; actual geometry, deadline and constrained convergence must be tested |
| Geometry-specialized IK | Benchmark IK-Geo/EAIK after classifying the real chain; retain the same interface and fallback | Do not assume a spherical wrist or that an analytical method stays exact for a calibrated nonideal model | 2025 research [S26, S27]; reported desktop advantages are not Octopus measurements |
| Trajectory generation | R1 host-prepared continuous trajectories, validated playback and stop horizon | Evaluate Ruckig state-to-state as an oracle/component; no cloud waypoint execution and no blind copy of current C++20 dependency into C++17 core | RSS method [S07], current implementation constraints [S08]; embedded allocation/timing and licence review remain |
| Path integrity | Validate actual interpolation, carried geometry, quantization and stop space after any geometric modification | Endpoint-only validation and unexamined smoothing are rejected | Existing robotics documentation explicitly warns about retiming path deviation [S09]; Helix collision pipeline remains host-side work |
| STEP/DIR | Hardware-timed bounded event engine with residual-preserving quantization | Timer-compare ISR versus DMA chosen from actual routing and measured load, not fashion | Driver electrical requirements [S10]; GPIO/timer/DMA qualification required |
| Feedback | Observation-first, timestamped independent channels, bounded correction, one owner per loop | Blind catch-up correction and motor-angle-as-output-position shortcuts are rejected | Existing dual-encoder drive documentation [S12, S13]; real transfer function and sensors determine gains |
| Drive architecture | Retained central robot coordinator with local servo electrical loops | Central per-PWM network control is rejected | moteus provides an implemented example [S11]; each selected drive still needs an adapter and qualification |
| Network | Qualified CAN FD first, vendor application adapters, measured clocks/skew | Classic CAN may fit a smaller budget; EtherCAT reserved for measured industrial needs, not automatic board replacement | Standards/vendor sources [S14-S17]; stock Octopus FD electrical capability remains unverified |
| USB framing | Bounded COBS framing and explicit CRC-32C candidate, versioned transactional commands | Text G-code compatibility belongs in an explicit adapter, not an ambiguous wire parser | Original COBS and CRC references [S24, S25]; no codec is implemented here |
| Runtime faults | Separate admission permission, physical stopping/support, fault latch and recovery | Universal disable-all and universal keep-driving policies are both rejected | System integration guidance [S18, S19]; independent hardware safety design and measurements remain |
| Firmware resilience | Protect/detect/recover, pinned compatible dependencies, controlled updates | No assumed dual-bank rollback, internet-facing MCU or CRC-as-authentication | NIST and ST guidance [S20, S23]; bootloader/flash/trust-model audit required |

## Stronger analytical IK deserves a benchmark, not a marketing promise

IK-Geo classifies all-revolute chains into geometry-dependent subproblems; favorable families admit closed-form solutions while more general cases require search [S26]. EAIK automates decomposition for supported geometries [S27]. Their relevance is potentially reducing solve work and obtaining alternative configurations. Their result sets still require joint-limit, continuity, calibration, collision and trajectory admission checks. Small physical axis misalignments can affect whether an idealized analytical model represents the calibrated arm adequately; evaluate this explicitly.

### IK-011: Geometry-specialized solver comparison
Phase: R1-IK; Issue: #4; Origin: derived
Requirement: After canonical geometry is available, evaluate applicable analytical/subproblem methods against the bounded numerical baseline using the same calibrated-model corpus, limits, continuity and target hardware budgets. Select by measured suitability, not published desktop headline speed.
Acceptance: Record geometry eligibility, residuals, all-candidate/branch handling, failure cases, memory and loaded-MCU timing; an alternative is enabled only after the shared solver conformance gates pass.

## Decisions deliberately deferred

Actual motor currents, GPIO maps, gains, stop distances, watchdog values, encoder types, CAN data rate and product accuracy/reliability numbers require measurements or hardware selection. They are recorded in parameters.json and the hardware audit rather than fabricated. A required measurement is an assigned release gate, not permission to omit the corresponding implementation design.

No third-party implementation code is imported by this specification. Ruckig, KDL, Pinocchio, IK-Geo, EAIK, moteus and ODrive references do not transfer ownership of the Helix architecture or establish interoperability by citation alone.
