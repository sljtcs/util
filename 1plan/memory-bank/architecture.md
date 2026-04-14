# Architecture Documentation

## Project Overview
A modern C++ utility library and experimental framework designed as a reusable toolkit for other C++ projects. Focuses on C++23 features, modular design, and reducing code duplication.

## Core Architecture Principles

### 1. **Modular Design**
- **util/**: Stable, production-ready utilities
- **src/**: Experimental features and modern C++ exploration
- **dTest/**: Test suite for utilities and experimental code
- **arch/**: Third-party library integrations

### 2. **Utility Categories**
- **Infrastructure**:File system, debugging, timing utilities
- **Modern C++**: C++23/26 features and patterns
- **Third-party**: Wrappers for common libraries
- **Testing**: Comprehensive test framework

### 3. **Layered Architecture**
```
Application Layer (User Projects)
    ↓
Utility Layer (util/*)
    ↓  
Infrastructure Layer (util/infra/*)
    ↓
Third-party Layer (arch/*)
```

## Module Responsibilities

### util/infra/
- **debug/**: Logging, timing, debugging macros
- **file/**: File system operations, path management
- **network/**: Network utilities (placeholder)

### src/feature/
- **cpp23/**: Modern C++23 implementations
- Experimental features and patterns

### dTest/
- Main test executable
- Integration testing for utilities
- Performance benchmarks

### arch/
- Third-party library configurations
- Build system integrations
- Dependency management

## Design Patterns
- **Command Pattern**: For operation-based utilities
- **Factory Pattern**: For utility creation
- **RAII**: For resource management
- **Modern C++ Features**: std::expected, concepts, ranges

## Build System
- CMake-based with C++23 standard
- Ninja generator for fast builds
- Modular target configuration
- Third-party dependency management