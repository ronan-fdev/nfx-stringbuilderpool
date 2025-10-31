# Benchmarks

---

## Test Environment

### Hardware Configuration

| Component                | Specification                                                 |
| ------------------------ | ------------------------------------------------------------- |
| **CPU**                  | 12th Gen Intel Core i7-12800H (20 logical, 14 physical cores) |
| **Base Clock**           | 2.80 GHz                                                      |
| **Turbo Clock**          | 4.80 GHz                                                      |
| **L1 Data Cache**        | 48 KiB (×6 P-cores) + 32 KiB (×8 E-cores)                     |
| **L1 Instruction Cache** | 32 KiB (×6 P-cores) + 64 KiB (×2 E-core clusters)             |
| **L2 Unified Cache**     | 1.25 MiB (×6 P-cores) + 2 MiB (×2 E-core clusters)            |
| **L3 Unified Cache**     | 24 MiB (×1 shared)                                            |
| **RAM**                  | DDR4-3200 (32GB)                                              |
| **GPU**                  | NVIDIA RTX A2000 4GB GDDR6                                    |

### Software Configuration

| Platform    | Benchmark Framework     | C++ Compiler              | nfx-stringbuilderpool Version |
| ----------- | ----------------------- | ------------------------- | ----------------------------- |
| **Linux**   | Google Benchmark v1.9.4 | GCC 14.2.0-x64            | v1.0.0                        |
| **Linux**   | Google Benchmark v1.9.4 | Clang 19.1.7-x64          | v1.0.0                        |
| **Windows** | Google Benchmark v1.9.4 | MinGW GCC 14.2.0-x64      | v1.0.0                        |
| **Windows** | Google Benchmark v1.9.4 | Clang-GNU-CLI 19.1.5-x64  | v1.0.0                        |
| **Windows** | Google Benchmark v1.9.4 | Clang-MSVC-CLI 19.1.5-x64 | v1.0.0                        |
| **Windows** | Google Benchmark v1.9.4 | MSVC 19.44.35217.0-x64    | v1.0.0                        |

---

# Performance Results

## String Building Benchmarks

| Operation                                | Linux GCC   | Linux Clang | Windows MinGW GCC | Windows Clang-GNU-CLI | Windows Clang-MSVC-CLI | Windows MSVC |
| ---------------------------------------- | ----------- | ----------- | ----------------: | --------------------: | ---------------------: | -----------: |
| **BM_StdString_SmallStrings**            | **29.2 ns** | 34.6 ns     |           60.6 ns |               92.3 ns |                76.0 ns |      57.9 ns |
| **BM_StringStream_SmallStrings**         | **155 ns**  | 170 ns      |            238 ns |                688 ns |                 565 ns |       415 ns |
| **BM_StringBuilderPool_SmallStrings**    | 46.4 ns     | **42.4 ns** |            100 ns |                117 ns |                92.6 ns |      67.0 ns |
| **BM_StdString_MediumStrings**           | **68.7 ns** | 74.1 ns     |            173 ns |                293 ns |                 226 ns |       158 ns |
| **BM_StringStream_MediumStrings**        | **184 ns**  | 204 ns      |            281 ns |               1138 ns |                 875 ns |       709 ns |
| **BM_StringBuilderPool_MediumStrings**   | 72.8 ns     | **70.8 ns** |            130 ns |                179 ns |                 137 ns |       104 ns |
| **BM_StdString_LargeStrings**            | **71.0 ns** | 82.4 ns     |            143 ns |                227 ns |                 168 ns |       131 ns |
| **BM_StringStream_LargeStrings**         | **209 ns**  | 249 ns      |            362 ns |               1228 ns |                 911 ns |       775 ns |
| **BM_StringBuilderPool_LargeStrings**    | 65.7 ns     | **64.5 ns** |            123 ns |                165 ns |                 118 ns |      99.8 ns |
| **BM_StdString_RapidCycles**             | **156 ns**  | 182 ns      |            452 ns |                823 ns |                 587 ns |       488 ns |
| **BM_StringBuilderPool_PoolEfficiency**  | **486 ns**  | 488 ns      |           1094 ns |               1313 ns |                1007 ns |       754 ns |
| **BM_StdString_MixedOperations**         | **78.5 ns** | 101 ns      |            168 ns |                403 ns |                 275 ns |       227 ns |
| **BM_StringStream_MixedOperations**      | **431 ns**  | 506 ns      |            516 ns |               2533 ns |                1746 ns |      1638 ns |
| **BM_StringBuilderPool_MixedOperations** | 230 ns      | **220 ns**  |            330 ns |                517 ns |                 350 ns |       278 ns |
| **BM_StringBuilderPool_BufferReuse**     | 808 ns      | **714 ns**  |           1271 ns |               1913 ns |                1306 ns |      1034 ns |
| **BM_StringBuilderPool_ZeroAlloc**       | 62.4 ns     | **58.8 ns** |            110 ns |                110 ns |                77.9 ns |      70.6 ns |
| **BM_StringBuilderPool_MemoryPressure**  | **1728 ns** | 1773 ns     |           3567 ns |               5817 ns |                3843 ns |      2936 ns |

---

_Benchmarks executed on October 31, 2025_
