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
| 0x00  | CMD_GET_PROPERTY_LIST       | C→S       | none                   | Request property list                       |
| 0x10  | CMD_SET_PROPERTY            | C→S       | map {name: value, ...} | Set one or more properties                  |
| 0x05  | CMD_INVOKE_METHOD           | C→S       | [method_id, params[]]  | Invoke method with parameters               |
| 0xFF  | CMD_HEARTBEAT               | C→S       | none                   | Heartbeat/keep-alive                       |
| 0x80  | RESP_GET_PROPERTY_LIST      | S→C       | map {name: {id, type}} | Property list response                      |

- C→S: Client to Server
- S→C: Server to Client

## Command Details

### CMD_GET_PROPERTY_LIST (0x00)

Request the list of available properties.

- **Packet:** `[0x00]`
- **Response:** `[0x80, <CBOR_MAP>]`

### RESP_GET_PROPERTY_LIST (0x80)

Response to CMD_GET_PROPERTY_LIST. Contains a CBOR map:

- **Format:** `[0x80, <CBOR_MAP>]`
- **CBOR_MAP Example:**

  ```cbor-diag
  {
    "temperature": {"id": 1, "type": "float"},
    "pressure": {"id": 2, "type": "float"},
    "pump1": {"id": 3, "type": "bool"},
    "tank_level": {"id": 4, "type": "int"}
  }
  ```

### CMD_SET_PROPERTY (0x10)

Set one or more properties by name.

- **Packet:** `[0x10, <CBOR_MAP>]`
- **CBOR_MAP Example:** `{ "temperature": 25.5, "pump1": true }`

### CMD_INVOKE_METHOD (0x05)

Invoke a method by ID, with optional parameters.

- **Packet:** `[0x05, method_id, <CBOR_ARRAY>]`
- **CBOR_ARRAY Example:** `[param1, param2, ...]`

### CMD_HEARTBEAT (0xFF)

Heartbeat/keep-alive packet.

- **Packet:** `[0xFF]`

## Example Session

1. **Client requests property list:**

   ```plaintext
   SLIP: [0xC0, 0x00, 0xC0]
   ```

2. **Server responds with property list:**

   ```plaintext
   SLIP: [0xC0, 0x80, <CBOR_MAP>, 0xC0]
   ```

3. **Client sets properties:**

   ```plaintext
   SLIP: [0xC0, 0x10, <CBOR_MAP>, 0xC0]
   ```

4. **Client invokes method:**

   ```plaintext
   SLIP: [0xC0, 0x05, method_id, <CBOR_ARRAY>, 0xC0]
   ```

5. **Heartbeat:**

   ```plaintext
   SLIP: [0xC0, 0xFF, 0xC0]
   ```

## Implementation Notes

- All property and method names are strings.
- All values and parameters are CBOR-encoded.
- The protocol is extensible: new commands/responses can be added.
- SLIP framing is required for all packets.
- No JSON is used; all data is CBOR.

## Error Handling

- Unknown commands are ignored.
- Malformed CBOR payloads are ignored.
- Unknown property names are ignored in CMD_SET_PROPERTY.
- No error responses are sent.

## Security

- No authentication or encryption is provided.
- Intended for use in trusted environments.
