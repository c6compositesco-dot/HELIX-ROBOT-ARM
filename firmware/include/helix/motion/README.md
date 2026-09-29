# Motion section

Required behavior: [continuous motion and STEP/DIR](../../../docs/requirements/04_motion_stepdir.md), with [fault/support requirements](../../../docs/requirements/05_runtime_safety_power.md).

The existing queue validates individual points only. Complete the continuous segment format, interior-extrema checks, coordinated interpolation, stopping horizon and pulse-engine qualification before treating it as a motion runtime.
