# Communication Protocol Specification

## Overview

The QML Remote Server uses a binary protocol over SLIP (Serial Line Internet Protocol) with all payloads encoded in CBOR. Each packet starts with a single command or response byte, followed by a CBOR-encoded payload (if required). This document describes the protocol, command set, and data formats.

## SLIP Framing (RFC 1055)

All packets are framed using SLIP for reliable transmission over serial and TCP connections.

## Packet Structure

```plaintext
┌──────────────┬────────────────────┐
│ Command/Resp │ CBOR Payload       │
│   (1 byte)   │ (optional, varies) │
└──────────────┴────────────────────┘
```

- The first byte is always the command or response code.
- The rest of the packet is a CBOR-encoded object, array, or map, depending on the command.

## Command and Response Codes

| Code  | Name                        | Direction | Payload (CBOR)         | Description                                 |
|-------|-----------------------------|-----------|------------------------|---------------------------------------------|
| 0x01  | CMD_GET_PROPERTY_LIST       | C→S       | none                   | Request property list                       |
| 0x02  | CMD_SET_PROPERTY            | C→S       | map {id: value, ...}   | Set one or more properties by ID            |
| 0x03  | CMD_INVOKE_METHOD           | C→S       | [method_id, params[]]  | Invoke method with parameters               |
| 0x04  | CMD_HEARTBEAT               | C→S       | none                   | Heartbeat/keep-alive                       |
| 0x20  | CMD_WATCH_PROPERTY          | C→S       | [id, ...]              | Watch property IDs for change notifications |
| 0x81  | RESP_GET_PROPERTY_LIST      | S→C       | map {name: {id, type}} | Property list response                      |
| 0x82  | RESP_PROPERTY_CHANGE        | S→C       | map {id: value, ...}   | Notification of watched property changes    |

- C→S: Client to Server
- S→C: Server to Client

## Command Details

### CMD_GET_PROPERTY_LIST (0x01)

Request the list of available properties.

- **Packet:** `[0x01]`
- **Response:** `[0x81, <CBOR_MAP>]`

### RESP_GET_PROPERTY_LIST (0x81)

Response to CMD_GET_PROPERTY_LIST. Contains a CBOR map:

- **Format:** `[0x81, <CBOR_MAP>]`
- **CBOR_MAP Example:**

  ```cbor-diag
  {
    "temperature": {"id": 1, "type": "float"},
    "pressure": {"id": 2, "type": "float"},
    "pump1": {"id": 3, "type": "bool"},
    "tank_level": {"id": 4, "type": "int"},
    "setpoint": {"id": 5, "type": "int"}
  }
  ```

### CMD_SET_PROPERTY (0x02)

Set one or more properties by ID.

- **Packet:** `[0x02, <CBOR_MAP>]`
- **CBOR_MAP Example:** `{ 5: 42 }` (set setpoint to 42)

### CMD_INVOKE_METHOD (0x03)

Invoke a method by ID, with optional parameters.

- **Packet:** `[0x03, method_id, <CBOR_ARRAY>]`
- **CBOR_ARRAY Example:** `[param1, param2, ...]`

### CMD_WATCH_PROPERTY (0x20)

Request to watch one or more property IDs. The server will send notifications when any of these properties change.

- **Packet:** `[0x20, <CBOR_ARRAY>]`
- **CBOR_ARRAY Example:** `[5]` (watch setpoint)
- Sending a new list replaces the previous set of watched properties.

### RESP_PROPERTY_CHANGE (0x82)

Notification sent by the server when a watched property changes.

- **Packet:** `[0x82, <CBOR_MAP>]`
- **CBOR_MAP Example:** `{ 5: 43 }` (setpoint changed to 43)
- Multiple properties may be included if several change at once.

### CMD_HEARTBEAT (0x04)

Heartbeat/keep-alive packet.

- **Packet:** `[0x04]`

## Example Session

1. **Client requests property list:**

   ```plaintext
   SLIP: [0xC0, 0x01, 0xC0]
   ```

2. **Server responds with property list:**

   ```plaintext
   SLIP: [0xC0, 0x81, <CBOR_MAP>, 0xC0]
   ```

3. **Client requests to watch the setpoint property:**

   ```plaintext
   SLIP: [0xC0, 0x20, [5], 0xC0]
   ```

4. **Server notifies when setpoint changes:**

   ```plaintext
   SLIP: [0xC0, 0x82, {5: 44}, 0xC0]
   ```

5. **Client sets setpoint property:**

   ```plaintext
   SLIP: [0xC0, 0x02, {5: 45}, 0xC0]
   ```

6. **Heartbeat:**

   ```plaintext
   SLIP: [0xC0, 0x04, 0xC0]
   ```

## Implementation Notes

- All property and method names are strings in the property list, but all subsequent commands use property/method IDs (integers).
- All values and parameters are CBOR-encoded.
- The protocol is extensible: new commands/responses can be added.
- SLIP framing is required for all packets.
- No JSON is used; all data is CBOR.
- Property watching is dynamic: sending a new CMD_WATCH_PROPERTY replaces the previous set.
- RESP_PROPERTY_CHANGE is only sent for properties currently being watched.

## Error Handling

- Unknown commands are ignored.
- Malformed CBOR payloads are ignored.
- Unknown property IDs are ignored in CMD_SET_PROPERTY and CMD_WATCH_PROPERTY.
- No error responses are sent.

## Security

- No authentication or encryption is provided.
- Intended for use in trusted environments.

## Changelog

- 2025-06-22: Protocol enums updated to match implementation. All command/response codes now reflect the codebase.
