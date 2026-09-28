# Octopus Pro V1.1 board layer

The intended central controller across all planned drive generations. A motor or encoder upgrade is not, by itself, a reason to replace it. Keep platform boundaries for testing and measured future constraints.

STATUS: NOT IMPLEMENTED / NOT QUALIFIED. No startup code, linker script, GPIO writes, USB driver, pulse engine or flashable target is supplied by this foundation.

Before a board implementation: identify the fitted MCU and revision, bootloader offset, actual clock, recoverable existing image/configuration, exact V1.1 schematic and pin allocation, timer/DMA ownership, enable polarity, stop behaviour and gravity-load support. Audit memory regions and cache/DMA coherency on the actual MCU.

CAN FD capability of a candidate MCU does not qualify the fitted transceiver, connector, termination, harness or slip rings. Leave rate and pin assignments unspecified until verified. Do not infer six/twelve directly attachable encoders from peripheral counts alone.

`hardware_audit.example.json` is an incomplete checklist, never a deployable configuration. `HELIX_BUILD_BOARD_IMAGE=ON` deliberately fails configuration. There is no switch in this revision that enables hardware.
