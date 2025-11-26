# Maestro QDMI Device

[![Built and tested on Ubuntu](https://github.com/QoroQuantum/maestro_qdmi_device/actions/workflows/cmake-single-platform.yml/badge.svg)](https://github.com/QoroQuantum/maestro_qdmi_device/actions/workflows/cmake-single-platform.yml)

A high-performance QDMI (Quantum Device Management Interface) device implementation for the [Maestro quantum simulator](https://github.com/QoroQuantum/maestro). This project provides a standards-compliant interface for quantum circuit execution, enabling seamless integration with QDMI-compatible quantum development tools.

## Features

- **QDMI Compliance**: Fully implements the QDMI specification for quantum device interfaces
- **Cross-Platform**: Supports Linux and Windows platforms
- **High Performance**: Optimized C++ implementation for efficient quantum circuit simulation
- **Dynamic Loading**: Runtime library loading for flexible device management
- **Comprehensive Testing**: Extensive test suite using Google Test framework

## Prerequisites

- **CMake**: Version 3.19 or higher
- **C++ Compiler**:
  - GCC 9.0+ or Clang 10.0+ (Linux)
  - MSVC 2019+ (Windows)
- **C Compiler**: For the C interface
- **Git**: For cloning dependencies

### Dependencies

The following dependencies are automatically fetched during the build process:

- [QDMI](https://github.com/Munich-Quantum-Software-Stack/QDMI): Quantum Device Management Interface specification
- [Google Test](https://github.com/google/googletest): Testing framework (required only for tests)

## Building from Source

### Quick Start

```bash
# Clone the repository
git clone https://github.com/QoroQuantum/maestro-qdmi-device.git
cd maestro-qdmi-device

# Create build directory
mkdir build && cd build

# Configure and build
cmake ..
cmake --build .
```

### Build Options

- `CXX_DEVICE`: Build the C++ device implementation (default: `ON`)
- `BUILD_MAESTRO_DEVICE_TESTS`: Build test suite (default: `ON` when building as the main project)

#### Building without tests:

```bash
cmake .. -DBUILD_MAESTRO_DEVICE_TESTS=OFF
cmake --build .
```

#### Building with a specific compiler:

```bash
cmake .. -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang
cmake --build .
```

### Platform-Specific Notes

#### Linux

Ensure you have the necessary development tools installed:

```bash
# Ubuntu/Debian
sudo apt-get install build-essential cmake git

# Fedora/RHEL
sudo dnf install gcc gcc-c++ cmake git
```

#### Windows

Use Visual Studio with CMake support or install CMake separately. Build from Developer Command Prompt:

```cmd
cmake .. -G "Visual Studio 16 2019"
cmake --build . --config Release
```

## Running Tests

After building with tests enabled:

```bash
cd build
ctest --output-on-failure
```

Or run the test executable directly:

```bash
./test/maestro_device_tests
```

## Usage

The Maestro QDMI device is designed to be loaded dynamically by QDMI-compatible quantum development environments. The device implements the standard QDMI interface functions for:

- Device initialization and configuration
- Quantum circuit submission and execution
- Result retrieval and processing
- Device capability querying

For integration examples and API documentation, please refer to the [QDMI specification](https://github.com/Munich-Quantum-Software-Stack/QDMI).

## Project Structure

```
maestro-qdmi-device/
├── src/                    # Source files
│   ├── Library.h          # Dynamic library loading utilities
│   ├── MaestroLib.hpp     # Core Maestro library interface
│   ├── Simulator.hpp      # Quantum simulator implementation
│   └── maestro_device.cpp # QDMI device implementation
├── test/                   # Test suite
│   ├── maestro_test_defs.cpp
│   └── test_maestro_device.cpp
├── cmake/                  # CMake modules
├── CMakeLists.txt         # Main CMake configuration
├── LICENSE                # Apache 2.0 License
└── README.md             # This file
```

## Contributing

We welcome contributions! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines on:

- Setting up your development environment
- Code formatting standards
- Running tests
- Submitting pull requests

## License

This project is licensed under the Apache License 2.0 - see the [LICENSE](LICENSE) file for details.

Copyright 2025 Qoro Quantum Ltd.

## Related Projects

- [Maestro Quantum Simulator](https://github.com/QoroQuantum/maestro) - High-performance quantum circuit simulator
- [QDMI Specification](https://github.com/Munich-Quantum-Software-Stack/QDMI) - Quantum Device Management Interface standard
- [Munich Quantum Software Stack](https://github.com/Munich-Quantum-Software-Stack) - Comprehensive quantum software development ecosystem

## Acknowledgments

This project is part of the Munich Quantum Software Stack initiative, contributing to the development of open standards for quantum computing interfaces.

## Support

For questions, issues, or feature requests, please:

- Open an issue on [GitHub Issues](https://github.com/QoroQuantum/maestro-qdmi-device/issues)
- Check existing discussions in [GitHub Discussions](https://github.com/QoroQuantum/maestro-qdmi-device/discussions)

---

**Note**: This is a QDMI device implementation. For general quantum programming, you'll need a QDMI-compatible development environment or framework.
