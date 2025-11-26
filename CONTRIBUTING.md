# Contributing to Maestro QDMI Device

Thank you for your interest in contributing to the Maestro QDMI Device! This document provides guidelines and instructions for contributing to the project.

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [Getting Started](#getting-started)
- [Development Workflow](#development-workflow)
- [Code Style and Formatting](#code-style-and-formatting)
- [Testing](#testing)
- [Submitting Changes](#submitting-changes)

## Code of Conduct

We are committed to providing a welcoming and inclusive environment. Please be respectful and professional in all interactions.

## Getting Started

### Setting Up Your Development Environment

1. **Fork the repository** on GitHub

2. **Clone your fork**:
   ```bash
   git clone https://github.com/YOUR_USERNAME/maestro-qdmi-device.git
   cd maestro-qdmi-device
   ```

3. **Add upstream remote**:
   ```bash
   git remote add upstream https://github.com/QoroQuantum/maestro-qdmi-device.git
   ```

4. **Install dependencies**:
   - CMake 3.19+
   - C++17 compatible compiler
   - clang-format (for code formatting)

5. **Build the project**:
   ```bash
   mkdir build && cd build
   cmake ..
   cmake --build .
   ```

## Development Workflow

1. **Create a feature branch**:
   ```bash
   git checkout -b feature/your-feature-name
   ```

2. **Make your changes**:
   - Write clear, concise commit messages
   - Keep commits focused and atomic
   - Add tests for new functionality

3. **Keep your branch up-to-date**:
   ```bash
   git fetch upstream
   git rebase upstream/main
   ```

4. **Test your changes** (see [Testing](#testing) section)

5. **Format your code** (see [Code Style](#code-style-and-formatting) section)

## Code Style and Formatting

We use **clang-format** to ensure consistent code formatting across the project.

### Formatting Your Code

Before committing, format your code with:

```bash
# Format a single file
clang-format -i path/to/your/file.cpp

# Format all source files
find src test -name '*.cpp' -o -name '*.hpp' -o -name '*.h' | xargs clang-format -i
```

### Checking Formatting

To check if your code is properly formatted without modifying files:

```bash
clang-format --dry-run --Werror path/to/your/file.cpp
```

### Code Style Guidelines

- Use **4 spaces** for indentation (no tabs)
- Maximum **100 characters** per line
- Use meaningful variable and function names
- Add comments for complex logic
- Follow the QDMI specification conventions

### C++ Best Practices

- Use modern C++17 features where appropriate
- Prefer RAII for resource management
- Use smart pointers instead of raw pointers when possible
- Mark functions `noexcept` where applicable
- Use `const` correctness

## Testing

### Running Tests

All changes should include appropriate tests. Run the test suite with:

```bash
cd build
ctest --output-on-failure
```

Or run tests directly:

```bash
./test/maestro_device_tests
```

### Writing Tests

- Add tests in the `test/` directory
- Use Google Test framework
- Follow the existing test structure
- Ensure tests are deterministic and isolated
- Test both success and failure cases

### Test Coverage

Aim for comprehensive test coverage of new functionality:
- Unit tests for individual components
- Integration tests for QDMI interface compliance
- Edge cases and error handling

## Submitting Changes

### Pull Request Process

1. **Ensure all tests pass**:
   ```bash
   cd build
   ctest --output-on-failure
   ```

2. **Verify code formatting**:
   ```bash
   find src test -name '*.cpp' -o -name '*.hpp' -o -name '*.h' | xargs clang-format --dry-run --Werror
   ```

3. **Push to your fork**:
   ```bash
   git push origin feature/your-feature-name
   ```

4. **Create a Pull Request** on GitHub:
   - Provide a clear description of the changes
   - Reference any related issues
   - Explain the motivation and context
   - Include screenshots/examples if applicable

### Pull Request Guidelines

- **Title**: Use a clear, descriptive title
- **Description**: Explain what changed and why
- **Small PRs**: Keep pull requests focused and reasonably sized
- **One feature per PR**: Don't combine unrelated changes
- **Update documentation**: Include relevant README or comment updates

### Review Process

- Maintainers will review your PR
- Address any feedback or requested changes
- Keep the discussion professional and constructive
- Once approved, a maintainer will merge your PR

## Reporting Bugs

When reporting bugs, please include:

- **Description**: Clear description of the issue
- **Steps to reproduce**: Minimal steps to reproduce the problem
- **Expected behavior**: What you expected to happen
- **Actual behavior**: What actually happened
- **Environment**: OS, compiler version, CMake version
- **Logs/Screenshots**: Any relevant error messages or screenshots

## Feature Requests

We welcome feature requests! Please:

- Check if the feature has already been requested
- Clearly describe the feature and its benefits
- Explain your use case
- Be open to discussion and alternative approaches

## Questions?

- Open a [GitHub Discussion](https://github.com/QoroQuantum/maestro-qdmi-device/discussions) for general questions
- Check existing issues and discussions first
- Join our community channels (if available)

## License

By contributing, you agree that your contributions will be licensed under the Apache License 2.0, the same license as the project.

---

Thank you for contributing to Maestro QDMI Device! 🚀
