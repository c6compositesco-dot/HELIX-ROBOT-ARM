# Networked drive adapters

STATUS: not implemented. CAN FD is an investigation direction, not a qualified rate or selected vendor protocol.

Implement the shared `Drive` contract per selected protocol. Preserve supported modes, measured feedback sources, units, local watchdogs and timestamp capability. A position-only drive must reject torque requests. No motor/current loop is streamed over the network one PWM sample at a time.

Qualify one node before a mixed local/remote arm. Measure delivery skew, jitter, load and timeout behaviour. Node reboot, duplicate ID, bus-off, stale feedback and reconnect must not silently resume queued motion. Future brushless current/commutation loops belong in appropriate local servo drives, not TMC2209 sockets.
