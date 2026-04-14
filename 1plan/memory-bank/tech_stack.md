# Technology Stack

## Core Technologies

### Build System
- **CMake** (v3.31+) - Cross-platform build system
- **C++23 Standard** - Modern C++ features and syntax
- **Ninja Generator** - Fast, incremental builds

### Testing Framework
- **doctest** - Lightweight, fast C++ testing framework
  - Header-only library
  - Fast compilation and execution
  - BDD-style test syntax support

### Third-Party Libraries
- **OpenCV** (v4.8.0) - Computer vision and image processing
- **RTAudio** - Real-time audio I/O
- **Lua** - Scripting language integration
- **libcurl** - HTTP client and file transfer
- **doctest** - Testing framework (currently active)

### Development Tools
- **.clangd** - Language Server Protocol implementation for C++
- **VSCode** - Primary development environment
- **Git** - Version control

## Language Features

### Modern C++ Standards
- **C++23** (Primary target)
  - `std::expected<T, E>` for error handling
  - `std::optional<T>` for optional values
  - Concepts for template constraints
  - Ranges library integration
  - `std::format` for string formatting

### C++ Features In Use
- **Templates** - Generic programming
- **Constexpr** - Compile-time evaluation
- **RAII** - Resource management
- **Smart Pointers** - Automatic memory management
- **Move Semantics** - Efficient value passing
- **Lambda Expressions** - Functional programming

## Platform Support

### Primary Target Platforms
- **Windows** (Primary development platform)
  - MSVC compiler support
  - Visual Studio integration

### Secondary Target Platforms
- **Linux** (Planned)
  - GCC/Clang compiler support
  - POSIX compatibility layer

### Cross-Platform Design
- Standard library focus for portability
- Platform-specific code isolated
- CMake handles platform differences

## Development Workflow

### Build Process
```bash
# Configure project
cmake -S . -B build -G Ninja

# Build project
cmake --build build --config Release
```

### Testing Process
- Unit tests for individual utilities
- Integration tests for module interactions
- Performance benchmarks for critical paths

### Code Quality
- Static analysis via clangd
- Compile-time type checking
- Runtime assertions in debug builds
- Comprehensive test coverage

## Dependency Management

### Current Approach
- Manual third-party library configuration
- Path-based includes and linking
- Version-specific configurations

### Planned Improvements
- Package manager integration (vcpkg/Conan)
- Automatic dependency resolution
- Version compatibility checks

## Performance Considerations

### Zero-Overhead Philosophy
- Header-only implementations where possible
- Compile-time optimization
- Minimal runtime abstraction

### Memory Management
- Stack allocation preference
- Smart pointers for heap allocation
- RAII for resource cleanup
- Move semantics to avoid copies

### Threading Support
- std::thread for basic concurrency
- Future and async patterns (planned)
- Lock-free data structures (where applicable)

## Documentation Standards

### Code Documentation
- Inline comments for complex logic
- Doxygen-style API documentation (planned)
- Usage examples in test files
- Design rationales in comments

### Project Documentation
- Architecture documentation
- Design documents
- Implementation plans
- Progress tracking

## Security Considerations

### Current Practices
- Input validation in file operations
- Error handling with std::expected
- Bounds checking in debug builds
- Safe string operations

### Future Enhancements
- Security audit of all utilities
- Fuzzing integration
- Static analysis tools
- Security best practices documentation

## Future Technology Plans

### C++26 Exploration
- Monitor standard committee progress
- Prepare for new language features
- Experimental implementations

### Additional Libraries
- Database integration (SQLite wrapper)
- JSON/XML parsing utilities
- Cryptographic primitives
- Regular expression utilities