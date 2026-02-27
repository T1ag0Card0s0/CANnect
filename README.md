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

- **Run cannect listenning on device vcan0 with can-ts protocol and address 65**
  ```sh
  ./build/cannect --can-iface vcan0 --protocol cants --addr 65
  ```

- **Show all available options:**
  ```sh
  ./build/cannect --help
  ```
### To create a virtual CAN interface
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
