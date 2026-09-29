# Kinematics section

Required behavior and numerical acceptance: [kinematics specification](../../../docs/requirements/03_kinematics.md). Compare geometry-specialized candidates using [IK-011](../../../docs/requirements/METHODS.md).

The current solver is an unavailable interface, not implemented IK. Begin with canonical-model/FK tests, independent Jacobian checks and bounded full-pose solving. No private geometry or assumed MCU timing belongs in public fixtures.
