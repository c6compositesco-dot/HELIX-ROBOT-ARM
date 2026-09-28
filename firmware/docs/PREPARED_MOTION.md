# Prepared motion and the next qualification boundary

This host-only increment adds a required positive `preparation_lead_us` and live checks for detached prepared requests. No motor output, timer driver, physical stop, or target calibration is provided.

Admission checks the positive lead both before and after validation. Preparation refuses the start instant and any handoff with less than the configured lead. Equality to the positive lead is allowed. The configuration value must cover measured worst-case conversion, scheduling, blocking and uncertainty; the tests' 100 microseconds is synthetic, not a Helix setting.

`prepared_authorized(request, now)` services the lease and checks current session, model, exact retained request, Prepared disposition, and unexpired segment interval. `execution_authorized` also requires the segment's start time. Disarm, fault, session/model replacement, expiry and receipt eviction deny authorization. A forged or merely queued request is not authorized. Receipt capacity must retain every prepared/in-flight request. Eviction fails closed and is not a physical stopping policy.

These are serialized-owner predicates, not durable capabilities. The eventual runtime must check immediately before committing normal output, under the same ownership boundary, without an intervening yield or asynchronous revocation race. A true result does not approve collision clearance, pulse generation, tracking, or stopping. Normal permission revocation must transfer control to a separately qualified stopping/holding path. It must never be interpreted as an instruction to universally remove motor power under gravity.

## Stopping and continuation design

The current inbox is not a certified stopping horizon. Before actuation, the runtime must maintain a separately validated continuation and stop certificate tied to the boot/session, model, tool, payload, limits and current commanded state. The certificate includes a dynamically valid path to a supported rest state, with continuous state matching and a bounded execution end.

Let `H` be remaining validated executable time, `D` worst-case detection delay, `G` worst-case stop generation/preparation delay, `S` worst-case stop execution duration and `U` scheduling/clock uncertainty. Normal continuation requires a supported certificate and `H > D + G + S + U`. Checked arithmetic must reject overflow or missing budgets. Trigger stopping at or before the boundary, not after the queue empties. Updates must atomically replace both continuation and its stop certificate; rejection preserves the existing usable certificate. Every state admitted for execution must have a feasible stop, including nonzero velocity/acceleration, joint limits and payload effects.

The required measured values and actual stop generator are still absent. Therefore no normal actuator integration is released. Planned regressions cover late producer, missing next segment, short horizon, expiry during validation, boundary equality, clock regression, cancellation while prepared, receipt exhaustion, lost actuation and power loss. Physical gravity support, tool retention and reset response remain qualification gates.
