# Platform section

Required behavior: [board and real-time architecture](../../../docs/requirements/02_board_realtime.md).

The abstract clock has no board implementation. Qualify timer wrap, coherent snapshots, reset epochs, peripheral ownership and DMA/cache interaction on the actual MCU. Host clocks and wall-clock changes do not define actuator timing.
