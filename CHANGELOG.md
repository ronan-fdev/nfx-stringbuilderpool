# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- NIL

### Changed

- NIL

### Deprecated

- NIL

### Removed

- NIL

### Fixed

- NIL

### Security

- NIL

## [1.0.0] - 2025-10-31 - Initial Release

### Added

- **StringBuilderPool**: String building with thread-safe pooling

  - Three-tier memory pooling architecture (thread-local → shared pool → dynamic allocation)
  - Small Buffer Optimization (256-byte stack buffer)
  - Zero-allocation path for strings < 256 bytes
  - Automatic buffer growth with 1.5× growth factor

- **DynamicStringBufferPool**: Internal shared buffer pool

  - Mutex-protected cross-thread buffer recycling
  - Configurable buffer limits and growth policies
  - Hit rate tracking and performance metrics
  - Memory statistics (allocations, deallocations, reuses, active buffers)

- **Build System**

  - CMake 3.20+ configuration with modular structure
  - Static and shared library targets
  - Cross-platform support (Linux GCC/Clang, Windows MinGW/Clang/MSVC)
  - Google Test integration for unit testing
  - Google Benchmark integration for performance benchmarking
  - Doxygen documentation generation support
  - CPack packaging for distribution (DEB, RPM, ZIP, WIX)
  - Installation support with CMake package config
  - Windows resource file with version information

- **Documentation**

  - README with feature overview
  - Detailed API documentation with Doxygen comments
  - Sample application demonstrating library usage
  - Build and installation instructions

- **Testing & Benchmarking**
  - Unit test suite
  - Performance benchmarks for all operations
  - Cross-compiler performance validation
