# Communication Protocol Specification

## Overview

The QML Remote Server uses a custom binary protocol over SLIP (Serial Line Internet Protocol) framing for reliable communication between embedded devices and the QML interface. This document describes the complete protocol specification, including packet structure, command types, and data encoding.

## SLIP Framing (RFC 1055)

All data packets are encoded using SLIP protocol for reliable transmission over serial and TCP connections.

### SLIP Constants

| Constant | Value | Description |
|----------|-------|-------------|
| END      | 0xC0  | Frame boundary marker |
| ESC      | 0xDB  | Escape character |
| ESC_END  | 0xDC  | Escaped END character |
| ESC_ESC  | 0xDD  | Escaped ESC character |

### SLIP Encoding Rules

1. Each packet begins and ends with END (0xC0)
2. If data contains END (0xC0), replace with ESC ESC_END (0xDB 0xDC)
3. If data contains ESC (0xDB), replace with ESC ESC_ESC (0xDB 0xDD)

### Example SLIP Encoding

```plaintext
Original data: [0x01, 0x02, 0xC0, 0x03]
SLIP encoded:  [0xC0, 0x01, 0x02, 0xDB, 0xDC, 0x03, 0xC0]
```

## Packet Structure

All packets follow this basic structure:

```plaintext
┌─────────────┬─────────────┬────────────────┬─────────────┐
│ Command     │ Property    │ Data           │ SLIP        │
│ Type (1B)   │ ID (1B)     │ (Variable)     │ Framing     │
└─────────────┴─────────────┴────────────────┴─────────────┘
```

## Command Types

### 0x00 - Get Property List

Requests the server to send the complete list of available properties.

**Format:**

```plaintext
[0x00, 0x00]
```

**Response:**
The server responds with a special packet containing the property list encoded in CBOR:

```plaintext
[0xFF, 0xFF, <CBOR_DATA>]
```

**CBOR Format:**

The CBOR data is a map where each key is the property name and the value is a map with keys `id` and `type`:

- Key: property name (string)
- Value: map with keys:
  - `id`: integer property ID
  - `type`: string type name

**Example (CBOR as diagnostic notation):**

```cbor-diag
{
  "temperature": {"id": 1, "type": "float"},
  "pressure": {"id": 2, "type": "float"},
  "pump1": {"id": 3, "type": "bool"},
  "tank_level": {"id": 4, "type": "int"}
}
```

> Note: The actual data is binary CBOR, not JSON. Use a CBOR decoder on the client side.

### 0x01 - Set Integer Property

Sets an integer (32-bit) property value.

**Format:**

```plaintext
[0x01, PROPERTY_ID, VALUE_BYTES(4)]
```

**Data Encoding:**

- Little-endian 32-bit signed integer
- Range: -2,147,483,648 to 2,147,483,647

**Example:**

```plaintext
Set property ID 4 to value 75:
[0x01, 0x04, 0x4B, 0x00, 0x00, 0x00]
```

### 0x02 - Set Float Property

Sets a floating-point (32-bit) property value.

**Format:**

```plaintext
[0x02, PROPERTY_ID, VALUE_BYTES(4)]
```

**Data Encoding:**

- Little-endian IEEE 754 single-precision float
- Range: ±1.175e-38 to ±3.403e+38

**Example:**

```plaintext
Set property ID 1 to value 23.5:
[0x02, 0x01, 0x00, 0x00, 0xBC, 0x41]
```

### 0x03 - Set String Property

Sets a string property value.

**Format:**

```plaintext
[0x03, PROPERTY_ID, STRING_BYTES...]
```

**Data Encoding:**

- UTF-8 encoded string
- No null terminator required
- Maximum length limited by packet size

**Example:**

```plaintext
Set property ID 5 to "ALARM":
[0x03, 0x05, 0x41, 0x4C, 0x41, 0x52, 0x4D]
```

### 0x04 - Set Boolean Property

Sets a boolean property value.

**Format:**

```plaintext
[0x04, PROPERTY_ID, VALUE_BYTE]
```

**Data Encoding:**

- 1 byte: 0x00 = false, 0x01 = true
- Any non-zero value is treated as true

**Example:**

```plaintext
Set property ID 3 to true:
[0x04, 0x03, 0x01]
```

### 0x05 - Invoke Method

Invokes a method (function) on the QML object.

**Format:**

```plaintext
[0x05, METHOD_ID]
```

**Note:**

- Methods are mapped using the same ID system as properties
- No parameters are currently supported
- Method execution is synchronous

**Example:**

```plaintext
Invoke method ID 6:
[0x05, 0x06]
```

### 0xFF - Heartbeat

Heartbeat packet sent periodically by the server to maintain connection.

**Format:**

```plaintext
[0xFF]
```

**Behavior:**

- Sent every 5 seconds by default
- Used for connection health monitoring
- Clients should ignore or acknowledge these packets

## Property ID Mapping

Properties are automatically assigned sequential IDs starting from 1 when the QML file is loaded. The mapping is consistent for the lifetime of the server session.

### Property Discovery Sequence

1. Client sends: `[0x00, 0x00]` (Get Property List)
2. Server responds with property list encoded in CBOR
3. Client uses the received IDs for subsequent property updates

### Example Property Mapping

| Property Name | ID | Type   | Description |
|---------------|-----|--------|-------------|
| temperature   | 1   | float  | Temperature in °C |
| pressure      | 2   | float  | Pressure in hPa |
| pump1         | 3   | bool   | Pump 1 state |
| tank_level    | 4   | int    | Tank level percentage |
| status        | 5   | string | System status message |
| reset         | 6   | method | Reset system function |

## Communication Examples

### Complete Session Example

1. **Client connects and requests property list:**

   ```plaintext
   SLIP: [0xC0, 0x00, 0x00, 0xC0]
   ```

2. **Server responds with property list:**

   ```plaintext
   SLIP: [0xC0, 0xFF, 0xFF, <CBOR_DATA>, 0xC0]
   ```

   Where `<CBOR_DATA>` is a CBOR-encoded map as described above.

3. **Client sets temperature to 25.5°C:**

   ```plaintext
   Raw:  [0x02, 0x01, 0x00, 0x00, 0xCC, 0x41]
   SLIP: [0xC0, 0x02, 0x01, 0x00, 0x00, 0xCC, 0x41, 0xC0]
   ```

4. **Client enables pump1:**

   ```plaintext
   Raw:  [0x04, 0x03, 0x01]
   SLIP: [0xC0, 0x04, 0x03, 0x01, 0xC0]
   ```

5. **Server sends heartbeat:**

   ```plaintext
   SLIP: [0xC0, 0xFF, 0xC0]
   ```

## Error Handling

### Invalid Property ID

- Server ignores packets with unknown property IDs
- No error response is sent

### Type Mismatches

- Server attempts type conversion when possible
- Invalid conversions are ignored

### Malformed Packets

- Packets with insufficient data are ignored
- SLIP decoding errors result in packet discard

### Connection Issues

- Missing heartbeats indicate connection problems
- Automatic reconnection should be implemented by clients

## Implementation Notes

### Endianness

All multi-byte values use little-endian byte order for consistency across platforms.

### Data Types

- **int**: 32-bit signed integer
- **float**: IEEE 754 single-precision floating-point
- **bool**: Single byte (0x00/0x01)
- **string**: UTF-8 encoded text

### Performance Considerations

- Property updates are processed immediately
- No buffering or batching is performed
- High-frequency updates may impact QML rendering performance

### Security

This protocol is designed for trusted environments and does not include:

- Authentication mechanisms
- Encryption
- Access control
- Input validation beyond basic type checking
