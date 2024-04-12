# UDP Packet Forwarder

This project implements a UDP packet forwarder using the ASIO library, simulating NAT behavior for packet forwarding. It's designed with multi-user support and is cross-platform, functioning seamlessly across all major operating systems and architectures.

## Features✨

- Facilitates UDP packet forwarding, akin to NAT behavior.
- Supports multiple users concurrently.
- Cross-platform compatibility: Works on Windows, Linux, and macOS.
- Leverages the ASIO library for high-performance asynchronous I/O operations.
- Development environment requirements:
  - Visual Studio Code
  - Git
  - CMake、Ninja
  - For Windows compilation: Visual Studio 2017 or newer
  - For Linux compilation: Docker

## Quick Start🚀

### Clone the Repository

```bash
git clone https://github.com/iGwkang/udp_forward.git
cd udp_forward
```

### Compilation

#### Using VSCode (Recommended for Development)
1. Install the `C/C++`, `CMake`, `clangd`, and `Dev Containers` extensions in VSCode.
2. For Windows compilation, select the Visual Studio toolchain in the CMake extension.
   For Linux compilation, press F1 in VSCode, select "Reopen in Container," then choose the GCC toolchain in the CMake extension.
   Finally, initiate the build process.

#### Using Command Line
```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

## Usage Example💬

1. Configure forwarding parameters following the format specified in `config/forward.ini`.
```
  [Forward]
  10000=10.10.1.1:12000
  10001=10.10.1.2:12000
  10002=10.10.1.3:12000
```

2. Run `udp_forward` with the command `./udp_forward -c config/forward.ini`.
   This will cause `udp_forward` to listen on `0.0.0.0:10000` and forward packets to `10.10.1.1:12000`.

## Contribution

Contributions via issues or pull requests to enhance this project are welcomed.

## License

[MIT License](LICENSE)
