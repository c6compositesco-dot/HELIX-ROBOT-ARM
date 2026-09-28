# Qualification plan and evidence records

All campaigns below are planned, not reported as executed by this specification. Existing foundation host-test results remain in ../VALIDATION.md and must not be reused as proof of new runtime behavior.

## Gate sequence

| Gate | Work permitted / release result | Evidence required |
|---|---|---|
| Q0: requirements and host contracts | Parallel host design and tests, no hardware image | Requirements checker, negative checker tests, existing compiler/sanitizer matrix, implementation-gap review |
| Q1: board identity and recoverable non-actuating bring-up | Reviewed diagnostics, no motor enablement | Chip/clock/bootloader/pins, reset output observations, recovery route, map/stack/DMA audit |
| Q2: open-loop execution | Qualified R1 motion profile | Independent waveform capture, full-segment limits, horizon/overload/stop tests, live permissions, reference and tool behavior |
| Q3: optional onboard IK | Advertised R1-IK capability only | Independent FK/Jacobian vectors, constrained pose corpus, branch continuity, deadline injection, loaded-MCU timing |
| Q4: motor feedback | Observation, then separately approved correction | Scale/sign/reference/delay/noise, injected sensor faults, bounded error response, stability and saturation evidence |
| Q5: networked drives | One node then mixed backend profile | Actual electrical/rate qualification, frame budget, clocks/skew, node/bus failures and recovery |
| Q6: output sensing | Qualified dual-feedback profile | Independent output metrology, timestamp alignment, flexible transmission dynamics and stable loop behavior |
| Q7: brushless profile | Qualified external servo integration | Electrical/thermal/regeneration/support evidence, loop ownership and shared backend conformance |
| Q8: useful cell task | Specified unattended-task envelope | Model-bound preflight, repeated observed outcomes, intervention/failure records and P-ENDURANCE criteria |

## Adversarial test families

Numerical/model: invalid floats and rotations; wrong handedness/units; joint winding; singular/near-bound targets; unreachable requests; mismatched tool and calibration; independent Jacobian perturbations; failed or delayed solver.

Motion/runtime: legal endpoints with illegal interiors; time regression/wrap; late producer; partial queue transaction; step quantization drift; simultaneous pulses; invalid start state; mode conflicts; stopping at envelope boundaries; unavailable actuation; gravity and carried objects.

Protocol/network: all byte values; truncation and framing loss; invalid lengths/CRC; duplicate IDs with identical/changed payload; lost acknowledgements; owner/session/epoch changes; partial-node reboot; duplicate node address; bus-off; stale feedback; bandwidth saturation.

Feedback/tool/power: count scaling/wrap alias; frozen/noisy/reversed sensor; saturation and accumulated error; grip command without grip; sensor open/short; early and severe thermal faults; supply/regeneration limits; power removal and retention failure. Physical experiments require reviewed low-energy fixtures and external support first.

## Evidence record shape

For each campaign record requirement IDs, source commit, compiler/options/dependencies, image/configuration fingerprints, board and peripherals, fixture/equipment calibration, preconditions, test inputs/seed, frozen pass criteria, raw captures, failures/interventions, outcome, reviewer and limitations. Separate planned, performed-pass and performed-fail. Never overwrite a failed run with a cleaned-up summary.

Timing reports include maximum observed elapsed time and test envelope plus analysis of untested worst cases; a maximum observation is not by itself a formal worst-case proof. Accuracy reports separate numeric residual, repeatability, systematic error, load deflection and reference uncertainty. Reliability reports include every intervention and operating condition; several successful demonstration cycles do not establish broad industrial reliability.

## Immediate implementation order

Finish #2 board/source audit while host work proceeds on #10 canonical model, #3 continuous trajectories/stopping design and #5 transactional protocol. #9 fault/power design is co-developed, not appended after motors move. #4 numerical work proceeds against synthetic/reference geometry until the actual model is audited. #6/#7/#8 remain capability-scoped upgrades; #11 maintains evidence across every phase.

Before release, resolve every relevant parameter register entry and replace study targets with configuration-specific evidence. No change here removes `HELIX_BUILD_BOARD_IMAGE` protection, merges the draft or flashes hardware.
