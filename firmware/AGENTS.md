# Firmware contributor rules

- Preserve the Octopus central-controller requirement across planned drive generations.
- Keep the existing root binary unchanged; do not claim its provenance or rebuild it without corresponding source.
- Keep private CAD, dimensions, ratios, calibration, credentials and commercial documents out of this public repository unless explicitly authorized.
- No guessed GPIO, currents, bootloader offsets or motion limits. Incomplete board configuration must not produce a usable hardware image.
- State what is implemented versus an interface/stub. Unsupported hardware operations must fail, not simulate success.
- Keep commanded position, issued-step estimate, motor sensing and output sensing distinct. Missing measurements remain invalid.
- Real-time paths: bounded work/storage, explicit failure, no blocking I/O or allocation. Do not enable fast-math around finite-input checks.
- Do not run competing position loops. Each loop and timebase has one documented owner.
- Tests before commits: Debug and Release, GCC and Clang where available, plus ASan/UBSan. Add regression tests for every bug.
- Host tests are not hardware qualification. Require measured timing, fault injection, stop/power-loss behaviour and reviewed physical controls before powered release.
