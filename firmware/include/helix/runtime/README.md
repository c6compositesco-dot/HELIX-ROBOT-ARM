# Runtime section

Required behavior: [lifecycle, faults and power](../../../docs/requirements/05_runtime_safety_power.md), [real-time architecture](../../../docs/requirements/02_board_realtime.md), and [qualification](../../../docs/requirements/QUALIFICATION.md).

ExecutionGate currently models admission permission only. Physical stopping, maintained support, live interlocks, homing and recovery require separate implementation and evidence. Do not connect every fault to unconditional motor disable.
