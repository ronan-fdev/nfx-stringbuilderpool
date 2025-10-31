/*
 * MIT License
 *
 * Copyright (c) 2025 nfx
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * @file DynamicStringBufferPool.h
 * @brief Thread-safe shared pool for memory buffer management
 * @details Internal implementation of the three-tier pooling system for DynamicStringBuffer instances.
 *          Thread-local cache → shared pool → new allocation fallback pattern.
 *
 * Implementation Notes:
 * - Thread-local cache: Zero synchronization overhead for single-threaded hotpaths
 * - Shared pool: Mutex-protected cross-thread buffer sharing with size limits
 * - Statistics tracking: Atomic counters for performance monitoring
 * - Memory management: Size-based retention limits prevent unbounded growth
 */

#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <vector>

namespace nfx::string
{
	class DynamicStringBuffer;

	//=====================================================================
	// DynamicStringBufferPool class
	//=====================================================================

	/**
	 * @brief Thread-safe shared pool for memory buffer management with configurable performance parameters
	 * @details Implements a two-tier pooling strategy:
	 *          1. Thread-local cache: Each thread keeps one buffer for immediate reuse (fastest)
	 *          2. Shared pool: Cross-thread buffer sharing with mutex protection (slower but still fast)
	 *          3. New allocation: Only when both caches are exhausted (slowest)
	 *
	 * @note Thread-local buffers are automatically cleaned up when threads exit via ThreadLocalCleanup RAII pattern
	 */
	class DynamicStringBufferPool final
	{
	public:
		//----------------------------------------------
		// Construction
		//----------------------------------------------

		/**
		 * @brief Constructs DynamicStringBufferPool with configurable performance parameters
		 * @param initialCapacity Initial buffer capacity for new allocations
		 * @param maximumRetainedCapacity Maximum buffer size retained in pool before deletion
		 * @param maxPoolSize Maximum number of buffers stored in the shared pool
		 */
		explicit DynamicStringBufferPool(
			size_t initialCapacity = 256,
			size_t maximumRetainedCapacity = 2048,
			size_t maxPoolSize = 24 )
			: m_initialCapacity{ initialCapacity },
			  m_maximumRetainedCapacity{ maximumRetainedCapacity },
			  m_maxPoolSize{ maxPoolSize }
		{
		}

		/**
		 * @brief Copy constructor
		 * @details Copying is disabled to maintain singleton behavior and prevent resource duplication
		 */
		DynamicStringBufferPool( const DynamicStringBufferPool& ) = delete;

		/**
		 * @brief Move constructor
		 * @details Moving is disabled to maintain singleton behavior and prevent resource transfer
		 */
		DynamicStringBufferPool( DynamicStringBufferPool&& ) = delete;

		/**
		 * @brief Copy assignment operator
		 * @return Reference to this instance
		 * @details Assignment is disabled to maintain singleton behavior and prevent resource duplication
		 */
		DynamicStringBufferPool& operator=( const DynamicStringBufferPool& ) = delete;

		/**
		 * @brief Move assignment operator
		 * @return Reference to this instance
		 * @details Assignment is disabled to maintain singleton behavior and prevent resource transfer
		 */
		DynamicStringBufferPool& operator=( DynamicStringBufferPool&& ) = delete;

		//----------------------------------------------
		// Destruction
		//----------------------------------------------

		/** @brief Destructor */
		~DynamicStringBufferPool();

		//----------------------------------------------
		// Pool management methods
		//----------------------------------------------

		/**
		 * @brief Retrieves buffer from pool or creates new one
		 * @return Pointer to memory buffer ready for use
		 * @details Retrieval priority: 1) Thread-local cache, 2) Shared pool, 3) New allocation
		 */
		DynamicStringBuffer* get();

		/**
		 * @brief Returns buffer to pool for reuse
		 * @param buffer Buffer to return (must not be null, but method handles null gracefully)
		 * @details Return priority: 1) Thread-local cache (if empty), 2) Shared pool (if not full), 3) Delete
		 */
		void returnToPool( DynamicStringBuffer* buffer );

		//----------------------------------------------
		// Statistics
		//----------------------------------------------

		//----------------------------
		// PoolStatistics struct
		//----------------------------

		/** @brief Statistics structure containing pool performance metrics */
		struct PoolStatistics
		{
			/**
			 * @brief Default constructor
			 * @details Initializes all atomic counters to zero
			 */
			PoolStatistics() = default;

			/**
			 * @brief Copy constructor
			 * @details Copying is disabled due to atomic member variables that cannot be copied safely
			 */
			PoolStatistics( const PoolStatistics& ) = delete;

			/**
			 * @brief Move constructor
			 * @details Moving is disabled due to atomic member variables that cannot be moved safely
			 */
			PoolStatistics( PoolStatistics&& ) noexcept = delete;

			/**
			 * @brief Copy assignment operator
			 * @return Reference to this instance
			 * @details Assignment is disabled due to atomic member variables that cannot be copied safely
			 */
			PoolStatistics& operator=( const PoolStatistics& ) = delete;

			/**
			 * @brief Move assignment operator
			 * @return Reference to this instance
			 * @details Assignment is disabled due to atomic member variables that cannot be moved safely
			 */
			PoolStatistics& operator=( PoolStatistics&& ) noexcept = delete;

			/**
			 * @brief Calculates the pool hit rate as a percentage (0.0 to 1.0)
			 * @return Hit rate as a decimal value between 0.0 and 1.0
			 */
			double hitRate() const noexcept;

			/** @brief Resets all statistics counters to zero */
			void reset() noexcept;

			/** @brief Number of buffers retrieved from thread-local cache */
			std::atomic<uint64_t> threadLocalHits{ 0 };

			/** @brief Number of buffers retrieved from shared pool */
			std::atomic<uint64_t> dynamicStringBufferPoolHits{ 0 };

			/** @brief Number of new buffer allocations (cache misses) */
			std::atomic<uint64_t> newAllocations{ 0 };

			/** @brief Total number of buffer requests made to the pool */
			std::atomic<uint64_t> totalRequests{ 0 };
		};

		//----------------------------
		// Statistics methods
		//----------------------------

		/**
		 * @brief Gets current pool statistics
		 * @return Const reference to the pool statistics structure
		 */
		const PoolStatistics& stats() const noexcept;

		/**
		 * @brief Clears all buffers from the pool and returns the count of cleared buffers
		 * @return Number of buffers that were cleared from current thread's cache and shared pool
		 * @note This method can throw if memory deallocation fails
		 * @warning Only clears current thread's cached buffer - other threads' caches remain untouched
		 */
		size_t clear();

		/**
		 * @brief Gets current number of buffers stored in the pool
		 * @return Total count including current thread's cached buffer and shared pool buffers
		 */
		size_t size() const noexcept;

		/** @brief Resets pool statistics to zero */
		void resetStats() noexcept;

	private:
		//----------------------------------------------
		// Private member variables
		//----------------------------------------------

		/** @brief Vector containing available pooled buffers for cross-thread sharing */
		std::vector<DynamicStringBuffer*> m_pool;

		/** @brief Mutex protecting shared pool access during concurrent operations */
		mutable std::mutex m_mutex;

		/** @brief Initial capacity for newly allocated buffers (optimization for common use cases) */
		const size_t m_initialCapacity;

		/** @brief Maximum buffer capacity retained in pool before deletion (prevents memory bloat) */
		const size_t m_maximumRetainedCapacity;

		/** @brief Maximum number of buffers stored in shared pool (prevents unbounded growth) */
		const size_t m_maxPoolSize;

		/** @brief Pool performance statistics with atomic counters for thread safety */
		mutable PoolStatistics m_stats;
	};

	//----------------------------------------------
	// Singleton instance access
	//----------------------------------------------

	/**
	 * @brief Gets the singleton DynamicStringBufferPool instance
	 * @return Reference to the global shared pool instance
	 * @details Uses static local variable for thread-safe initialization and automatic cleanup
	 */
	inline static DynamicStringBufferPool& dynamicStringBufferPool() noexcept
	{
		// Parameters: 256-byte initial capacity, 2048-byte max retained, 24 buffer pool size
		static DynamicStringBufferPool pool{ 256, 2048, 24 };

		return pool;
	}
} // namespace nfx::string
