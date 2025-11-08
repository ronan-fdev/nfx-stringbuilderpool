# nfx-stringbuilderpool TODO

Project roadmap and task tracking for the nfx-stringbuilderpool library.

### Todo

- [ ] Implement formatted append operations (similar to std::format)
- [ ] Add StringBuilder::appendFormat() method with variadic template support
- [ ] Additional buffer operations
  - [ ] insert() method for mid-buffer insertion
  - [ ] erase() method for range removal
  - [ ] replace() method for substring replacement
- [ ] Unicode support
  - [ ] UTF-8 validation and manipulation
  - [ ] UTF-16/UTF-32 conversion utilities
  - [ ] Consider Grapheme cluster boundary detection

### v2.0.0 (Breaking changes)

- [ ] Refactor API for improved ergonomics
  - [ ] Consider removing redundant methods
  - [ ] Evaluate streamlining builder creation
  - [ ] Review exception specifications
  - [ ] Evaluate noexcept guarantees
- [ ] Add custom allocator support for DynamicStringBuffer
- [ ] Enhanced pool configuration
  - [ ] Configurable thread-local cache size
  - [ ] Configurable shared pool limits
  - [ ] Runtime pool tuning API
- [ ] Performance optimizations
  - [ ] SIMD-optimized string operations (AVX2/SSE4.2)
- [ ] Update all tests, samples, and documentation

### In Progress

- NIL

### Done ✓

- NIL
