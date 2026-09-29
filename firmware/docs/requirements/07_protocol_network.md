# Host protocol, transactional admission and drive networking

Sources: [S11], [S14], [S15], [S16], [S17], [S24], [S25]. Status: specification-only. Issues #5 and #7. Current `CommandAdmission` is not a transport, wire codec, authentication layer or complete transaction manager.

## Host protocol design direction

Use bounded binary frames over the local USB connection. Candidate framing is COBS with a zero delimiter [S24], plus an explicitly specified CRC-32C implementation [S25]. CRC detects corruption; it does not authenticate the sender. Specify endianness and every field, rather than transmitting C++ object memory. Bound encoded/decoded sizes, assembly timeout, work per parser invocation and outstanding requests. P-FRAME is a proposed implementation limit to be finalized through tests.

A versioned envelope should carry message kind, length, boot/session identity, command ID/sequence, model/configuration fingerprint, expiry/application timing and payload integrity. Exact field widths, CRC coverage and packet IDs belong in a reviewed wire-ABI document with byte-level test vectors before implementation; this baseline deliberately does not invent compatibility with the old G-code binary.

Admission is a transaction: decode and validate -> check current session/ownership/configuration -> reserve all required resources -> atomically commit queue data and command identity -> acknowledge. A full queue must not consume a command ID that the host cannot safely retry. Duplicate identical requests return the recorded outcome without executing again; duplicate IDs with changed payload are rejected. After reboot or uncertain physical outcome, query state and recover rather than promise exactly-once physical execution across arbitrary failures.

## Host requirements

### API-001: Bounded framing and codec
Phase: R1; Issue: #5; Origin: derived
Requirement: The wire protocol shall define byte order, finite field representations, lengths, integrity coverage, version and framing. Oversized, malformed or incomplete frames shall be rejected within bounded memory/work.
Acceptance: Byte-exact golden vectors and fuzz tests cover every byte value, truncation, delimiter loss, length overflow, invalid floats and corrupt checksums; recovery accepts the next valid frame without executing malformed data.

### API-002: Explicit compatibility negotiation
Phase: R1; Issue: #5; Origin: derived
Requirement: Connection setup shall negotiate protocol version, actual capabilities, limits and configuration identity. Unsupported major versions and incompatible motion formats shall fail before actuation.
Acceptance: Old/new host fixtures exercise accepted and rejected combinations; no silent interpretation of legacy G-code as the new protocol occurs.

### API-003: Atomic admission and acknowledgement
Phase: R1; Issue: #5; Origin: derived
Requirement: Command identity consumption, resource reservation, multi-axis queue insertion and acknowledgement shall have a documented atomic transaction boundary. Partial admission shall not produce physical side effects.
Acceptance: Fault injection at every transaction boundary, queue-full tests and retries preserve a single coherent accepted/rejected outcome without duplicate motion.

### API-004: Idempotent retries and uncertain outcomes
Phase: R1; Issue: #5; Origin: derived
Requirement: Identical retransmissions within the supported session window shall not execute twice. Changed payload under the same command ID shall be rejected. Unknown outcomes after reset shall be reported as unknown, not assumed unexecuted.
Acceptance: Drop acknowledgements, duplicate frames, reboot mid-command and replay IDs with altered payload; the host resolves uncertainty through state/recovery rather than blind retry.

### API-005: Single owner and fresh session
Phase: R1; Issue: #5; Origin: derived
Requirement: Motion/configuration authority shall use a single explicit owner/session. Reconnect or ownership transfer shall revoke old permission and invalidate incompatible queued commands.
Acceptance: Two clients, stale sessions and delayed old commands cannot interleave actuator control or automatically resume motion.

### API-006: Time and expiry semantics
Phase: R1; Issue: #5; Origin: derived
Requirement: Command expiry and scheduled execution shall use a declared controller-time mapping with bounded uncertainty, future horizon and reset epoch. Host wall-clock jumps shall not change active motion time.
Acceptance: Drift, delayed packets, future-dated floods and reset epochs are tested against P-CLOCK and P-HORIZON limits; invalid timing cannot refresh authority.

### API-007: Meaningful heartbeat
Phase: R1; Issue: #9; Origin: derived
Requirement: Only valid authorized health/lease messages shall refresh the supervisor timeout. Malformed, stale or unrelated traffic shall not keep a failed owner alive.
Acceptance: Saturate the link with invalid and read-only traffic while withholding valid heartbeat; the configured P-HEARTBEAT reaction still occurs.

### API-008: Backpressure and priority
Phase: R1; Issue: #5; Origin: derived
Requirement: Telemetry, configuration transfer and diagnostics shall have bounded bandwidth/storage and shall not starve motion admission or fault handling. Stop requests remain subject to local deterministic reaction, not arbitrary host queue delay.
Acceptance: Flood telemetry/configuration requests and measure critical-path latency; dropped optional telemetry is counted rather than blocking execution.

### API-009: Honest task/command status
Phase: R1; Issue: #5; Origin: derived
Requirement: Status shall distinguish received, admitted, executing, command-complete, physically verified, failed, cancelled and outcome-unknown where applicable. A receipt acknowledgement shall not imply task success.
Acceptance: The host displays correct states for lost grip, aborted trajectories, open-loop completion and unavailable feedback.

## Network design direction

Investigate CAN FD with qualified transceivers and wiring, not a new proprietary electrical bus [S16]. The Octopus remains coordinator; an interface addition is allowed if needed. Start with one node, then mixed local/remote drives. Use standard physical/link layers and a documented adapter for the selected drive application protocol. CAN FD, CANopen/CiA 402, moteus and ODrive CANSimple are not interchangeable names [S11, S14, S15].

Illustrative lower bound: six axes exchanging 32 command bytes plus 32 feedback bytes at 1 kHz need 3.072 Mbit/s of payload alone. Actual sizing includes arbitration, headers, checksums, stuffing, idle time, retries, synchronization and diagnostics. Current ODrive CANSimple documentation retains payloads of at most eight bytes even when FD is supported [S14]; protocol choice changes the budget.

Investigate P-NETWORK initially at 250-500 Hz with local interpolation. Do not promise a specific bus rate or synchronized application where the actual drive lacks timestamp support. EtherCAT distributed clocks are a later interoperability option [S17], not a firmware-only promise for the stock Octopus connector.

### BUS-001: Qualified physical interface
Phase: R3; Issue: #7; Origin: derived
Requirement: The fitted transceiver, board routing, connector, termination, isolation/reference strategy, harness and slip-ring path shall be qualified for the selected nominal/data rates. MCU FDCAN capability alone shall not authorize FD operation.
Acceptance: Record actual parts/routing and measure signal/error behavior under representative length, loading, movement and electrical noise before enabling the bus profile.

### BUS-002: Real frame and latency budget
Phase: R3; Issue: #7; Origin: derived
Requirement: The bus budget shall include the selected protocol's real frame sizes, arbitration/priorities, retransmissions and worst-case interference. Payload bandwidth alone shall not establish schedulability.
Acceptance: Analytical budgeting and saturated-bus measurements agree within declared bounds; missed command/feedback deadlines invoke a defined reaction.

### BUS-003: Coordinated timebase
Phase: R3; Issue: #7; Origin: derived
Requirement: Network adapters shall either support qualified timestamped command application or explicitly declare and bound delivery/application skew. Clock error and sample-time mapping shall be included in P-SKEW.
Acceptance: Multi-node measurements include load, drift and synchronization loss; non-timestamped drives never advertise simultaneous scheduled application.

### BUS-004: Drive-local interpolation and watchdogs
Phase: R3; Issue: #7; Origin: derived
Requirement: Drive update and interpolation behavior shall be documented, including behavior when messages stop. Immediate local motor protection shall not depend on per-cycle central communication.
Acceptance: Disconnect the coordinator at varied times and verify drive-local timeout behavior and system-level coordination limits.

### BUS-005: Identity, boot and reconfiguration
Phase: R3; Issue: #7; Origin: derived
Requirement: Duplicate node IDs, wrong drive types, rebooted nodes and changed firmware/configuration shall be detected before coordinated operation. Discovery shall not enable torque or replay stale commands.
Acceptance: Substitute, duplicate, reboot and reconfigure nodes while disarmed and active; incompatible identity prevents/revokes operation.

### BUS-006: Bus faults and partial-axis failure
Phase: R3; Issue: #9; Origin: derived
Requirement: Bus-off, excessive error, stale required feedback and partial-axis failure shall trigger the qualified arm reaction. Automatic reconnection shall not imply automatic resumption.
Acceptance: Inject faults and measure remaining-node response, queued-data invalidation and recovery requirements without assuming the failed node obeys stop commands.

### BUS-007: Application-protocol adapters
Phase: R3; Issue: #7; Origin: derived
Requirement: Vendor register layouts, state machines, firmware versions and unit conversions shall stay behind the shared Drive interface. Compliance claims shall identify the actual implemented and tested profile.
Acceptance: Byte-level vendor fixtures and shared drive conformance tests pass; CAN use alone never produces a CANopen/CiA 402 certification claim.

### BUS-008: Retained coordinator with measured expansion
Phase: R3; Issue: #7; Origin: user
Requirement: Network expansion shall retain the Octopus as the central coordinator unless a separately documented measured limitation requires another decision. Additional interfaces shall have explicit resource ownership.
Acceptance: The design review accounts for timer/DMA/USB/bus coexistence and measured budgets before requesting replacement of the central board.
