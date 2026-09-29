# Protocol section

Required behavior: [host and network protocol](../../../docs/requirements/07_protocol_network.md) and [trust boundaries](../../../docs/requirements/09_release_observability.md).

CommandAdmission is an in-memory check, not a wire codec or atomic dispatcher. Queue reservation, sequence commitment, acknowledgement and retry handling must be designed as one transaction. CRC and sequence numbers are not authentication.
