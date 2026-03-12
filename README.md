# CANnect

A modern C++ framework for CAN (Controller Area Network) communication with protocol handling, frame dispatch across customizable interfaces.

## Overview

CANnect is a lightweight, modular C++ library designed to simplify CAN communication. It provides a flexible architecture for handling multiple CAN interfaces, implementing custom protocols, and processing CAN frames with filtering and logging support.

### Key Features

- **Multiple Interface Support**: Manage and dispatch frames across multiple CAN interfaces simultaneously
- **Protocol Abstraction**: Pluggable protocol handlers for implementing custom CAN protocols
- **Thread-Safe Dispatching**: Dedicated threads for each CAN interface with proper synchronization
- **Flexible Filtering**: Apply custom filters to CAN frames before processing
- **Comprehensive Logging**: Multi-sink logging system with console and file output
- **SocketCAN Integration**: Native support for Linux SocketCAN interfaces
- **CAN-TS Protocol Implementation**: Built-in support for CAN-TS protocol with block transfer capabilities

## Architecture

### Core Components

#### **Cannect** (`include/cannect/Cannect.hpp`)
The main application class that orchestrates CAN communication:
- Add CAN interfaces for communication
- Register frame handlers for protocol processing
- Configure filters for frame processing
- Control application lifecycle (start, stop, run)

#### **CanDispatcher** (`include/cannect/CanDispatcher.hpp`)
Manages frame routing and multi-threaded dispatching:
- Maintains a registry of CAN interfaces
- Associates handlers and filters with specific interfaces
- Spawns dedicated threads for each interface
- Routes received frames to registered handlers

#### **ICanInterface** (`include/cannect/ICanInterface.hpp`)
Abstract interface for CAN communication endpoints:
- Open/close interface connections
- Transmit CAN frames
- Receive CAN frames
- Query interface status

#### **SocketCanInterface** (`src/SocketCanInterface.hpp`)
Linux SocketCAN implementation:
- Interface with virtual CAN devices (e.g., `vcan0`)
- Send and receive CAN frames
- Full Linux socket-based CAN support

#### **CanTsProtocol** (`src/CanTsProtocol.hpp`)
CAN-TS protocol implementation:
- **Message Types**: TIMESYNC, UNSOLICITED, TELECOMMAND, TELEMETRY, SETBLOCK, GETBLOCK
- **Block Memory Transfer**: Support for transferring blocks of data up to 512 bytes
- **Request/ACK Handling**: Proper request acknowledgment mechanisms
- **Node Addressing**: CAN node communication with addresses (0-255)

#### **Logger** (`include/cannect/Logger.hpp`)
Singleton logging system with multiple output sinks:
- Debug, Info, Warning, Error logging levels
- Configurable log level at compile time (`LOG_LEVEL` macro)
- Multiple simultaneous log sinks (console, file, custom)
- Thread-safe logging

## Project Structure

```
.
├── include/cannect/           # Public headers
│   ├── Cannect.hpp           # Main application class
│   ├── CanDispatcher.hpp     # Frame dispatcher
│   ├── ICanInterface.hpp     # CAN interface abstraction
│   ├── ICanFrameHandler.hpp  # Frame handler interface
│   ├── ICanFrameTransmitter.hpp # Frame transmission interface
│   ├── ICanProtocol.hpp      # Protocol handler interface
│   ├── IFilter.hpp           # Frame filter interface
│   ├── ILogSink.hpp          # Log sink interface
│   ├── Logger.hpp            # Logging system
│   ├── LogSinks.hpp          # Built-in log sinks
│   ├── Status.hpp            # Status codes
│   └── Types.hpp             # Core data structures
├── src/                       # Implementation files
│   ├── Cannect.cpp
│   ├── CanDispatcher.cpp
│   ├── Logger.cpp
│   ├── SocketCanInterface.cpp
│   ├── CanTsProtocol.hpp/.cpp
│   └── main.cpp              # Example application
├── tests/                     # Unit tests
│   ├── Makefile
├── docs/                      # Documentation
├── Makefile                   # Build configuration
└── compile_commands.json      # Compilation database
```

## Building

### Prerequisites

- **C++ Compiler**: GCC with C++17 support
- **Linux System**: For SocketCAN support
- **Make**: GNU Make for building

### Build Commands

```bash
# Build the project
make

# Clean build artifacts
make clean
```

## Usage

### Basic Example

```cpp
#include "cannect/Cannect.hpp"
#include "cannect/Logger.hpp"
#include "cannect/LogSinks.hpp"
#include "SocketCanInterface.hpp"
#include "CanTsProtocol.hpp"

using namespace cannect;

int main()
{
    // Setup logging
    Logger::instance()->addSink(std::make_shared<ConsoleSink>());
    Logger::instance()->addSink(std::make_shared<FileSink>("log.txt"));

    // Create application
    Cannect app;

    // Create SocketCAN interface (virtual CAN device)
    auto iface = std::make_shared<SocketCanInterface>("vcan0");
    
    // Create protocol handler (CAN-TS protocol, node address 65)
    auto proto = std::make_shared<CanTsProtocol>(65);
    
    // Register callbacks for protocol messages
    proto->setSetBlockHandler(
        [](uint8_t from, uint8_t channel, std::vector<uint8_t> &request) -> bool {
            std::cout << "Received SETBLOCK from node " << from << "\n";
            return true;
        });
    
    // Configure frame transmitter
    proto->setFrameTransmitter(iface);

    // Register interface and handler
    app.addInterface(iface);
    app.addHandler(iface->getName(), proto);

    // Run application
    app.run();
    app.stop();
    
    return 0;
}
```

### Creating Custom CAN Interfaces

Implement the `ICanInterface` interface:

```cpp
class CustomCanInterface : public ICanInterface
{
public:
    Status open() override { /* implementation */ }
    Status close() override { /* implementation */ }
    Status receive(CanFrame &frame) override { /* implementation */ }
    Status transmit(const CanFrame &frame) override { /* implementation */ }
    std::string getName() override { return "custom"; }
    bool isClosed() const override { /* implementation */ }
};
```

### Creating Custom Protocol Handlers

Implement the `ICanFrameHandler` interface:

```cpp
class CustomProtocolHandler : public ICanFrameHandler
{
public:
    Status onFrame(const CanFrame &frame) override
    {
        // Process CAN frame
        return Status::SUCCESS;
    }
};
```

### Applying Filters

Implement the `IFilter` interface:

```cpp
class CustomFilter : public IFilter
{
public:
    bool accept(const CanFrame &frame) override
    {
        // Return true to accept frame, false to reject
        return frame.id < 0x100;
    }
};

// Add filter to interface
app.addFilter("vcan0", std::make_shared<CustomFilter>());
```

### Log Levels

- `Debug`: 0 - Detailed debug information
- `Info`: 1 - General information
- `Warning`: 2 - Warning messages
- `Error`: 3 - Error messages
- `Off`: 4 - Disable logging

Set via `LOG_LEVEL` compile-time macro.

## Future Enhancements

Potential improvements for future versions:
- Support for non-Linux CAN interfaces
- Additional protocol implementations (J1939, AUTOSAR, CANOpen)
- Performance metrics and monitoring
- Configuration file support
- Extended CAN FD support

## Contributing

When contributing to CANnect:
- Follow the existing code style
- Use C++17 features appropriately
- Maintain thread-safe implementations
- Add appropriate logging
- Include tests for new functionality

## License

MIT License
