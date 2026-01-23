> [!WARNING]  
> This is still a work in progress project, not all of the features are implemented.

# CANnect

CANnect is a C++ toolkit for interacting with Controller Area Network (CAN) interfaces. It provides a modular command-line interface and core components for logging, dispatching, bridging, and handling CAN frames.

The main purpose of this project is to offer an easy-to-use tool for controlling and experimenting with CAN-based protocols, such as CAN-TS. CANnect is designed for flexibility and extensibility, helping developers build, monitor, and analyze CAN systems efficiently.

## Usage

### Building

To build CANnect, run:

```sh
make
```

The compiled binary will be located at `build/cannect`.

### Running

You can run CANnect with various options. Some common examples:

- **Listen to a CAN interface:**
  ```sh
  ./build/cannect --iface can0
  ```
  Replace `can0` with your CAN interface name.

- **Bridge two CAN interfaces:**
  ```sh
  ./build/cannect --iface-a can0 --iface-b can1
  ```

- **Use a specific protocol (e.g., CAN-TS):**
  ```sh
  ./build/cannect --iface can0 --protocol cants
  ```

- **Show all available options:**
  ```sh
  ./build/cannect --help
  ```

### Common Options
- `--iface`      : CAN interface name (e.g. can0, vcan0)
- `--iface-a`    : Source CAN interface (for bridge)
- `--iface-b`    : Destination CAN interface (for bridge)
- `--protocol`   : Protocol: raw | canopen | cants (default: raw)
- `--output`     : Output file (record)
- `--input`      : Input file (replay)
- `--filter`     : Frame filter expression
- `--decode`     : Decode frames using selected protocol
- `--id`         : CAN ID (hex)
- `--data`       : CAN data bytes (e.g. "DE AD BE EF")
- `--help`       : Show help message

For more advanced usage, combine options as needed.


```bash
#!/bin/bash
# Make sure the script runs with super user privileges.
[ "$UID" -eq 0 ] || exec sudo bash "$0" "$@"
# Load the kernel module.
modprobe vcan
# Create the virtual CAN interface.
ip link add dev vcan0 type vcan
# Bring the virtual CAN interface online.
ip link set up vcan0
```
cansend vcan0 123#00FFAA5501020304
