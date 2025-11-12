# HELIX Robot Arm

> 3D-printed, belt-driven 6-axis arm targeting **2 kg payload** and **≤0.2 mm repeatability** for **<$400 USD**.

![hero](docs/hero.gif)

## Specs
| Metric | Target | Notes |
|---|---|---|
| Reach | ___ mm | J2/J3 geometry |
| Payload | 2 kg | at ___ mm reach |
| Repeatability | ≤0.2 mm | ISO 9283 method, see Tests |
| Max speed | ___ °/s | joint-limited |
| Controller | ___ | firmware ___ |
| Cost (BOM) | <$400 | v0.1 bill below |

## Status & Roadmap
Alpha. Next: J2/J3 gearbox rev, encoder option, auto-cal.  
- [ ] v0.1 motion demo  
- [ ] v0.2 encoder on pulleys  
- [ ] v1.0 kit & docs

## Bill of Materials
See [`docs/bom.csv`](docs/bom.csv). Highlight: NEMA __, belts GT2 6 mm (lengths ___, ___), bearings ___, rails ___.

## Print & Assembly
- Material: ___ (critical parts: ___), infill ___%, walls ___
- No supports on ___; press-fit tolerances: shaft +0.02 / bore −0.02
- Belt lengths & routing diagrams: `docs/assembly.md`

## Electronics & Firmware
- Controller: ___ (pinout in `docs/wiring.pdf`)
- Drivers: ___ microsteps ___
- Firmware: fork of ___ with **adjustable IK** (`/firmware/`)

## Quickstart
1. Clone repo; print parts in `stls/`.
2. Assemble per `docs/assembly.md`.
3. Wire per `docs/wiring.pdf`.
4. Flash firmware (`/firmware/README.md`).
5. Run `examples/first_move.gcode` (or `scripts/first_move.py`).

## Calibration & Tests
- Belt tension procedure
- Home offsets / tool-center point
- Repeatability test (method + CSV + plot in `tests/`)
- Known issues: belt slip at J2 > __Nm, mitigation ___

## Safety
Pinch points, high torque, mains isolation. Use at your own risk.

## License & Contributing
License: ___. PRs welcome (see `CONTRIBUTING.md`). FAQ in `docs/faq.md`.
