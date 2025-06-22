# QML Remote Server

A Qt/QML-based server application that serves as a GUI frontend for embedded devices, exposing live properties through a standardized communication protocol.

## Purpose

This project enables embedded devices to expose their properties and state to a dynamic QML-based graphical interface. The server acts as a bridge between embedded systems and modern UI frameworks, allowing real-time visualization and control of device parameters without requiring the embedded device to handle complex GUI rendering.

### Key Features

- **Dynamic Property Discovery**: Automatically discovers and exposes QML properties for remote access
- **Real-time Updates**: Live synchronization of property values between embedded devices and GUI
- **Dual Communication**: Supports both serial (UART) and TCP/IP communication channels
- **SLIP Protocol**: Uses Serial Line Internet Protocol (RFC 1055) for reliable packet framing
- **Responsive Dashboard**: Modern, adaptive QML interface with smooth animations
- **Cross-platform**: Runs on Linux, Windows, and macOS

## Project Structure

```plaintext
qml-remoteserver/
├── CMakeLists.txt                 # Build configuration
├── README.md                      # This file
├── docs/
│   └── PROTOCOL.md               # Communication protocol specification
├── examples/
│   ├── dashboard.qml             # Example SCADA-style dashboard
│   ├── test_dashboard.py         # Python test client with smooth value generation
│   └── slip_processor.py         # Python SLIP protocol implementation
├── main.cpp                      # Application entry point
├── genericqmlbridge.h/.cpp       # Core bridge implementation
└── slipprocessor.h/.cpp          # C++ SLIP protocol implementation
```

## High-Level Architecture

```plaintext
┌─────────────────┐    SLIP/Serial     ┌──────────────────┐    Qt Signals     ┌─────────────────┐
│ Embedded Device │ ◄────────────────► │ GenericQMLBridge │ ◄───────────────► │   QML Engine    │
└─────────────────┘                    └──────────────────┘                   └─────────────────┘
                                               │                                       │
┌─────────────────┐    SLIP/TCP/IP             │                                       │
│ Python Client   │ ◄──────────────────────────┘                                       │
└─────────────────┘                                                                    │
                                                                                       ▼
                                                                              ┌─────────────────┐
                                                                              │  Dashboard UI   │
                                                                              │  - Temperature  │
                                                                              │  - Pressure     │
                                                                              │  - Pumps        │
                                                                              │  - Alarms       │
                                                                              └─────────────────┘
```

### Core Components

1. **GenericQMLBridge**: Central coordinator that:
   - Loads and manages QML files
   - Discovers QML properties dynamically
   - Handles communication protocols (Serial/TCP)
   - Processes incoming commands and updates properties

2. **SlipProcessor**: SLIP protocol implementation that:
   - Encodes/decodes data packets reliably
   - Handles escape sequences and framing
   - Ensures data integrity over unreliable channels

3. **QML Dashboard**: Example for responsive user interface that:
   - Displays live data with appropriate precision
   - Provides visual feedback and animations
   - Adapts to different screen sizes
   - Shows connection status and errors

## Usage

### Building the Project

```bash
mkdir build && cd build
cmake ..
make
```

### Running the Server

**For Serial Communication:**

```bash
./appqml-remoteserver examples/dashboard.qml --port /dev/ttyUSB0 --baudrate 115200
```

**For TCP Communication:**

```bash
./appqml-remoteserver examples/dashboard.qml --tcp 8080
```

### Testing with Python Client

```bash
cd examples
python3 test_dashboard.py --host localhost --port 8080
```

## Communication Protocol

The system uses a custom protocol over SLIP framing for reliable communication. See [PROTOCOL.md](docs/PROTOCOL.md) for detailed specification.

### Supported Property Types

- `bool`: Boolean values (ON/OFF states)
- `int`: Integer values (levels, counts)
- `float/double`: Floating-point values (temperatures, pressures)
- `string`: Text values (status messages)

## Use Cases

- **Industrial Automation**: Monitor PLCs, sensors, and actuators
- **IoT Dashboards**: Visualize sensor data from microcontrollers
- **Testing Equipment**: Control and monitor test instruments
- **Embedded Prototyping**: Rapid GUI development for embedded projects
- **Remote Monitoring**: Real-time visualization of distributed systems

## Requirements

- Qt 6.5+ with QML support
- CMake 3.16+
- C++17 compatible compiler
- Python 3.6+ (for test clients)

## License

This project is provided as-is for educational and development purposes.
