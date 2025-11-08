# nfx-stringbuilderpool

<!-- Project Info -->

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg?style=flat-square)](https://github.com/ronan-fdev/nfx-stringbuilderpool/blob/main/LICENSE.txt) [![GitHub release (latest by date)](https://img.shields.io/github/v/release/ronan-fdev/nfx-stringbuilderpool?style=flat-square)](https://github.com/ronan-fdev/nfx-stringbuilderpool/releases) [![GitHub tag (latest by date)](https://img.shields.io/github/tag/ronan-fdev/nfx-stringbuilderpool?style=flat-square)](https://github.com/ronan-fdev/nfx-stringbuilderpool/tags)<br/>

![C++20](https://img.shields.io/badge/C%2B%2B-20-blue?style=flat-square) ![CMake](https://img.shields.io/badge/CMake-3.20%2B-green.svg?style=flat-square) ![Cross Platform](https://img.shields.io/badge/Platform-Linux_Windows-lightgrey?style=flat-square)

<!-- CI/CD Status -->

[![Linux GCC](https://img.shields.io/github/actions/workflow/status/ronan-fdev/nfx-stringbuilderpool/build-linux-gcc.yml?branch=main&label=Linux%20GCC&style=flat-square)](https://github.com/ronan-fdev/nfx-stringbuilderpool/actions/workflows/build-linux-gcc.yml) [![Linux Clang](https://img.shields.io/github/actions/workflow/status/ronan-fdev/nfx-stringbuilderpool/build-linux-clang.yml?branch=main&label=Linux%20Clang&style=flat-square)](https://github.com/ronan-fdev/nfx-stringbuilderpool/actions/workflows/build-linux-clang.yml) [![Windows MinGW](https://img.shields.io/github/actions/workflow/status/ronan-fdev/nfx-stringbuilderpool/build-windows-mingw.yml?branch=main&label=Windows%20MinGW&style=flat-square)](https://github.com/ronan-fdev/nfx-stringbuilderpool/actions/workflows/build-windows-mingw.yml) [![Windows MSVC](https://img.shields.io/github/actions/workflow/status/ronan-fdev/nfx-stringbuilderpool/build-windows-msvc.yml?branch=main&label=Windows%20MSVC&style=flat-square)](https://github.com/ronan-fdev/nfx-stringbuilderpool/actions/workflows/build-windows-msvc.yml) [![CodeQL](https://img.shields.io/github/actions/workflow/status/ronan-fdev/nfx-stringbuilderpool/codeql.yml?branch=main&label=CodeQL&style=flat-square)](https://github.com/ronan-fdev/nfx-stringbuilderpool/actions/workflows/codeql.yml)

> A C++20 library for zero-allocation string building with thread-safe pooling and Small Buffer Optimization

## Overview

**nfx-stringbuilderpool** is a modern C++20 library providing string building capabilities through a three-tier memory pooling system. Designed for applications requiring efficient string concatenation with minimal allocations, it features thread-local caching, cross-thread buffer sharing, and comprehensive performance monitoring.

## Features

### 🔄 Three-Tier Memory Pooling Architecture

- **Tier 1: Thread-Local Cache**
  - Zero synchronization overhead for single-threaded hotpaths
  - Immediate buffer availability with no locking
  - Perfect for high-frequency string operations
- **Tier 2: Shared Cross-Thread Pool**
  - Mutex-protected buffer sharing across threads
  - Size-limited to prevent memory bloat
  - Configurable pool size and retention limits
- **Tier 3: Dynamic Allocation**
  - Fallback when pools are exhausted
  - Pre-sized buffers for optimal performance
  - Automatic capacity management with 1.5x growth factor

### ⚡ Performance Optimized

- **Small Buffer Optimization (SBO)**: 256-byte stack buffer eliminates heap allocations for most strings
- **Zero-Copy Operations**: `string_view` access without allocation
- **High Cache Hit Rates**: 90%+ pool hit rate in typical workloads
- **Sub-Microsecond Operations**: Faster than `std::string` and `std::ostringstream`
- **Memory Efficiency**: Automatic buffer recycling and size management

### 🛠️ Rich String Building Interface

- **Fluent API**: Stream operators (`<<`) for natural concatenation
- **Type Support**: String, string_view, C-strings, characters, and custom types
- **Direct Buffer Access**: High-performance operations without wrappers
- **Iterator Support**: Range-based for loops and STL algorithms
- **RAII Lease Management**: Automatic cleanup and pool return

### 📊 Performance Monitoring

- **Built-in Statistics**: Track pool hits, misses, and allocations
- **Hit Rate Calculation**: Monitor pooling efficiency
- **Thread-Local Metrics**: Per-thread and global statistics
- **Zero Overhead**: Statistics can be disabled at compile time

### 🌍 Cross-Platform Support

- **Operating Systems**: Linux, Windows
- **Compilers**: GCC 14+, Clang 18+, MSVC 2022+
- **Thread-Safe**: All pool operations are thread-safe
- **Consistent Behavior**: Same performance characteristics across platforms

## Quick Start

### Requirements

- C++20 compatible compiler:
  - **GCC 14+** (14.2.0 tested)
  - **Clang 18+** (19.1.7 tested)
  - **MSVC 2022+** (19.44+ tested)
- CMake 3.20 or higher

### CMake Integration

```cmake
# Build options
option(NFX_STRINGBUILDERPOOL_BUILD_STATIC         "Build static library"               ON  )
option(NFX_STRINGBUILDERPOOL_BUILD_SHARED         "Build shared library"               ON  )
option(NFX_STRINGBUILDERPOOL_USE_CACHE            "Enable compiler cache"              ON  )

# Development options
option(NFX_STRINGBUILDERPOOL_BUILD_TESTS          "Build tests"                        ON  )
option(NFX_STRINGBUILDERPOOL_BUILD_SAMPLES        "Build samples"                      ON  )
option(NFX_STRINGBUILDERPOOL_BUILD_BENCHMARKS     "Build benchmarks"                   ON  )
option(NFX_STRINGBUILDERPOOL_BUILD_DOCUMENTATION  "Build Doxygen documentation"        ON  )

# Installation and packaging
option(NFX_STRINGBUILDERPOOL_INSTALL_PROJECT      "Install project"                    ON  )
option(NFX_STRINGBUILDERPOOL_PACKAGE_SOURCE       "Enable source package generation"   ON  )
option(NFX_STRINGBUILDERPOOL_PACKAGE_ARCHIVE      "Enable TGZ/ZIP package generation"  ON  )
option(NFX_STRINGBUILDERPOOL_PACKAGE_DEB          "Enable DEB package generation"      ON  )
option(NFX_STRINGBUILDERPOOL_PACKAGE_RPM          "Enable RPM package generation"      ON  )
option(NFX_STRINGBUILDERPOOL_PACKAGE_WIX          "Enable WiX MSI installer"           ON  )
```

### Using in Your Project

#### Option 1: Using FetchContent (Recommended)

```cmake
include(FetchContent)
FetchContent_Declare(
  nfx-stringbuilderpool
  GIT_REPOSITORY https://github.com/ronan-fdev/nfx-stringbuilderpool.git
  GIT_TAG        main  # or use specific version tag like "1.0.0"
)
FetchContent_MakeAvailable(nfx-stringbuilderpool)

# Link with static library (recommended)
target_link_libraries(your_target PRIVATE nfx-stringbuilderpool::static)

# Or link with shared library
# target_link_libraries(your_target PRIVATE nfx-stringbuilderpool::nfx-stringbuilderpool)
```

#### Option 2: As a Git Submodule

```bash
# Add as submodule
git submodule add https://github.com/ronan-fdev/nfx-stringbuilderpool.git third-party/nfx-stringbuilderpool
```

```cmake
# In your CMakeLists.txt
add_subdirectory(third-party/nfx-stringbuilderpool)
target_link_libraries(your_target PRIVATE nfx-stringbuilderpool::static)
```

#### Option 3: Using find_package (After Installation)

```cmake
find_package(nfx-stringbuilderpool REQUIRED)
target_link_libraries(your_target PRIVATE nfx-stringbuilderpool::static)
```

### Building

```bash
# Clone the repository
git clone https://github.com/ronan-fdev/nfx-stringbuilderpool.git
cd nfx-stringbuilderpool

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build the library
cmake --build . --config Release --parallel

# Run tests (optional)
ctest -C Release --output-on-failure

# Run benchmarks (optional)
./build/bin/benchmarks/BM_StringBuilderPool
```

### Documentation

nfx-stringbuilderpool includes comprehensive API documentation generated with Doxygen.

#### 📚 Online Documentation

The complete API documentation is available online at:
**https://ronan-fdev.github.io/nfx-stringbuilderpool**

#### Building Documentation Locally

```bash
# Configure with documentation enabled
cmake .. -DCMAKE_BUILD_TYPE=Release -DNFX_STRINGBUILDERPOOL_BUILD_DOCUMENTATION=ON

# Build the documentation
cmake --build . --target documentation
```

#### Requirements

- **Doxygen** - Documentation generation tool
- **Graphviz Dot** (optional) - For generating class diagrams

#### Accessing Local Documentation

After building, open `./build/doc/html/index.html` in your web browser.

## Usage Examples

### Basic String Building

```cpp
#include <nfx/string/StringBuilderPool.h>

using namespace nfx::string;

int main()
{
    // Acquire a lease from the pool (RAII - automatic cleanup)
    auto lease = StringBuilderPool::lease();
    auto builder = lease.create();

    // Build strings with fluent interface
    builder << "Hello" << ", " << "World" << "!";

    // Convert to std::string
    std::string result = lease.toString();
    // Output: "Hello, World!"

    return 0;
} // Buffer automatically returned to pool
```

### Efficient String Concatenation

```cpp
#include <nfx/string/StringBuilderPool.h>
#include <vector>

using namespace nfx::string;

std::string buildReport(const std::vector<std::string>& items)
{
    auto lease = StringBuilderPool::lease();
    auto builder = lease.create();

    // Reserve capacity for better performance
    lease.buffer().reserve(1024);

    // Build header
    builder << "=== Report ===\n";

    // Add items
    for (size_t i = 0; i < items.size(); ++i)
    {
        builder << "Item " << std::to_string(i + 1) << ": " << items[i] << "\n";
    }

    // Add footer
    builder << "Total items: " << std::to_string(items.size());

    return lease.toString();
}
```

### Working with Different String Types

```cpp
#include <nfx/string/StringBuilderPool.h>

using namespace nfx::string;

void demonstrateStringTypes()
{
    auto lease = StringBuilderPool::lease();
    auto builder = lease.create();

    // std::string
    std::string stdStr = "from std::string";
    builder << stdStr << " | ";

    // string_view (zero-copy)
    std::string_view sv = "from string_view";
    builder << sv << " | ";

    // C-string
    builder << "from C-string" << " | ";

    // Single characters
    builder << 'A' << 'B' << 'C';

    std::string result = lease.toString();
    // Output: "from std::string | from string_view | from C-string | ABC"
}
```

### Direct Buffer Manipulation

```cpp
#include <nfx/string/StringBuilderPool.h>

using namespace nfx::string;

void directBufferAccess()
{
    auto lease = StringBuilderPool::lease();

    // Direct access to underlying buffer
    auto& buffer = lease.buffer();

    // Append methods
    buffer.append("Direct ");
    buffer.append("buffer ");
    buffer.append("access");
    buffer.push_back('!');

    // Get size and capacity
    size_t size = buffer.size();         // Current content size
    size_t capacity = buffer.capacity(); // Allocated capacity

    // Zero-copy string_view access
    std::string_view view = buffer.toStringView();

    // Iterator support
    for (char c : buffer)
    {
        // Process each character
    }

    // Clear for reuse
    buffer.clear();
}
```

### Pool Statistics and Monitoring

```cpp
#include <nfx/string/StringBuilderPool.h>
#include <iostream>

using namespace nfx::string;

void demonstrateStatistics()
{
    // Reset statistics for clean measurement
    StringBuilderPool::resetStats();

    // Perform string operations
    for (int i = 0; i < 1000; ++i)
    {
        auto lease = StringBuilderPool::lease();
        auto builder = lease.create();
        builder << "Iteration " << std::to_string(i);
        std::string result = lease.toString();
    }

    // Get statistics
    auto stats = StringBuilderPool::stats();

    std::cout << "Total Requests: " << stats.totalRequests << "\n";
    std::cout << "Thread-Local Hits: " << stats.threadLocalHits << "\n";
    std::cout << "Pool Hits: " << stats.dynamicStringBufferPoolHits << "\n";
    std::cout << "New Allocations: " << stats.newAllocations << "\n";
    std::cout << "Hit Rate: " << (stats.hitRate * 100.0) << "%\n";

    // Clear pool if needed
    size_t cleared = StringBuilderPool::clear();
    std::cout << "Cleared " << cleared << " buffers from pool\n";
}
```

### Multi-threaded Usage

```cpp
#include <nfx/string/StringBuilderPool.h>
#include <thread>
#include <vector>

using namespace nfx::string;

void threadWorker(int threadId)
{
    // Each thread safely uses the pool
    for (int i = 0; i < 100; ++i)
    {
        auto lease = StringBuilderPool::lease();
        auto builder = lease.create();

        builder << "Thread " << std::to_string(threadId)
                << " - Iteration " << std::to_string(i);

        std::string result = lease.toString();
        // Process result...
    }
}

void demonstrateMultithreading()
{
    std::vector<std::thread> threads;

    // Spawn multiple threads
    for (int i = 0; i < 4; ++i)
    {
        threads.emplace_back(threadWorker, i);
    }

    // Wait for completion
    for (auto& t : threads)
    {
        t.join();
    }

    // Check pool statistics
    auto stats = StringBuilderPool::stats();
    std::cout << "Multi-threaded hit rate: "
              << (stats.hitRate * 100.0) << "%\n";
}
```

### Performance Comparison Example

```cpp
#include <nfx/string/StringBuilderPool.h>
#include <sstream>
#include <chrono>
#include <iostream>

using namespace nfx::string;

void performanceComparison()
{
    const int iterations = 10000;
    const std::vector<std::string> words = {"Hello", "World", "Performance", "Test"};

    // Test 1: StringBuilderPool
    auto start1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i)
    {
        auto lease = StringBuilderPool::lease();
        auto builder = lease.create();
        for (const auto& word : words)
        {
            builder << word << " ";
        }
        std::string result = lease.toString();
    }
    auto end1 = std::chrono::high_resolution_clock::now();
    auto duration1 = std::chrono::duration_cast<std::chrono::microseconds>(end1 - start1);

    // Test 2: std::string
    auto start2 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i)
    {
        std::string result;
        for (const auto& word : words)
        {
            result += word + " ";
        }
    }
    auto end2 = std::chrono::high_resolution_clock::now();
    auto duration2 = std::chrono::duration_cast<std::chrono::microseconds>(end2 - start2);

    // Test 3: std::ostringstream
    auto start3 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i)
    {
        std::ostringstream oss;
        for (const auto& word : words)
        {
            oss << word << " ";
        }
        std::string result = oss.str();
    }
    auto end3 = std::chrono::high_resolution_clock::now();
    auto duration3 = std::chrono::duration_cast<std::chrono::microseconds>(end3 - start3);

    std::cout << "StringBuilderPool: " << duration1.count() << " μs\n";
    std::cout << "std::string:       " << duration2.count() << " μs\n";
    std::cout << "std::ostringstream:" << duration3.count() << " μs\n";
}
```

### Complete Example

```cpp
#include <iostream>
#include <nfx/string/StringBuilderPool.h>

int main()
{
    using namespace nfx::string;

    // Example: Building a formatted log message
    {
        auto lease = StringBuilderPool::lease();
        auto builder = lease.create();

        // Build structured log entry
        builder << "[INFO] ";
        builder << "User logged in: ";
        builder << "username=john.doe, ";
        builder << "ip=192.168.1.100, ";
        builder << "timestamp=2025-10-31";

        std::string logMessage = lease.toString();
        std::cout << logMessage << std::endl;
    } // Lease returned to pool here

    // Acquire a new lease (should hit the pool cache)
    {
        auto lease = StringBuilderPool::lease();
        auto builder = lease.create();

        builder << "[ERROR] ";
        builder << "Connection failed: ";
        builder << "timeout after 30s";

        std::string errorMessage = lease.toString();
        std::cout << errorMessage << std::endl;
    } // Lease returned to pool here

    // Display pool statistics
    auto stats = StringBuilderPool::stats();
    std::cout << "\nPool Statistics:\n";
    std::cout << "Total requests: " << stats.totalRequests << "\n";
    std::cout << "Hit rate: " << (stats.hitRate * 100.0) << "%\n";

    return 0;
}
```

**Sample Output:**

```
[INFO] User logged in: username=john.doe, ip=192.168.1.100, timestamp=2025-10-31
[ERROR] Connection failed: timeout after 30s

Pool Statistics:
Total requests: 2
Hit rate: 50%
```

## Installation & Packaging

nfx-stringbuilderpool provides comprehensive packaging options for distribution.

### Package Generation

```bash
# Configure with packaging options
cmake .. -DCMAKE_BUILD_TYPE=Release \
		 -DNFX_STRINGBUILDERPOOL_BUILD_STATIC=ON \
		 -DNFX_STRINGBUILDERPOOL_BUILD_SHARED=ON \
		 -DNFX_STRINGBUILDERPOOL_PACKAGE_ARCHIVE=ON \
		 -DNFX_STRINGBUILDERPOOL_PACKAGE_DEB=ON \
		 -DNFX_STRINGBUILDERPOOL_PACKAGE_RPM=ON

# Generate binary packages
cmake --build . --target package
# or
cd build && cpack

# Generate source packages
cd build && cpack --config CPackSourceConfig.cmake
```

### Supported Package Formats

| Format      | Platform       | Description                        | Requirements |
| ----------- | -------------- | ---------------------------------- | ------------ |
| **TGZ/ZIP** | Cross-platform | Compressed archive packages        | None         |
| **DEB**     | Debian/Ubuntu  | Native Debian packages             | `dpkg-dev`   |
| **RPM**     | RedHat/SUSE    | Native RPM packages                | `rpm-build`  |
| **WiX**     | Windows        | Professional MSI installer         | `WiX 3.11+`  |
| **Source**  | Cross-platform | Source code distribution (TGZ+ZIP) | None         |

### Installation

```bash
# Linux (DEB-based systems)
sudo dpkg -i nfx-stringbuilderpool_*_amd64.deb

# Linux (RPM-based systems)
sudo rpm -ivh nfx-stringbuilderpool-*-Linux.rpm

# Windows
# Run the .exe installer with administrator privileges
nfx-stringbuilderpool-*-win64.exe

# Manual installation (extract archive)
tar -xzf nfx-stringbuilderpool-*-Linux.tar.gz -C /usr/local/
```

## Project Structure

```
nfx-stringbuilderpool/
├── benchmark/             # Performance benchmarks with Google Benchmark
├── cmake/                 # CMake modules and configuration
├── include/nfx/           # Public headers
├── samples/               # Example usage and demonstrations
├── samples/               # Example usage and demonstrations
├── src/                   # Implementation files
└── test/                  # Comprehensive unit tests with GoogleTest
```

## Performance

For detailed performance metrics across all compilers (GCC, Clang, MSVC, MinGW) and platforms, see the [benchmark documentation](benchmark/README.md).

## Roadmap

See [TODO.md](TODO.md) for upcoming features and project roadmap.

## Changelog

See the [changelog](CHANGELOG.md) for a detailed history of changes, new features, and bug fixes.

## License

This project is licensed under the MIT License.

## Dependencies

- **[GoogleTest](https://github.com/google/googletest)**: Testing framework (BSD 3-Clause License) - Development only
- **[Google Benchmark](https://github.com/google/benchmark)**: Performance benchmarking framework (Apache 2.0 License) - Development only

All dependencies are automatically fetched via CMake FetchContent when building tests or benchmarks.

---

_Updated on November 8, 2025_
