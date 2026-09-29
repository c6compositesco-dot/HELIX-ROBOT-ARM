# Shared forward kinematics and bounded inverse kinematics

Sources: [S04], [S05], [S06]. Status: specification-only. Issue #4. Interface: `kinematics/solver.hpp`; current `UnavailableKinematics` intentionally remains unimplemented.

## Selected direction

Implement a fixed-size six-joint geometric model and analytical geometric Jacobian, with cached transforms and a scaled damped-least-squares/Levenberg-Marquardt solver. KDL documents adaptive damping, task weighting and allocation outside the solve path [S04]; these are reference techniques, not a dependency on its full runtime. Use double precision initially to match the existing interface, then evaluate float only through error and timing evidence.

Minimize a dimensionless weighted pose residual. Position and orientation tolerances are separate physical quantities; do not sum metres and radians without an explicit scale. Use a rotation-log/quaternion-derived orientation residual in a documented frame. Its differential must match the chosen error definition, particularly for large angles; verify against finite differences rather than assuming the geometric angular Jacobian is the exact derivative everywhere [S05]. Solve the small linear system by an appropriate factorization, not an explicit matrix inverse.

Preserve joint feasibility and bounded steps with a capped active-bound/line-search strategy. Clamp-and-hope is not a convergence guarantee. Nearby targets use the relevant previous accepted trajectory configuration as a seed; achieved feedback and future planned configuration are distinct states. Pose convergence alone neither proves continuous configuration nor a collision-free path.

TRAC-IK's concurrent numerical/SQP searches illustrate stronger global joint-limit handling [S06], but are deferred to host-side comparison/fallback rather than adopted in the real-time MCU path. A closed-form solver is a later candidate only after actual geometry establishes its applicability. Do not assume a spherical wrist from six degrees of freedom.

## Requirements

### IK-001: Shared FK and model conformance
Phase: R1-IK; Issue: #4; Origin: derived
Requirement: FK shall use the canonical model/tool/base conventions and share test vectors with Motion Studio. The solver shall not embed unverified Helix dimensions or scene scale.
Acceptance: Synthetic reference chains, zero poses, random poses, transform composition and independent desktop calculations agree within declared numerical tolerances.

### IK-002: Verified geometric Jacobian
Phase: R1-IK; Issue: #4; Origin: derived
Requirement: The geometric Jacobian shall be computed from the actual chain in a declared frame, with transform reuse and no numerical finite-difference loop in production solely for convenience.
Acceptance: Central finite-difference checks over random/nontrivial configurations and several perturbation sizes validate translational and angular columns, signs and tool offsets.

### IK-003: Explicit full-pose objective
Phase: R1-IK; Issue: #4; Origin: derived
Requirement: A full-pose request shall retain both position and orientation goals, their tolerances and weighting. The solver shall not silently relax orientation to claim success near a singularity.
Acceptance: Test infeasible orientation, quaternion sign equivalence, near-zero and near-pi rotations and separated position/orientation residual reporting.

### IK-004: Robust bounded numerical step
Phase: R1-IK; Issue: #4; Origin: derived
Requirement: Numerical solving shall use bounded damping/step selection and checked factorization, reject nonfinite intermediates and enforce joint feasibility without assuming clipping solves constrained IK.
Acceptance: Near-singular, joint-bound, ill-conditioned and invalid-input fixtures produce a valid converged configuration or explicit failure without NaNs or large uncontrolled jumps.

### IK-005: Configuration continuity
Phase: R1-IK; Issue: #4; Origin: derived
Requirement: Nearby target solving shall honor allowed joint displacement, winding and configuration continuity relative to the trajectory seed. An alternative elbow/wrist branch shall require renewed path validation.
Acceptance: Continuous target sweeps and repeated boundary crossings never silently switch to an unapproved configuration; a rejected candidate cannot overwrite the seed.

### IK-006: Deadline and iteration bounds
Phase: R1-IK; Issue: #4; Origin: derived
Requirement: Each solve shall have both an absolute monotonic deadline and finite iteration/line-search caps. P-IK timing is a study target; acceptance requires measured worst-case atomic work and total elapsed time.
Acceptance: Fake-clock tests exhaust budgets at each boundary, then loaded-MCU tests verify no late result is published as executable.

### IK-007: Failure semantics
Phase: R1-IK; Issue: #4; Origin: derived
Requirement: Results shall distinguish invalid input/model, limit or continuity rejection, numerical failure, nonconvergence and deadline exhaustion. Nonconvergence shall not be labelled proof of global unreachability.
Acceptance: A categorized fixture suite exercises every status; failures leave admitted motion/reference state unchanged and return no permission to execute partial candidates.

### IK-008: Independent admission
Phase: R1-IK; Issue: #4; Origin: derived
Requirement: IK output shall pass trajectory continuity, dynamic-limit, configuration and preflight checks before actuation. Reaching a pose mathematically shall not authorize motion directly.
Acceptance: Valid IK endpoints connected by invalid/colliding/overspeed paths are rejected by the surrounding execution pipeline.

### IK-009: Measured numerical performance
Phase: R1-IK; Issue: #4; Origin: derived
Requirement: The implementation shall publish convergence distributions, worst-case timing, memory and residuals on a declared reachable-target corpus plus adversarial cases. A desktop benchmark shall not be used as an MCU rate guarantee.
Acceptance: Run deterministic seeded corpora on host and actual board under concurrent stepping/IO; explain failures and preserve reproducible raw measurements.

### IK-010: Optional capability until qualified
Phase: R1-IK; Issue: #4; Origin: derived
Requirement: R1 may execute host-generated trajectories without onboard IK. Cartesian commands shall remain unsupported until this capability's numerical and hardware timing gates pass.
Acceptance: A non-IK build rejects Cartesian solve commands while continuing valid joint-trajectory execution; feature discovery and release notes agree.
