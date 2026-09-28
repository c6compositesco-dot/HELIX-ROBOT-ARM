# CAN transport boundary

STATUS: no CAN peripheral initialization, wire protocol, driver binding or FD-rate claim.

Keep electrical transport separate from drive semantics. Verify actual board interface first, then define frame budget, priorities, feedback age limits, timebase mapping and bounded clock error. Timestamped coordinated application is a negotiated drive capability, not an assumption.

Retain the Octopus coordinator. Add a qualified interface when needed. Transport changes must not leak vendor register layouts into tasks, IK or the robot joint model. EtherCAT or another later transport is a separate engineering decision, not implemented here.
