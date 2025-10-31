/**
 * @file StringBuilderPool.h
 * @brief String pooling implementation for high-performance string building
 * @details Thread-safe shared pool with RAII lease management and statistics
 *
 * ## StringBuilderPool High-Level Architecture:
 *
 * ```
 * StringBuilderPool Public API Structure:
 * ┌─────────────────────────────────────────────────────────────┐
 * │            StringBuilderPool (Static Interface)             │
 * ├─────────────────────────────────────────────────────────────┤
 * │                lease() → StringBuilderLease                 │ ← Primary factory method
 * │                            ↓                                │
 * │  ┌─────────────────────────────────────────────────────┐    │
 * │  │            StringBuilderLease (RAII)                │    │ ← Automatic cleanup
 * │  │  ┌─────────────────────────────────────────────┐    │    │
 * │  │  │  create()  → StringBuilder                  │    │    │ ← Fluent interface
 * │  │  │  buffer()  → DynamicStringBuffer            │    │    │ ← Direct access
 * │  │  │  toString() → std::string                   │    │    │ ← Conversion
 * │  │  └─────────────────────────────────────────────┘    │    │
 * │  └─────────────────────────────────────────────────────┘    │
 * │                                                             │
 * │  StringBuilder: Fluent string building interface            │ ← Stream operators
 * │  DynamicStringBuffer: High-performance buffer (SBO)         │ ← Zero-copy operations
 * └─────────────────────────────────────────────────────────────┘
 * ```
 *
 * ## Typical Usage Pattern:
 *
 * ```
 * StringBuilderPool Usage Flow:
 * ┌─────────────────────────────────────────────────────────────┐
 * │                  User Code Example                          │
 * ├─────────────────────────────────────────────────────────────┤
 * │  // 1. Acquire lease (automatic pooling)                    │
 * │  auto lease = StringBuilderPool::lease();                   │
 * │                            ↓                                │
 * │  // 2. Create builder for fluent operations                 │
 * │  auto builder = lease.create();                             │
 * │                            ↓                                │
 * │  // 3. Build string with zero allocations                   │
 * │  builder << "Hello" << ", " << "World" << "!";              │
 * │  builder.append(" Additional text");                        │
 * │                            ↓                                │
 * │  // 4. Extract result                                       │
 * │  std::string result = lease.toString();                     │
 * │                            ↓                                │
 * │  // 5. Automatic cleanup (lease destructor)                 │
 * │  // Buffer returned to pool for reuse                       │
 * └─────────────────────────────────────────────────────────────┘
 * ```
 */

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace nfx::string
{
	//=====================================================================
	// DynamicStringBuffer class
	//=====================================================================

	/**
	 * @brief High-performance dynamic string buffer with efficient memory management
	 * @details Provides a growable character buffer optimized for string building operations.
	 *          Features automatic capacity management, iterator support, and zero-copy
	 *          string_view access. Designed for internal use by StringBuilderPool.
	 *
	 * @warning Not thread-safe - external synchronization required for concurrent access.
	 *
	 * @see StringBuilderPool for the recommended high-level interface
	 * @see StringBuilder for a more convenient wrapper around this buffer
	 */
	class DynamicStringBuffer final
	{
		friend class DynamicStringBufferPool;

	private:
		//----------------------------------------------
		// Construction
		//----------------------------------------------

		/** @brief Default constructor */
		DynamicStringBuffer();

		/**
		 * @brief Constructor with specified initial capacity
		 * @param initialCapacity Initial buffer capacity in bytes
		 * @details Pre-allocates buffer to avoid reallocations during initial growth
		 */
		explicit DynamicStringBuffer( size_t initialCapacity );

	public:
		/**
		 * @brief Copy constructor
		 * @param other The DynamicStringBuffer to copy from
		 */
		DynamicStringBuffer( const DynamicStringBuffer& other );

		/**
		 * @brief Move constructor
		 * @param other The DynamicStringBuffer to move from
		 */
		DynamicStringBuffer( DynamicStringBuffer&& other ) noexcept;

		//----------------------------------------------
		// Destruction
		//----------------------------------------------

		/** @brief Destructor */
		~DynamicStringBuffer() = default;

		//----------------------------------------------
		// Assignment
		//----------------------------------------------

		/**
		 * @brief Copy assignment operator
		 * @param other The DynamicStringBuffer to copy from
		 * @return Reference to this DynamicStringBuffer after assignment
		 */
		DynamicStringBuffer& operator=( const DynamicStringBuffer& other );

		/**
		 * @brief Move assignment operator
		 * @param other The DynamicStringBuffer to move from
		 * @return Reference to this DynamicStringBuffer after assignment
		 */
		DynamicStringBuffer& operator=( DynamicStringBuffer&& other ) noexcept;

		//----------------------------------------------
		// Capacity and size information
		//----------------------------------------------

		/**
		 * @brief Get current buffer size in bytes
		 * @return Number of bytes currently stored in buffer
		 * @details Returns actual content size, not allocated capacity
		 * @note This function is marked [[nodiscard]] - the return value should not be ignored
		 */
		[[nodiscard]] size_t size() const noexcept;

		/**
		 * @brief Get current buffer capacity in bytes
		 * @return Number of bytes allocated for buffer storage
		 * @details Capacity may be larger than size to avoid frequent reallocations
		 * @note This function is marked [[nodiscard]] - the return value should not be ignored
		 */
		[[nodiscard]] size_t capacity() const noexcept;

		/**
		 * @brief Check if buffer is empty
		 * @return true if buffer contains no data, false otherwise
		 * @note This function is marked [[nodiscard]] - the return value should not be ignored
		 */
		[[nodiscard]] bool isEmpty() const noexcept;

		//----------------------------------------------
		// Buffer management
		//----------------------------------------------

		/**
		 * @brief Clear buffer content without deallocating memory
		 * @details Sets size to 0 but preserves allocated capacity for reuse
		 */
		void clear() noexcept;

		/**
		 * @brief Reserve minimum capacity for buffer
		 * @param newCapacity Minimum desired capacity in bytes
		 * @details May allocate more than requested for efficiency
		 * @throws std::bad_alloc if memory allocation fails
		 */
		void reserve( size_t newCapacity );

		/**
		 * @brief Resize buffer to specified size
		 * @param newSize New buffer size in bytes
		 * @details May truncate content or extend with undefined bytes
		 * @throws std::bad_alloc if memory allocation fails
		 */
		void resize( size_t newSize );

		//----------------------------------------------
		// Data access
		//----------------------------------------------

		/**
		 * @brief Get mutable pointer to buffer data
		 * @return Pointer to first byte of buffer data
		 * @details Provides direct memory access for high-performance operations
		 * @note This function is marked [[nodiscard]] - the return value should not be ignored
		 */
		[[nodiscard]] char* data() noexcept;

		/**
		 * @brief Get immutable pointer to buffer data
		 * @return Const pointer to first byte of buffer data
		 * @details Safe read-only access to buffer contents
		 * @note This function is marked [[nodiscard]] - the return value should not be ignored
		 */
		[[nodiscard]] const char* data() const noexcept;

		/**
		 * @brief Access buffer element by index (mutable)
		 * @param index Zero-based index of element to access
		 * @return Reference to element at specified index
		 * @details No bounds checking - undefined behavior if index >= size()
		 */
		char& operator[]( size_t index );

		/**
		 * @brief Access buffer element by index (immutable)
		 * @param index Zero-based index of element to access
		 * @return Const reference to element at specified index
		 * @details No bounds checking - undefined behavior if index >= size()
		 */
		const char& operator[]( size_t index ) const;

		//----------------------------------------------
		// Content manipulation
		//----------------------------------------------

		/**
		 * @brief Append string_view content to buffer
		 * @param str String view to append
		 * @details Efficient append without copying string data
		 * @throws std::bad_alloc if buffer expansion fails
		 */
		void append( std::string_view str );

		/**
		 * @brief Append std::string content to buffer
		 * @param str String to append
		 * @details Convenience overload for std::string
		 * @throws std::bad_alloc if buffer expansion fails
		 */
		void append( const std::string& str );

		/**
		 * @brief Append null-terminated C string to buffer
		 * @param str Null-terminated string to append
		 * @details Safe handling of nullptr (no-op)
		 * @throws std::bad_alloc if buffer expansion fails
		 */
		void append( const char* str );

		/**
		 * @brief Append single character to buffer
		 * @param c Character to append
		 * @details Efficient single-character append
		 * @throws std::bad_alloc if buffer expansion fails
		 */
		void push_back( char c );

		//----------------------------------------------
		// String conversion
		//----------------------------------------------

		/**
		 * @brief Convert buffer content to std::string
		 * @return String copy of buffer content
		 * @details Creates new string object - consider toStringView() for read-only access
		 */
		[[nodiscard]] std::string toString() const;

		/**
		 * @brief Get string_view of buffer content
		 * @return String view referencing buffer data
		 * @details Zero-copy access - view becomes invalid if buffer is modified
		 * @note This function is marked [[nodiscard]] - the return value should not be ignored
		 */
		[[nodiscard]] std::string_view toStringView() const noexcept;

		//----------------------------------------------
		// Iterator interface
		//----------------------------------------------

		/** @brief Character type for iterator compatibility */
		using value_type = char;

		/** @brief Mutable iterator type for buffer traversal */
		using iterator = char*;

		/** @brief Immutable iterator type for buffer traversal */
		using const_iterator = const char*;

		/**
		 * @brief Get mutable iterator to beginning of buffer
		 * @return Iterator pointing to first element
		 * @details Enables range-based for loops and STL algorithms
		 * @note This function is marked [[nodiscard]] - the return value should not be ignored
		 */
		[[nodiscard]] iterator begin() noexcept;

		/**
		 * @brief Get immutable iterator to beginning of buffer
		 * @return Const iterator pointing to first element
		 * @details Safe read-only iteration over buffer contents
		 * @note This function is marked [[nodiscard]] - the return value should not be ignored
		 */
		[[nodiscard]] const_iterator begin() const noexcept;

		/**
		 * @brief Get mutable iterator to end of buffer
		 * @return Iterator pointing one past last element
		 * @details Standard STL end iterator semantics
		 * @note This function is marked [[nodiscard]] - the return value should not be ignored
		 */
		[[nodiscard]] iterator end() noexcept;

		/**
		 * @brief Get immutable iterator to end of buffer
		 * @return Const iterator pointing one past last element
		 * @details Standard STL end iterator semantics
		 * @note This function is marked [[nodiscard]] - the return value should not be ignored
		 */
		[[nodiscard]] const_iterator end() const noexcept;

	private:
		//----------------------------------------------
		// Small buffer optimization constants
		//----------------------------------------------

		/** @brief Stack buffer size optimized for typical string operations */
		static constexpr size_t STACK_BUFFER_SIZE = 256;

		/** @brief Growth factor for heap allocation */
		static constexpr auto GROWTH_FACTOR = 1.5;

		//----------------------------------------------
		// Private members
		//----------------------------------------------

		/** @brief Stack-allocated buffer for small strings */
		alignas( char ) char m_stackBuffer[STACK_BUFFER_SIZE];

		/** @brief Heap-allocated buffer for large strings */
		std::unique_ptr<char[]> m_heapBuffer;

		/** @brief Current size of data in buffer */
		size_t m_size;

		/** @brief Current capacity of buffer */
		size_t m_capacity;

		/** @brief True if using heap buffer, false if using stack buffer */
		bool m_onHeap;

		//----------------------------------------------
		// Private methods
		//----------------------------------------------

		/**
		 * @brief Ensures buffer has at least the specified capacity
		 * @param neededCapacity Minimum required capacity
		 */
		void ensureCapacity( size_t needed_capacity );

		/**
		 * @brief Returns pointer to current buffer (stack or heap)
		 * @return Pointer to active buffer
		 */
		char* currentBuffer() noexcept;

		/**
		 * @brief Returns const pointer to current buffer (stack or heap)
		 * @return Const pointer to active buffer
		 */
		const char* currentBuffer() const noexcept;
	};

	//=====================================================================
	// StringBuilder class
	//=====================================================================

	/**
	 * @brief High-performance string builder with fluent interface and efficient memory management
	 * @details Provides a convenient wrapper around DynamicStringBuffer with stream-like operators
	 *          for intuitive string construction. Features efficient append operations, iterator
	 *          support, and automatic memory management through the underlying buffer.
	 *
	 * @note This class is a lightweight wrapper that references an underlying DynamicStringBuffer.
	 *       It does not own the buffer memory - use StringBuilderPool::lease() for proper RAII management.
	 *
	 * @warning Not thread-safe - external synchronization required for concurrent access.
	 *          Multiple StringBuilder instances should not reference the same buffer concurrently.
	 *
	 * @see StringBuilderPool for the recommended way to obtain StringBuilder instances
	 * @see StringBuilderLease for RAII management of pooled buffers
	 * @see DynamicStringBuffer for the underlying buffer implementation
	 */
	class StringBuilder final
	{
		friend class StringBuilderLease;

		//----------------------------------------------
		// Construction
		//----------------------------------------------
	private:
		/** @brief Constructs StringBuilder wrapper around memory buffer */
		inline explicit StringBuilder( DynamicStringBuffer& buffer );

	public:
		/** @brief Default constructor */
		StringBuilder() = delete;

		/** @brief Copy constructor */
		StringBuilder( const StringBuilder& ) = default;

		/** @brief Move constructor */
		StringBuilder( StringBuilder&& ) noexcept = delete;

		//----------------------------------------------
		// Destruction
		//----------------------------------------------

		/** @brief Destructor */
		~StringBuilder() = default;

		//----------------------------------------------
		// Assignment
		//----------------------------------------------

		/** @brief Copy assignment operator */
		StringBuilder& operator=( const StringBuilder& ) = delete;

		/** @brief Move assignment operator */
		StringBuilder& operator=( StringBuilder&& ) noexcept = delete;

		//----------------------------------------------
		// Array access operators
		//----------------------------------------------

		/**
		 * @brief Provides read-write access to character at specified index
		 * @param index Zero-based character index
		 * @return Reference to character at the specified position
		 */
		inline char& operator[]( size_t index );

		/**
		 * @brief Provides read-only access to character at specified index
		 * @param index Zero-based character index
		 * @return Const reference to character at the specified position
		 */
		inline const char& operator[]( size_t index ) const;

		//----------------------------------------------
		// String append operations
		//----------------------------------------------

		/**
		 * @brief Appends string_view contents to the buffer efficiently
		 * @param str String view to append
		 */
		inline void append( std::string_view str );

		/**
		 * @brief Appends std::string contents to the buffer
		 * @param str String to append
		 */
		inline void append( const std::string& str );

		/**
		 * @brief Appends null-terminated C-string to the buffer
		 * @param str Null-terminated C-string to append (null pointer handled gracefully)
		 */
		inline void append( const char* str );

		/**
		 * @brief Appends single character to the buffer
		 * @param c Character to append
		 */
		inline void push_back( char c );

		//----------------------------------------------
		// Stream operators
		//----------------------------------------------

		/**
		 * @brief Stream operator for string_view
		 * @param str String view to append
		 * @return Reference to this StringBuilder for chaining
		 */
		inline StringBuilder& operator<<( std::string_view str );

		/**
		 * @brief Stream operator for std::string
		 * @param str String to append
		 * @return Reference to this StringBuilder for chaining
		 */
		inline StringBuilder& operator<<( const std::string& str );

		/**
		 * @brief Stream operator for C-string
		 * @param str Null-terminated C-string to append
		 * @return Reference to this StringBuilder for chaining
		 */
		inline StringBuilder& operator<<( const char* str );

		/**
		 * @brief Stream operator for single character
		 * @param c Character to append
		 * @return Reference to this StringBuilder for chaining
		 */
		inline StringBuilder& operator<<( char c );

		//----------------------------------------------
		// Size and capacity management
		//----------------------------------------------

		/**
		 * @brief Returns current buffer size in characters
		 * @return Number of characters currently stored in the buffer
		 */
		inline size_t length() const noexcept;

		/**
		 * @brief Resizes buffer to specified character count
		 * @param newSize New buffer size in characters
		 */
		inline void resize( size_t newSize );

		//----------------------------------------------
		// Iterator interface
		//----------------------------------------------

		/** @brief Character type for iterator compatibility */
		using value_type = char;

		/** @brief Mutable iterator type for buffer traversal */
		using iterator = char*;

		/** @brief Immutable iterator type for buffer traversal */
		using const_iterator = const char*;

		/**
		 * @brief Returns mutable iterator to beginning of character sequence
		 * @return Iterator pointing to the first character in the buffer
		 */
		inline iterator begin();

		/**
		 * @brief Returns const iterator to beginning of character sequence
		 * @return Const iterator pointing to the first character in the buffer
		 */
		inline const_iterator begin() const;

		/**
		 * @brief Returns mutable iterator to end of character sequence
		 * @return Iterator pointing one past the last character in the buffer
		 */
		inline iterator end();

		/**
		 * @brief Returns const iterator to end of character sequence
		 * @return Const iterator pointing one past the last character in the buffer
		 */
		inline const_iterator end() const;

		//----------------------------------------------
		// StringBuilder::Enumerator class
		//----------------------------------------------

		/**
		 * @brief Forward-only iterator for character-by-character enumeration of StringBuilder content
		 * @details Provides a simple enumeration interface for iterating through StringBuilder characters
		 *          one at a time. Features reset capability and bounds-checked iteration with explicit
		 *          advancement control. Designed for scenarios requiring controlled iteration patterns.
		 *
		 * @note This enumerator maintains pointers to the underlying buffer data. The StringBuilder
		 *       must remain valid and unmodified during the enumeration lifetime.
		 *
		 * @warning Not thread-safe - external synchronization required for concurrent access.
		 *          Enumerator becomes invalid if the underlying StringBuilder is modified.
		 *
		 * @see StringBuilder for the parent container
		 * @see StringBuilder::begin(), StringBuilder::end() for STL-compatible iterators
		 */
		class Enumerator
		{
		public:
			//----------------------------
			// Construction
			//----------------------------

			/**
			 * @brief Constructs an enumerator for iterating over the characters in the given StringBuilder buffer.
			 * @param builder The StringBuilder to enumerate
			 * @note Calling current() is only valid after next() has returned true and until it returns false.
			 */
			inline Enumerator( const StringBuilder& builder );

			//----------------------------
			// Destruction
			//----------------------------

			/** @brief Destructor */
			~Enumerator() = default;

			//----------------------------
			// Enumerator operations
			//----------------------------

			/**
			 * @brief Advances to the next character in the buffer
			 * @return true if advanced to a valid character, false if reached end of buffer
			 */
			inline bool next();

			/**
			 * @brief Returns the current character in the buffer
			 * @return The character at the current enumeration position
			 */
			inline char current() const;

			/** @brief Resets the enumerator to the initial position (before the first character). */
			inline void reset();

		private:
			//----------------------------
			// Private member variables
			//----------------------------

			/** @brief Pointer to start of character data */
			const char* m_data;

			/** @brief Pointer to end of character data */
			const char* m_end;

			/** @brief Pointer to current position in iteration */
			const char* m_current;
		};

	private:
		//----------------------------------------------
		// Private member variables
		//----------------------------------------------

		/** @brief Reference to the underlying buffer */
		DynamicStringBuffer& m_buffer;
	};

	//=====================================================================
	// StringBuilderLease class
	//=====================================================================

	/**
	 * @brief RAII lease wrapper for pooled StringBuilder buffers with automatic resource management
	 * @details Provides exclusive access to a pooled DynamicStringBuffer through RAII semantics.
	 *          Automatically returns the buffer to the pool when the lease is destroyed, ensuring
	 *          optimal memory reuse and preventing resource leaks. Features move-only semantics
	 *          for safe transfer of ownership and convenient access methods.
	 *
	 * @note This class implements move-only semantics - copying is disabled to prevent
	 *       multiple ownership of the same buffer. Use std::move() for ownership transfer.
	 *
	 * @warning Not thread-safe - external synchronization required for concurrent access.
	 *          Do not share lease instances between threads without proper synchronization.
	 *
	 * @see StringBuilderPool::lease() for obtaining lease instances
	 * @see StringBuilder for the high-level string building interface
	 * @see DynamicStringBuffer for the underlying buffer implementation
	 */
	class StringBuilderLease final
	{
		friend class StringBuilderPool;

		//----------------------------------------------
		// Construction
		//----------------------------------------------
	private:
		/** @brief Constructs lease with pooled buffer ownership */
		inline explicit StringBuilderLease( DynamicStringBuffer* buffer );

	public:
		/** @brief Default constructor */
		StringBuilderLease() = delete;

		/** @brief Copy constructor */
		StringBuilderLease( const StringBuilderLease& ) = delete;

		/**
		 * @brief Move constructor
		 * @param other The StringBuilderLease to move from
		 */
		inline StringBuilderLease( StringBuilderLease&& other ) noexcept;

		//----------------------------------------------
		// Destruction
		//----------------------------------------------

		/** @brief Destructor */
		~StringBuilderLease();

		//----------------------------------------------
		// Assignment
		//----------------------------------------------

		/** @brief Copy assignment operator */
		StringBuilderLease& operator=( const StringBuilderLease& ) = delete;

		/**
		 * @brief Move assignment operator
		 * @param other The StringBuilderLease to move from
		 * @return Reference to this StringBuilderLease after assignment
		 */
		inline StringBuilderLease& operator=( StringBuilderLease&& other ) noexcept;

		//----------------------------------------------
		// Public interface methods
		//----------------------------------------------

		/**
		 * @brief Creates StringBuilder wrapper for buffer manipulation
		 * @return StringBuilder instance wrapping the leased buffer
		 * @note This function is marked [[nodiscard]] - the return value should not be ignored
		 */
		[[nodiscard]] inline StringBuilder create();

		/**
		 * @brief Provides direct access to underlying memory buffer
		 * @return Reference to the underlying DynamicStringBuffer
		 * @note This function is marked [[nodiscard]] - the return value should not be ignored
		 */
		[[nodiscard]] inline DynamicStringBuffer& buffer();

		/**
		 * @brief Converts buffer contents to std::string
		 * @return String copy of the buffer contents
		 * @note This function is marked [[nodiscard]] - the return value should not be ignored
		 */
		[[nodiscard]] inline std::string toString() const;

	private:
		//----------------------------------------------
		// Private implementation methods
		//----------------------------------------------

		/** @brief Returns buffer to pool and invalidates lease */
		void dispose();

		/** @brief Throws exception for invalid lease access */
		[[noreturn]] void throwInvalidOperation() const;

		//----------------------------------------------
		// Private member variables
		//----------------------------------------------

		/** @brief Pointer to the pooled memory buffer */
		DynamicStringBuffer* m_buffer;

		/** @brief Flag indicating if the lease is valid and buffer is accessible */
		bool m_valid;
	};

	//=====================================================================
	// StringBuilderPool class
	//=====================================================================

	/**
	 * @brief Thread-safe memory pool for high-performance StringBuilder instances with optimized allocation strategy
	 * @details Implements a three-tier pooling system for DynamicStringBuffer instances
	 *          to minimize allocation overhead in high-frequency string building scenarios. Features
	 *          thread-local caching, shared cross-thread pooling, and comprehensive statistics tracking.
	 *          Designed as a singleton with static factory methods for global access.
	 *
	 * ## Three-Tier Pooling Architecture:
	 *
	 * ```
	 * StringBuilderPool::lease() Buffer Acquisition Strategy:
	 * ┌─────────────────────────────────────────────────────────────┐
	 * │                      Client Request                         │
	 * │               StringBuilderPool::lease()                    │
	 * └─────────────────────────────────────────────────────────────┘
	 *                              ↓ (try first)
	 * ┌─────────────────────────────────────────────────────────────┐
	 * │               Tier 1: Thread-Local Cache                    │ ← Fastest (no locks)
	 * │  ┌─────────────────────────────────────────────────────┐    │
	 * │  │     thread_local DynamicStringBuffer* cache         │    │
	 * │  │       - Zero synchronization overhead               │    │
	 * │  │       - Immediate buffer availability               │    │
	 * │  │       - Perfect for single-threaded hotpaths        │    │
	 * │  └─────────────────────────────────────────────────────┘    │
	 * └─────────────────────────────────────────────────────────────┘
	 *                              ↓ (on miss - cache empty)
	 * ┌─────────────────────────────────────────────────────────────┐
	 * │            Tier 2: Shared Cross-Thread Pool                 │ ← Fast (mutex-protected)
	 * │  ┌─────────────────────────────────────────────────────┐    │
	 * │  │   DynamicStringBufferPool (singleton)               │    │
	 * │  │       - Mutex-protected buffer queue                │    │
	 * │  │       - Cross-thread buffer sharing                 │    │
	 * │  │       - Size-limited to prevent bloat               │    │
	 * │  └─────────────────────────────────────────────────────┘    │
	 * └─────────────────────────────────────────────────────────────┘
	 *                              ↓ (on miss - pool empty)
	 * ┌─────────────────────────────────────────────────────────────┐
	 * │                Tier 3: New Allocation                       │ ← Fallback (heap allocation)
	 * │  ┌─────────────────────────────────────────────────────┐    │
	 * │  │   new DynamicStringBuffer(optimal_size)             │    │
	 * │  │       - Pre-sized for typical usage patterns        │    │
	 * │  │       - Small Buffer Optimization (256-byte stack)  │    │
	 * │  │       - 1.5x growth factor for cache efficiency     │    │
	 * │  └─────────────────────────────────────────────────────┘    │
	 * └─────────────────────────────────────────────────────────────┘
	 *
	 * Buffer Return Process (via StringBuilderLease destructor):
	 * ┌─────────────────────────────────────────────────────────────┐
	 * │  1. Clear buffer content (zero-cost operation)              │
	 * │  2. Check size limits (prevent memory bloat)                │
	 * │  3. Return to thread-local cache (if space available)       │
	 * │  4. Return to shared pool (if thread-local full)            │
	 * │  5. Deallocate (if both pools full or buffer too large)     │
	 * └─────────────────────────────────────────────────────────────┘
	 * ```
	 *
	 * @note This class uses a singleton pattern with static methods - no instantiation required.
	 *       All pool operations are thread-safe and optimized for concurrent access patterns.
	 *
	 * @warning Pool buffers have size limits to prevent memory bloat - extremely large buffers
	 *          may not be returned to the pool and will be deallocated normally.
	 *
	 * @see StringBuilderLease for RAII buffer management
	 * @see StringBuilder for the high-level string building interface
	 * @see DynamicStringBuffer for the underlying buffer implementation
	 */
	class StringBuilderPool final
	{
	public:
		//----------------------------------------------
		// Pool statistics structure
		//----------------------------------------------

		/** @brief Pool performance statistics for external access */
		struct PoolStatistics
		{
			/** @brief Number of successful buffer retrievals from thread-local cache */
			uint64_t threadLocalHits;

			/** @brief Number of successful buffer retrievals from shared cross-thread pool */
			uint64_t dynamicStringBufferPoolHits;

			/** @brief Number of new buffer allocations when pools were empty */
			uint64_t newAllocations;

			/** @brief Total number of buffer requests made to the pool */
			uint64_t totalRequests;

			/** @brief Cache hit rate as a percentage (0.0 to 1.0) */
			double hitRate;
		};

	private:
		//----------------------------------------------
		// Construction
		//----------------------------------------------

		/** @brief Private constructor for singleton pattern */
		StringBuilderPool() = default;

	public:
		//----------------------------------------------
		// Static factory methods
		//----------------------------------------------

		/**
		 * @brief Creates a new StringBuilder lease with an optimally sourced memory buffer
		 * @return StringBuilderLease managing a pooled buffer with automatic cleanup
		 *
		 * This is the primary factory method for obtaining StringBuilder instances.
		 * The buffer is sourced using a three-tier optimization strategy:
		 * 1. Thread-local cache (fastest, zero synchronization overhead)
		 * 2. Shared cross-thread pool (fast, mutex-protected access)
		 * 3. New allocation (fallback, pre-sized for optimal performance)
		 *
		 * The returned lease automatically returns the buffer to the pool when
		 * destroyed, ensuring optimal memory reuse and preventing leaks.
		 *
		 * This method is thread-safe and optimized for high-frequency usage patterns.
		 * Buffers are automatically cleared before reuse and size-limited to prevent bloat.
		 */
		[[nodiscard]] static StringBuilderLease lease();

		//----------------------------
		// Statistics methods
		//----------------------------

		/**
		 * @brief Gets current pool statistics
		 * @return Current pool performance statistics structure
		 */
		static PoolStatistics stats() noexcept;

		/** @brief Resets pool statistics */
		static void resetStats() noexcept;

		//----------------------------
		// Lease management
		//----------------------------

		/**
		 * @brief Clears all buffers from the pool and returns the count of cleared buffers
		 * @return Number of buffers that were cleared from the pool
		 */
		static size_t clear();

		/**
		 * @brief Gets current number of buffers stored in the pool
		 * @return Number of buffers currently available in the pool
		 */
		static size_t size() noexcept;
	};
} // namespace nfx::string

#include "nfx/detail/string/StringBuilderPool.inl"
