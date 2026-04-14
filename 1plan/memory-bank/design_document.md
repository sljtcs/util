# Design Document

## Design Philosophy

### Core Principles
1. **Vibe-Coding Aesthetic**: Clean, intuitive API design with modern C++ practices
2. **Zero Overhead**: Utilities should add minimal runtime cost
3. **Header-Only Preference**: Where possible, utilities should be header-only for easy integration
4. **Fail-Safe**: Comprehensive error handling using modern C++ features
5. **Explicit > Implicit**: Clear interfaces, no hidden magic

## Design Goals

### 1. **Developer Experience**
- **Intuitive APIs**: Self-documenting code names
- **Compile-Time Safety**: Leverage type system and concepts
- **Zero Configuration**: Works out of the box
- **Clear Documentation**: Inline comments and examples

### 2. **Performance**
- **Zero Abstraction Penalty**: Templates and constexpr for performance
- **Minimal Dependencies**: Reduce compilation time
- **Cache-Friendly**: Data-oriented design where appropriate

### 3. **Maintainability**
- **Test Coverage**: Comprehensive tests in dTest/
- **Modular**: Each utility independent and reusable
- **Future-Proof**: Prepare for C++26 and beyond

## Utility Design Patterns

### Error Handling
- Use `std::expected<T, E>` for recoverable errors
- Use `std::optional<T>` for optional values
- Use `assert()` and debug logging for programming errors

### Resource Management
- RAII for all resource types
- Smart pointers for ownership
- Value semantics for simple types

### API Design
```cpp
// Good: Explicit, clear, type-safe
auto result = parse_int(input);
if (result) {
    use_value(*result);
} else {
    handle_error(result.error());
}

// Avoid: Ambiguous, implicit, error-prone
auto value = parse_int(input); // What happens on error?
```

## Code Style Guide

### Naming Conventions
- **Classes**: PascalCase (e.g., `FileSystem`, `Logger`)
- **Functions**: camelCase (e.g., `parseFile`, `logMessage`)
- **Variables**: camelCase (e.g., `fileName`, `bufferSize`)
- **Constants**: UPPER_SNAKE_CASE (e.g., `MAX_BUFFER_SIZE`)
- **Namespaces**: lowercase (e.g., `infra_file`, `debug_util`)

### File Organization
- **Headers**: `.hpp` for headers, `.cpp` for implementations
- **One Concept Per File**: Keep files focused and small
- **Include Guards**: Use `#pragma once`

### Comments
- **Public APIs**: Document with clear comments
- **Complex Logic**: Explain reasoning
- **Avoid Obvious Comments**: Code should be self-documenting

## Third-Party Integration Strategy

### Wrapper Guidelines
- **Thin Wrappers**: Minimal abstraction layer
- **Native Feel**: Make foreign APIs feel like C++
- **Optional**: Users can choose to use original library
- **Version-Agnostic**: Work across reasonable version ranges