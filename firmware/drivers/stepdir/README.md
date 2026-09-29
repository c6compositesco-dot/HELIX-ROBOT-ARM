# Local STEP/DIR and TMC2209 adapter

STATUS: interface only; use `UnavailableDrive` until implemented. No pulses or UART traffic are generated.

Own motor-unit conversion, driver current/microstep configuration, pulse timing and issued-step accounting here. Robot-side interfaces stay in radians. An issued-step estimate must not populate motor/output encoder measurements.

Next implementation gate: recover exact configuration; qualify one unloaded channel; prove pulse/enable timing with a logic analyser; then coordinated multi-axis load, buffer underrun, reset and fault tests. Test stopping/braking separately from logic-state changes. No default currents, ratios, pin numbers or travel limits are guessed.
