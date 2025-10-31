/**
 * @file TESTS_StringBuilderPool.cpp
 * @brief Comprehensive tests for StringBuilderPool high-performance string building
 * @details Tests covering pooling, lease management, thread safety, performance optimization,
 *          and enterprise-grade string building operations with zero-allocation design
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <iterator>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include <nfx/string/StringBuilderPool.h>

namespace nfx::string::test
{
	//=====================================================================
	// StringBuilderPool
	//=====================================================================

	//----------------------------------------------
	// Construction and basic functionality
	//----------------------------------------------

	TEST( StringBuilderPoolConstruction, BasicLease )
	{
		// Basic lease acquisition
		auto lease{ string::StringBuilderPool::lease() };

		// Access to StringBuilder and DynamicStringBuffer
		auto builder{ lease.create() };
		static_cast<void>( builder );
		auto& buffer{ lease.buffer() };

		EXPECT_NO_THROW( buffer.clear() );
		EXPECT_TRUE( buffer.isEmpty() );
		EXPECT_EQ( buffer.size(), 0 );
	}

	TEST( StringBuilderPoolConstruction, LeaseMoveSemantics )
	{
		// Create lease
		auto lease1{ string::StringBuilderPool::lease() };

		// Move construction
		auto lease2{ std::move( lease1 ) };

		// Move assignment
		auto lease3{ string::StringBuilderPool::lease() };
		lease3 = std::move( lease2 );

		// Should be able to use lease3
		auto builder{ lease3.create() };
		EXPECT_NO_THROW( builder.append( "test" ) );
	}

	TEST( StringBuilderPoolConstruction, LeaseAutomaticDisposal )
	{
		size_t initialPoolSize{ string::StringBuilderPool::size() };

		{
			auto lease{ string::StringBuilderPool::lease() };
			auto builder{ lease.create() };
			builder.append( "Test content" );
			// StringBuilder should be in use
		}
		// Lease goes out of scope, StringBuilder should be returned to pool

		// Pool size should be back to at least initial size (possibly larger due to caching)
		size_t finalPoolSize{ string::StringBuilderPool::size() };
		EXPECT_GE( finalPoolSize, initialPoolSize );
	}

	//----------------------------------------------
	// Pool statistics and management
	//----------------------------------------------

	TEST( StringBuilderPoolManagement, PoolStatistics )
	{
		// Clear pool and reset stats for clean test
		string::StringBuilderPool::clear();
		string::StringBuilderPool::resetStats();

		auto initialStats{ string::StringBuilderPool::stats() };
		EXPECT_EQ( initialStats.threadLocalHits, 0 );
		EXPECT_EQ( initialStats.dynamicStringBufferPoolHits, 0 );
		EXPECT_EQ( initialStats.newAllocations, 0 );
		EXPECT_EQ( initialStats.totalRequests, 0 );

		// Acquire lease and check stats
		{
			auto lease{ string::StringBuilderPool::lease() };
			auto builder{ lease.create() };
			builder.append( "Test statistics" );
		}

		auto statsAfterOne{ string::StringBuilderPool::stats() };
		EXPECT_EQ( statsAfterOne.totalRequests, 1 );
		EXPECT_GT( statsAfterOne.newAllocations + statsAfterOne.threadLocalHits + statsAfterOne.dynamicStringBufferPoolHits, 0 );

		// Acquire another lease (should potentially hit cache)
		{
			auto lease{ string::StringBuilderPool::lease() };
			auto builder{ lease.create() };
			builder.append( "Second test" );
		}

		auto statsAfterTwo{ string::StringBuilderPool::stats() };
		EXPECT_EQ( statsAfterTwo.totalRequests, 2 );
	}

	TEST( StringBuilderPoolManagement, PoolClearOperation )
	{
		// Create some leases to populate pool
		{
			auto lease1{ string::StringBuilderPool::lease() };
			auto lease2{ string::StringBuilderPool::lease() };
			auto lease3{ string::StringBuilderPool::lease() };
		}

		size_t poolSizeBeforeClear{ string::StringBuilderPool::size() };
		EXPECT_GT( poolSizeBeforeClear, 0 );

		// Clear pool
		size_t clearedCount{ string::StringBuilderPool::clear() };
		EXPECT_GT( clearedCount, 0 );

		size_t poolSizeAfterClear{ string::StringBuilderPool::size() };
		EXPECT_EQ( poolSizeAfterClear, 0 );
	}

	//----------------------------------------------
	// StringBuilderPool advanced
	//----------------------------------------------

	TEST( StringBuilderPoolAdvanced, SequentialReuse )
	{
		// Reset statistics for clean measurement
		string::StringBuilderPool::resetStats();

		// Use and release sequentially to test reuse
		{
			auto lease{ string::StringBuilderPool::lease() };
			lease.create().append( "test1" );
		} // Returns to thread-local cache

		{
			auto lease{ string::StringBuilderPool::lease() };
			EXPECT_EQ( lease.toString(), "" ); // Should be cleared
			lease.create().append( "test2" );
		} // Returns to thread-local cache

		{
			auto lease{ string::StringBuilderPool::lease() };
			EXPECT_EQ( lease.toString(), "" ); // Should be cleared
			lease.create().append( "test3" );
		}

		const auto& stats{ string::StringBuilderPool::stats() };
		EXPECT_EQ( stats.totalRequests, 3 );
		EXPECT_GE( stats.threadLocalHits, 2 ); // At least 2 hits from cache
	}

	TEST( StringBuilderPoolAdvanced, RapidAllocationDeallocation )
	{
		// Reset statistics for clean measurement
		string::StringBuilderPool::resetStats();

		const int iterations{ 100 };

		// Rapid allocation/deallocation cycles
		for ( int i{ 0 }; i < iterations; ++i )
		{
			auto lease{ string::StringBuilderPool::lease() };
			auto builder{ lease.create() };
			builder.append( "iteration " );
			builder.append( std::to_string( i ) );

			std::string expected{ "iteration " + std::to_string( i ) };
			EXPECT_EQ( lease.toString(), expected );
		} // Each lease destroyed at end of loop

		const auto& stats{ string::StringBuilderPool::stats() };
		EXPECT_EQ( stats.totalRequests, iterations );

		// Should have high hit rate due to thread-local caching
		double hitRate{ stats.hitRate };
		EXPECT_GT( hitRate, 0.90 ); // At least 90% hit rate
	}

	TEST( StringBuilderPoolAdvanced, StatisticsAccuracy )
	{
		// Reset statistics for clean measurement
		string::StringBuilderPool::resetStats();

		const auto& initialStats{ string::StringBuilderPool::stats() };
		EXPECT_EQ( initialStats.totalRequests, 0 );
		EXPECT_EQ( initialStats.threadLocalHits, 0 );
		EXPECT_EQ( initialStats.dynamicStringBufferPoolHits, 0 );
		EXPECT_EQ( initialStats.newAllocations, 0 );
		EXPECT_EQ( initialStats.hitRate, 0.0 );

		// Create and destroy several leases
		const int count{ 10 };
		for ( int i{ 0 }; i < count; ++i )
		{
			auto lease{ string::StringBuilderPool::lease() };
			auto builder{ lease.create() };
			builder.append( "stats test " );
			builder.append( std::to_string( i ) );
		}

		const auto& finalStats{ string::StringBuilderPool::stats() };
		EXPECT_EQ( finalStats.totalRequests, count );
		EXPECT_GT( finalStats.hitRate, 0.0 );

		// Total should equal sum of all hit types
		uint64_t totalHits{ finalStats.threadLocalHits +
							finalStats.dynamicStringBufferPoolHits +
							finalStats.newAllocations };
		EXPECT_EQ( totalHits, finalStats.totalRequests );
	}

	TEST( StringBuilderPoolAdvanced, ResourceManagement )
	{
		// Reset statistics for clean measurement
		string::StringBuilderPool::resetStats();

		// Create many leases simultaneously to test resource limits
		std::vector<string::StringBuilderLease> leases;
		const size_t maxLeases{ 30 }; // More than max pool size (24)

		for ( size_t i{ 0 }; i < maxLeases; ++i )
		{
			auto lease{ string::StringBuilderPool::lease() };
			auto builder{ lease.create() };
			builder.append( "lease " );
			builder.append( std::to_string( i ) );
			leases.push_back( std::move( lease ) );
		}

		// All should be valid and independent
		for ( size_t i{ 0 }; i < maxLeases; ++i )
		{
			std::string expected{ "lease " + std::to_string( i ) };
			EXPECT_EQ( leases[i].toString(), expected );
		}

		const auto& stats{ string::StringBuilderPool::stats() };
		EXPECT_EQ( stats.totalRequests, maxLeases );

		// Should have created new allocations since we exceeded pool capacity
		EXPECT_GT( stats.newAllocations, 0 );
	}

	//=====================================================================
	// StringBuilder
	//=====================================================================

	//----------------------------------------------
	// StringBuilder Construction
	//----------------------------------------------

	TEST( StringBuilderAdvanced, BasicConstruction )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto builder{ lease.create() };

		// Test initial state
		EXPECT_EQ( builder.length(), 0 );
		EXPECT_EQ( lease.toString(), "" );
	}

	TEST( StringBuilderAdvanced, CopyConstructor )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto builder1{ lease.create() };
		builder1.append( "Original" );

		// Test copy constructor
		auto builder2{ builder1 };
		EXPECT_EQ( builder2.length(), 8 );

		// Modify copy - should affect both (they share the same buffer)
		builder2.push_back( '!' );
		EXPECT_EQ( builder1.length(), 9 );
		EXPECT_EQ( builder2.length(), 9 );
		EXPECT_EQ( lease.toString(), "Original!" );
	}

	TEST( StringBuilderAdvanced, ConstArrayAccess )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto builder{ lease.create() };
		builder.append( "const" );

		const auto& constBuilder{ builder };

		// Test const access
		EXPECT_EQ( constBuilder[0], 'c' );
		EXPECT_EQ( constBuilder[1], 'o' );
		EXPECT_EQ( constBuilder[2], 'n' );
		EXPECT_EQ( constBuilder[3], 's' );
		EXPECT_EQ( constBuilder[4], 't' );
	}

	TEST( StringBuilderAdvanced, AppendStringView )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto builder{ lease.create() };

		std::string_view testStr{ "Hello, World!" };
		builder.append( testStr );

		EXPECT_EQ( builder.length(), 13 );
		EXPECT_EQ( lease.toString(), "Hello, World!" );
	}

	TEST( StringBuilderAdvanced, AppendStdString )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto builder{ lease.create() };

		std::string testStr{ "C++ StringBuilder" };
		builder.append( testStr );

		EXPECT_EQ( builder.length(), 17 );
		EXPECT_EQ( lease.toString(), "C++ StringBuilder" );
	}

	TEST( StringBuilderAdvanced, AppendCString )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto builder{ lease.create() };

		const char* testStr{ "Null-terminated" };
		builder.append( testStr );

		EXPECT_EQ( builder.length(), 15 );
		EXPECT_EQ( lease.toString(), "Null-terminated" );
	}

	TEST( StringBuilderAdvanced, AppendNullCString )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto builder{ lease.create() };

		// Should handle null pointer gracefully
		builder.append( static_cast<const char*>( nullptr ) );

		EXPECT_EQ( builder.length(), 0 );
		EXPECT_EQ( lease.toString(), "" );
	}

	//----------------------------------------------
	// StringBuilder basic operations
	//----------------------------------------------

	TEST( StringBuilderBasicOperations, AppendOperations )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto builder{ lease.create() };

		// String view append
		builder.append( "Hello" );
		EXPECT_EQ( lease.toString(), "Hello" );
		EXPECT_EQ( builder.length(), 5 );

		// String append
		std::string world{ " World" };
		builder.append( world );
		EXPECT_EQ( lease.toString(), "Hello World" );
		EXPECT_EQ( builder.length(), 11 );

		// C-string append
		builder.append( "!" );
		EXPECT_EQ( lease.toString(), "Hello World!" );
		EXPECT_EQ( builder.length(), 12 );

		// Character append
		builder.push_back( '?' );
		EXPECT_EQ( lease.toString(), "Hello World!?" );
		EXPECT_EQ( builder.length(), 13 );
	}

	TEST( StringBuilderBasicOperations, StreamOperators )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto builder{ lease.create() };

		// Fluent interface with stream operators
		builder << "Hello" << " " << "World" << "!";
		EXPECT_EQ( lease.toString(), "Hello World!" );

		// Clear and reuse
		lease.buffer().clear();
		EXPECT_TRUE( lease.buffer().isEmpty() );
		EXPECT_EQ( builder.length(), 0 );

		// Mixed stream and append operations
		builder << "Count: ";
		builder.append( "42" );
		builder << " items";
		EXPECT_EQ( lease.toString(), "Count: 42 items" );
	}

	TEST( StringBuilderBasicOperations, CapacityManagement )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto& buffer{ lease.buffer() };
		auto builder{ lease.create() };

		size_t initialCapacity{ buffer.capacity() };
		EXPECT_GT( initialCapacity, 0 );

		// Reserve larger capacity
		buffer.reserve( initialCapacity * 2 );
		EXPECT_GE( buffer.capacity(), initialCapacity * 2 );

		// Add content within reserved capacity
		std::string longContent( initialCapacity, 'x' );
		builder.append( longContent );
		EXPECT_EQ( builder.length(), initialCapacity );
		EXPECT_GE( buffer.capacity(), initialCapacity );
	}

	TEST( StringBuilderBasicOperations, DataAccess )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto& buffer{ lease.buffer() };
		auto builder{ lease.create() };

		builder.append( "Test Data" );

		// Data access
		const char* data{ buffer.data() };
		EXPECT_NE( data, nullptr );
		EXPECT_EQ( lease.toString(), "Test Data" );

		// String view access
		std::string_view view{ buffer.toStringView() };
		EXPECT_EQ( view, "Test Data" );
		EXPECT_EQ( view.size(), buffer.size() );

		// Index access
		EXPECT_EQ( builder[0], 'T' );
		EXPECT_EQ( builder[4], ' ' );
		EXPECT_EQ( builder[8], 'a' );

		// Modify via index
		builder[4] = '_';
		EXPECT_EQ( lease.toString(), "Test_Data" );
	}

	//----------------------------------------------
	// Iterator interface
	//----------------------------------------------

	TEST( StringBuilderIterator, BasicIteration )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto builder{ lease.create() };

		builder.append( "Hello" );

		// Range-based for loop
		std::string reconstructed;
		for ( char c : builder )
		{
			reconstructed += c;
		}
		EXPECT_EQ( reconstructed, "Hello" );

		// Manual iterator usage
		auto it{ builder.begin() };
		auto end{ builder.end() };
		EXPECT_NE( it, end );
		EXPECT_EQ( *it, 'H' );

		std::advance( it, 4 );
		EXPECT_EQ( *it, 'o' );

		it = std::next( it );
		EXPECT_EQ( it, end );
	}

	TEST( StringBuilderIterator, EmptyBuilderIteration )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto builder{ lease.create() };

		// Empty builder iteration
		auto begin_it{ builder.begin() };
		auto end_it{ builder.end() };
		EXPECT_EQ( begin_it, end_it );

		// Range-based for loop should not execute
		size_t count{ 0 };
		for ( [[maybe_unused]] char c : builder )
		{
			++count;
		}
		EXPECT_EQ( count, 0 );
	}

	TEST( StringBuilderIterator, STLIteratorInterface )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto builder{ lease.create() };
		builder.append( "hello" );

		// Test iterator types
		static_assert( std::is_same_v<string::StringBuilder::iterator, char*> );
		static_assert( std::is_same_v<string::StringBuilder::const_iterator, const char*> );
		static_assert( std::is_same_v<string::StringBuilder::value_type, char> );

		// Test begin/end iterators
		auto it_begin{ builder.begin() };
		auto it_end{ builder.end() };
		auto const_it_begin{ builder.begin() };
		auto const_it_end{ builder.end() };

		EXPECT_NE( it_begin, it_end );
		EXPECT_EQ( it_end - it_begin, 5 );
		EXPECT_EQ( const_it_end - const_it_begin, 5 );
	}

	TEST( StringBuilderIterator, RangeBasedForLoop )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto builder{ lease.create() };
		builder.append( "abc" );

		std::string result;
		for ( char c : builder )
		{
			result += c;
		}

		EXPECT_EQ( result, "abc" );
	}

	TEST( StringBuilderIterator, STLAlgorithmsIntegration )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto builder{ lease.create() };
		builder.append( "hello" );

		// Test std::find
		auto it{ std::find( builder.begin(), builder.end(), 'l' ) };
		EXPECT_NE( it, builder.end() );
		EXPECT_EQ( *it, 'l' );

		// Test std::count
		auto count{ std::count( builder.begin(), builder.end(), 'l' ) };
		EXPECT_EQ( count, 2 );

		// Test std::distance
		auto distance{ std::distance( builder.begin(), builder.end() ) };
		EXPECT_EQ( distance, 5 );
	}

	//----------------------------------------------
	// StringBuilder Enumerator
	//----------------------------------------------

	TEST( StringBuilderEnumerator, BasicEnumeration )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto builder{ lease.create() };

		builder.append( "Test" );

		// Get enumerator - construct directly
		string::StringBuilder::Enumerator enumerator{ builder };

		// Manual enumeration
		std::string result;
		while ( enumerator.next() )
		{
			result += enumerator.current();
		}
		EXPECT_EQ( result, "Test" );

		// Reset and enumerate again
		enumerator.reset();
		result.clear();
		while ( enumerator.next() )
		{
			result += enumerator.current();
		}
		EXPECT_EQ( result, "Test" );
	}

	TEST( StringBuilderEnumerator, EmptyBuilderEnumeration )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto builder{ lease.create() };

		string::StringBuilder::Enumerator enumerator{ builder };

		// Should not have any elements
		EXPECT_FALSE( enumerator.next() );

		// Reset should work on empty
		enumerator.reset();
		EXPECT_FALSE( enumerator.next() );
	}

	TEST( StringBuilderEnumerator, ResetFunctionality )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto builder{ lease.create() };
		builder.append( "abc" );

		string::StringBuilder::Enumerator enumerator{ builder };

		// Move to second character
		EXPECT_TRUE( enumerator.next() );
		EXPECT_TRUE( enumerator.next() );
		EXPECT_EQ( enumerator.current(), 'b' );

		// Reset and start over
		enumerator.reset();

		EXPECT_TRUE( enumerator.next() );
		EXPECT_EQ( enumerator.current(), 'a' );

		EXPECT_TRUE( enumerator.next() );
		EXPECT_EQ( enumerator.current(), 'b' );

		EXPECT_TRUE( enumerator.next() );
		EXPECT_EQ( enumerator.current(), 'c' );

		EXPECT_FALSE( enumerator.next() );
	}

	TEST( StringBuilderEnumerator, CSStyleUsage )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto builder{ lease.create() };
		builder.append( "hello" );

		string::StringBuilder::Enumerator enumerator{ builder };

		std::string result;
		while ( enumerator.next() )
		{
			result += enumerator.current();
		}

		EXPECT_EQ( result, "hello" );
	}

	TEST( StringBuilderEnumerator, AllIteratorTypesConsistency )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto builder{ lease.create() };
		builder.append( "consistency" );

		// STL iterator result
		std::string stl_result;
		for ( auto it{ builder.begin() }; it != builder.end(); it = std::next( it ) )
		{
			stl_result += *it;
		}

		// Range-based for result
		std::string range_result;
		for ( char c : builder )
		{
			range_result += c;
		}

		// Enumerator result
		std::string enum_result;
		string::StringBuilder::Enumerator enumerator{ builder };
		while ( enumerator.next() )
		{
			enum_result += enumerator.current();
		}

		// All should produce the same result
		EXPECT_EQ( stl_result, "consistency" );
		EXPECT_EQ( range_result, "consistency" );
		EXPECT_EQ( enum_result, "consistency" );
	}

	//----------------------------------------------
	// Performance and capacity optimization
	//----------------------------------------------

	TEST( StringBuilderPerformance, LargeStringBuilding )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto& buffer{ lease.buffer() };
		auto builder{ lease.create() };

		// Build large string efficiently
		const size_t iterations{ 1000 };
		const std::string segment{ "This is a test segment that will be repeated many times. " };

		// Reserve capacity upfront for better performance
		buffer.reserve( iterations * segment.size() );

		for ( size_t i{ 0 }; i < iterations; ++i )
		{
			builder << segment;
		}

		EXPECT_EQ( builder.length(), iterations * segment.size() );
		EXPECT_GE( buffer.capacity(), buffer.size() );

		// Verify content correctness
		std::string result{ lease.toString() };
		EXPECT_EQ( result.size(), iterations * segment.size() );

		// Check that first and last segments are correct
		EXPECT_TRUE( result.starts_with( segment ) );
		EXPECT_TRUE( result.ends_with( segment ) );
	}

	TEST( StringBuilderPerformance, ReuseOptimization )
	{
		string::StringBuilderPool::resetStats();

		// Multiple sequential uses should demonstrate reuse
		for ( int i{ 0 }; i < 10; ++i )
		{
			auto lease{ string::StringBuilderPool::lease() };
			auto builder{ lease.create() };
			builder.append( "Iteration " );
			builder.append( std::to_string( i ) );
			builder.append( " content" );

			// Use the string
			std::string result{ lease.toString() };
			EXPECT_FALSE( result.empty() );
		}

		auto stats{ string::StringBuilderPool::stats() };
		EXPECT_EQ( stats.totalRequests, 10 );

		// Should have some cache hits (exact numbers depend on thread-local caching)
		EXPECT_GT( stats.threadLocalHits + stats.dynamicStringBufferPoolHits, 0 );
	}

	//----------------------------------------------
	// Thread safety (basic verification)
	//----------------------------------------------

	TEST( StringBuilderThreadSafety, ConcurrentLeaseAcquisition )
	{
		const size_t numThreads{ 4 };
		const size_t iterationsPerThread{ 100 };

		std::vector<std::thread> threads;
		std::atomic<size_t> successCount{ 0 };

		// Clear stats for clean measurement
		string::StringBuilderPool::resetStats();

		for ( size_t t{ 0 }; t < numThreads; ++t )
		{
			threads.emplace_back( [&successCount]() {
				for ( size_t i{ 0 }; i < iterationsPerThread; ++i )
				{
					auto lease{ string::StringBuilderPool::lease() };
					auto builder{ lease.create() };

					builder.append( "Thread content " );
					builder.append( std::to_string( i ) );

					if ( !lease.toString().empty() )
					{
						++successCount;
					}
				}
			} );
		}

		// Wait for all threads to complete
		for ( auto& thread : threads )
		{
			thread.join();
		}

		// All operations should have succeeded
		EXPECT_EQ( successCount, numThreads * iterationsPerThread );

		// Verify total requests match
		auto stats{ string::StringBuilderPool::stats() };
		EXPECT_EQ( stats.totalRequests, numThreads * iterationsPerThread );
	}

	//----------------------------------------------
	// Integration with string_view
	//----------------------------------------------

	TEST( StringBuilderIntegration, StringViewIntegration )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto& buffer{ lease.buffer() };
		auto builder{ lease.create() };

		// Build string
		builder << "Hello" << " " << "World";

		// Get string_view without allocation
		std::string_view view{ buffer.toStringView() };
		EXPECT_EQ( view, "Hello World" );
		EXPECT_EQ( view.size(), buffer.size() );

		// String view should point to same data
		EXPECT_EQ( view.data(), buffer.data() );

		// Modify builder and verify view is updated
		builder.push_back( '!' );
		std::string_view newView{ buffer.toStringView() };
		EXPECT_EQ( newView, "Hello World!" );
	}

	//----------------------------------------------
	// Memory efficiency validation
	//----------------------------------------------

	TEST( StringBuilderMemoryEfficiency, PoolingBenefit )
	{
		// This test demonstrates the pooling benefit by measuring allocations
		string::StringBuilderPool::clear();
		string::StringBuilderPool::resetStats();

		// First acquisition - should result in new allocation
		{
			auto lease{ string::StringBuilderPool::lease() };
			auto builder{ lease.create() };
			builder.append( "First use" );
		}

		auto statsAfterFirst{ string::StringBuilderPool::stats() };
		EXPECT_EQ( statsAfterFirst.totalRequests, 1 );
		EXPECT_GT( statsAfterFirst.newAllocations, 0 );

		// Second acquisition - should potentially reuse
		{
			auto lease{ string::StringBuilderPool::lease() };
			auto builder{ lease.create() };
			builder.append( "Second use" );
		}

		auto statsAfterSecond{ string::StringBuilderPool::stats() };
		EXPECT_EQ( statsAfterSecond.totalRequests, 2 );

		// Should demonstrate reuse (thread-local cache hit)
		EXPECT_GT( statsAfterSecond.threadLocalHits + statsAfterSecond.dynamicStringBufferPoolHits, 0 );
	}

	TEST( StringBuilderMemoryEfficiency, CapacityRetention )
	{
		const size_t testCapacity{ 1000 };

		// Build large string to establish capacity
		{
			auto lease{ string::StringBuilderPool::lease() };
			auto& buffer{ lease.buffer() };
			auto builder{ lease.create() };

			std::string largeContent( testCapacity, 'x' );
			builder.append( largeContent );
			EXPECT_GE( buffer.capacity(), testCapacity );
		}

		// Acquire new lease - should potentially retain capacity
		{
			auto lease{ string::StringBuilderPool::lease() };
			auto& buffer{ lease.buffer() };

			// Buffer should be clean but may retain capacity
			EXPECT_TRUE( buffer.isEmpty() );
			// Note: Capacity retention depends on pool configuration
		}
	}

	//----------------------------------------------
	// StringBuilder advanced
	//----------------------------------------------

	TEST( StringBuilderAdvanced, ArrayAccessOperators )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto builder{ lease.create() };

		builder.append( "test" );

		// Test read access
		EXPECT_EQ( builder[0], 't' );
		EXPECT_EQ( builder[1], 'e' );
		EXPECT_EQ( builder[2], 's' );
		EXPECT_EQ( builder[3], 't' );

		// Test write access
		builder[1] = 'a';
		builder[2] = 'x';
		builder[3] = 'i';

		EXPECT_EQ( lease.toString(), "taxi" );

		// Test const access
		const auto& constBuilder{ builder };
		EXPECT_EQ( constBuilder[0], 't' );
		EXPECT_EQ( constBuilder[1], 'a' );
		EXPECT_EQ( constBuilder[2], 'x' );
		EXPECT_EQ( constBuilder[3], 'i' );
	}

	TEST( StringBuilderAdvanced, StreamOperatorChaining )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto builder{ lease.create() };

		// Test comprehensive stream operator chaining
		builder << "Hello" << ", " << std::string( "C++" ) << '!';
		EXPECT_EQ( builder.length(), 11 );
		EXPECT_EQ( lease.toString(), "Hello, C++!" );

		// Mix stream operators with regular methods
		lease.buffer().clear();
		builder << "Start";
		builder.append( " middle" );
		builder << " end";
		EXPECT_EQ( lease.toString(), "Start middle end" );
	}

	TEST( StringBuilderAdvanced, ResizeOperations )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto builder{ lease.create() };

		builder.append( "Hello" );
		EXPECT_EQ( builder.length(), 5 );

		// Resize larger
		builder.resize( 10 );
		EXPECT_EQ( builder.length(), 10 );

		// Resize smaller
		builder.resize( 3 );
		EXPECT_EQ( builder.length(), 3 );
		EXPECT_EQ( lease.toString(), "Hel" );
	}

	//----------------------------------------------
	// Edge cases and error handling
	//----------------------------------------------

	TEST( StringBuilderEdgeCases, EmptyStringOperations )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto& buffer{ lease.buffer() };
		auto builder{ lease.create() };

		// Operations on empty builder
		EXPECT_TRUE( buffer.isEmpty() );
		EXPECT_EQ( buffer.size(), 0 );
		EXPECT_EQ( lease.toString(), "" );
		EXPECT_EQ( buffer.toStringView(), "" );
		EXPECT_NE( buffer.data(), nullptr );

		// Clear empty builder
		buffer.clear();
		EXPECT_TRUE( buffer.isEmpty() );

		// Append empty string
		builder.append( "" );
		EXPECT_TRUE( buffer.isEmpty() );

		builder << "";
		EXPECT_TRUE( buffer.isEmpty() );
	}

	TEST( StringBuilderEdgeCases, LargeCapacityRequests )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto& buffer{ lease.buffer() };
		auto builder{ lease.create() };

		// Very large capacity request
		const size_t largeCapacity{ 1000000 };
		buffer.reserve( largeCapacity );
		EXPECT_GE( buffer.capacity(), largeCapacity );

		// Should still work normally
		builder.append( "Test with large capacity" );
		EXPECT_EQ( lease.toString(), "Test with large capacity" );
	}

	//----------------------------------------------
	// StringBuilderLease advanced
	//----------------------------------------------

	TEST( StringBuilderLeaseAdvanced, MoveSemantics )
	{
		auto lease1{ string::StringBuilderPool::lease() };
		lease1.create().append( "test data" );

		// Move construction
		auto lease2{ std::move( lease1 ) };

		// lease2 should be valid
		EXPECT_NO_THROW( [[maybe_unused]] auto builder = lease2.create() );
		EXPECT_EQ( lease2.toString(), "test data" );

		// Move assignment
		auto lease3{ string::StringBuilderPool::lease() };
		lease3.create().append( "original data" );
		auto lease4{ string::StringBuilderPool::lease() };
		lease4.create().append( "new data" );

		lease3 = std::move( lease4 );
		EXPECT_EQ( lease3.toString(), "new data" );
	}

	TEST( StringBuilderLeaseAdvanced, SelfMoveAssignment )
	{
		auto lease{ string::StringBuilderPool::lease() };
		lease.create().append( "self move test" );

		// Self move assignment should be safe
		// Intentional self-move for testing - suppress compiler warnings
#if defined( __clang__ )
#	pragma clang diagnostic push
#	pragma clang diagnostic ignored "-Wself-move"
#elif defined( __GNUC__ )
#	pragma GCC diagnostic push
#	pragma GCC diagnostic ignored "-Wself-move"
#	pragma GCC diagnostic ignored "-Wpessimizing-move"
#elif defined( _MSC_VER )
#	pragma warning( push )
#	pragma warning( disable : 4996 )
#endif
		lease = std::move( lease );
#if defined( __clang__ )
#	pragma clang diagnostic pop
#elif defined( __GNUC__ )
#	pragma GCC diagnostic pop
#elif defined( _MSC_VER )
#	pragma warning( pop )
#endif

		// Lease should still be valid
		EXPECT_NO_THROW( [[maybe_unused]] auto builder = lease.create() );
		EXPECT_EQ( lease.toString(), "self move test" );
	}

	TEST( StringBuilderLeaseAdvanced, MultipleBuilderCalls )
	{
		auto lease{ string::StringBuilderPool::lease() };

		// Multiple builder() calls should return wrappers to same buffer
		auto builder1{ lease.create() };
		auto builder2{ lease.create() };

		builder1.append( "first" );
		EXPECT_EQ( builder2.length(), 5 ); // Should see changes from builder1

		builder2.append( " second" );
		EXPECT_EQ( builder1.length(), 12 ); // Should see changes from builder2

		EXPECT_EQ( lease.toString(), "first second" );
	}

	TEST( StringBuilderLeaseAdvanced, BufferAndBuilderInteraction )
	{
		auto lease{ string::StringBuilderPool::lease() };

		// Mix buffer and builder operations
		auto& buffer{ lease.buffer() };
		auto builder{ lease.create() };

		buffer.push_back( 'X' );
		builder.append( "YZ" );
		buffer.push_back( '!' );

		EXPECT_EQ( lease.toString(), "XYZ!" );
		EXPECT_EQ( buffer.size(), 4 );
		EXPECT_EQ( builder.length(), 4 );
	}

	TEST( StringBuilderLeaseAdvanced, RAIICleanup )
	{
		// Reset statistics for clean measurement
		string::StringBuilderPool::resetStats();

		{
			auto lease{ string::StringBuilderPool::lease() };
			lease.create().append( "RAII test" );
			// Lease goes out of scope here - should return buffer to pool
		}

		// Next lease should get recycled buffer from thread-local cache
		auto newLease{ string::StringBuilderPool::lease() };
		EXPECT_EQ( newLease.toString(), "" ); // Buffer should be cleared

		const auto& stats{ string::StringBuilderPool::stats() };
		EXPECT_GT( stats.threadLocalHits, 0 ); // Should have hit cache
	}

	//=====================================================================
	// DynamicStringBuffer
	//=====================================================================

	//----------------------------------------------
	// Construction and Destruction
	//----------------------------------------------

	TEST( DynamicStringBufferConstruction, DefaultConstruction )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto& buffer{ lease.buffer() };

		// Test initial state
		EXPECT_EQ( buffer.size(), 0 );
		EXPECT_GE( buffer.capacity(), 256 ); // Should have at least initial capacity
		EXPECT_TRUE( buffer.isEmpty() );
		EXPECT_EQ( lease.toString(), "" );
	}

	TEST( DynamicStringBufferConstruction, CopyConstruction )
	{
		auto lease1{ string::StringBuilderPool::lease() };
		auto& buffer1{ lease1.buffer() };
		buffer1.append( "Copy test data" );

		auto lease2{ string::StringBuilderPool::lease() };
		auto& buffer2{ lease2.buffer() };

		buffer2 = buffer1;

		EXPECT_EQ( buffer2.size(), 14 );
		EXPECT_EQ( buffer2.toString(), "Copy test data" );
		EXPECT_EQ( buffer1.toString(), "Copy test data" ); // Original unchanged
	}

	TEST( DynamicStringBufferConstruction, MoveConstruction )
	{
		auto lease1{ string::StringBuilderPool::lease() };
		auto& buffer1{ lease1.buffer() };
		buffer1.append( "Move test data" );

		auto lease2{ string::StringBuilderPool::lease() };
		auto& buffer2{ lease2.buffer() };

		buffer2 = std::move( buffer1 );

		EXPECT_EQ( buffer2.size(), 14 );
		EXPECT_EQ( buffer2.toString(), "Move test data" );
	}

	//----------------------------------------------
	// CapacityAndSize Management
	//----------------------------------------------

	TEST( DynamicStringBufferCapacityAndSize, SizeAndCapacity )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto& buffer{ lease.buffer() };

		// Initial state
		EXPECT_EQ( buffer.size(), 0 );
		EXPECT_GE( buffer.capacity(), 256 );
		EXPECT_TRUE( buffer.isEmpty() );

		// Add content
		buffer.append( "test" );
		EXPECT_EQ( buffer.size(), 4 );
		EXPECT_FALSE( buffer.isEmpty() );
		EXPECT_GE( buffer.capacity(), 4 );
	}

	TEST( DynamicStringBufferCapacityAndSize, Reserve )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto& buffer{ lease.buffer() };

		// Reserve larger capacity
		buffer.reserve( 1024 );
		EXPECT_GE( buffer.capacity(), 1024 );
		EXPECT_EQ( buffer.size(), 0 ); // Size unchanged

		// Reserve smaller capacity (should not shrink)
		buffer.reserve( 512 );
		EXPECT_GE( buffer.capacity(), 1024 ); // Capacity should not decrease
	}

	TEST( DynamicStringBufferCapacityAndSize, Resize )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto& buffer{ lease.buffer() };

		buffer.append( "Hello World" );
		EXPECT_EQ( buffer.size(), 11 );

		// Resize larger
		buffer.resize( 20 );
		EXPECT_EQ( buffer.size(), 20 );
		EXPECT_GE( buffer.capacity(), 20 );

		// Resize smaller
		buffer.resize( 5 );
		EXPECT_EQ( buffer.size(), 5 );
		EXPECT_EQ( lease.toString(), "Hello" );

		// Resize to zero
		buffer.resize( 0 );
		EXPECT_EQ( buffer.size(), 0 );
		EXPECT_TRUE( buffer.isEmpty() );
	}

	TEST( DynamicStringBufferCapacityAndSize, Clear )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto& buffer{ lease.buffer() };

		buffer.append( "Clear test data" );
		size_t originalCapacity{ buffer.capacity() };

		EXPECT_EQ( buffer.size(), 15 );
		EXPECT_FALSE( buffer.isEmpty() );

		buffer.clear();

		EXPECT_EQ( buffer.size(), 0 );
		EXPECT_TRUE( buffer.isEmpty() );
		EXPECT_EQ( buffer.capacity(), originalCapacity ); // Capacity preserved
		EXPECT_EQ( buffer.toString(), "" );
	}

	//----------------------------------------------
	// Data Access
	//----------------------------------------------

	TEST( DynamicStringBufferDataAccess, DataAccess )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto& buffer{ lease.buffer() };

		buffer.append( "Data access test" );

		// Test const data access
		const char* constData{ buffer.data() };
		EXPECT_NE( constData, nullptr );
		EXPECT_EQ( *constData, 'D' );
		EXPECT_EQ( *std::next( constData, 4 ), ' ' );

		// Test mutable data access
		char* mutableData{ buffer.data() };
		EXPECT_NE( mutableData, nullptr );
		EXPECT_EQ( mutableData, constData ); // Should point to same memory

		// Modify through mutable pointer
		*mutableData = 'M';
		EXPECT_EQ( lease.toString(), "Mata access test" );
	}

	TEST( DynamicStringBufferDataAccess, ArrayAccess )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto& buffer{ lease.buffer() };

		buffer.append( "Array" );

		// Test read access
		EXPECT_EQ( buffer[0], 'A' );
		EXPECT_EQ( buffer[1], 'r' );
		EXPECT_EQ( buffer[2], 'r' );
		EXPECT_EQ( buffer[3], 'a' );
		EXPECT_EQ( buffer[4], 'y' );

		// Test write access
		buffer[1] = 'R';
		buffer[2] = 'R';
		EXPECT_EQ( lease.toString(), "ARRay" );

		// Test const access
		const auto& constBuffer{ buffer };
		EXPECT_EQ( constBuffer[0], 'A' );
		EXPECT_EQ( constBuffer[1], 'R' );
	}

	//----------------------------------------------
	// Content Manipulation
	//----------------------------------------------

	TEST( DynamicStringBufferManipulation, AppendStringView )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto& buffer{ lease.buffer() };

		std::string_view testStr{ "StringView test" };
		buffer.append( testStr );

		EXPECT_EQ( buffer.size(), 15 );
		EXPECT_EQ( lease.toString(), "StringView test" );
	}

	TEST( DynamicStringBufferManipulation, AppendStdString )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto& buffer{ lease.buffer() };

		std::string testStr{ "StdString test" };
		buffer.append( testStr );

		EXPECT_EQ( buffer.size(), 14 );
		EXPECT_EQ( lease.toString(), "StdString test" );
	}

	TEST( DynamicStringBufferManipulation, AppendCString )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto& buffer{ lease.buffer() };

		const char* testStr{ "CString test" };
		buffer.append( testStr );

		EXPECT_EQ( buffer.size(), 12 );
		EXPECT_EQ( lease.toString(), "CString test" );
	}

	TEST( DynamicStringBufferManipulation, AppendNullCString )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto& buffer{ lease.buffer() };

		// Should handle null gracefully
		buffer.append( static_cast<const char*>( nullptr ) );

		EXPECT_EQ( buffer.size(), 0 );
		EXPECT_EQ( lease.toString(), "" );
	}

	TEST( DynamicStringBufferManipulation, PushBack )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto& buffer{ lease.buffer() };

		buffer.push_back( 'A' );
		buffer.push_back( 'B' );
		buffer.push_back( 'C' );

		EXPECT_EQ( buffer.size(), 3 );
		EXPECT_EQ( lease.toString(), "ABC" );
	}

	TEST( DynamicStringBufferManipulation, MultipleAppends )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto& buffer{ lease.buffer() };

		buffer.append( "Hello" );
		buffer.append( std::string( ", " ) );
		buffer.append( std::string_view( "World" ) );
		buffer.push_back( '!' );

		EXPECT_EQ( buffer.size(), 13 );
		EXPECT_EQ( lease.toString(), "Hello, World!" );
	}

	//----------------------------------------------
	// String Conversion
	//----------------------------------------------

	TEST( DynamicStringBufferConversion, ToString )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto& buffer{ lease.buffer() };

		// Empty buffer
		EXPECT_EQ( lease.toString(), "" );

		// With content
		buffer.append( "ToString test" );
		std::string result{ lease.toString() };
		EXPECT_EQ( result, "ToString test" );

		// Multiple calls should return same result
		EXPECT_EQ( lease.toString(), "ToString test" );
		EXPECT_EQ( lease.toString(), result );
	}

	TEST( DynamicStringBufferConversion, ToStringView )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto& buffer{ lease.buffer() };

		// Empty buffer
		std::string_view emptyView{ buffer.toStringView() };
		EXPECT_EQ( emptyView.size(), 0 );
		EXPECT_TRUE( emptyView.empty() );

		// With content
		buffer.append( "StringView test" );
		std::string_view view{ buffer.toStringView() };
		EXPECT_EQ( view.size(), 15 );
		EXPECT_EQ( view, "StringView test" );

		// View should reference same memory as buffer
		EXPECT_EQ( view.data(), buffer.data() );
	}

	//----------------------------------------------
	// Iterator Interface
	//----------------------------------------------

	TEST( DynamicStringBufferIteration, IteratorTypes )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto builder{ lease.create() };
		builder.append( "hello" );

		// Test iterator types
		static_assert( std::is_same_v<string::StringBuilder::iterator, char*> );
		static_assert( std::is_same_v<string::StringBuilder::const_iterator, const char*> );
		static_assert( std::is_same_v<string::StringBuilder::value_type, char> );

		// Test begin/end iterators
		auto it_begin{ builder.begin() };
		auto it_end{ builder.end() };
		auto const_it_begin{ builder.begin() };
		auto const_it_end{ builder.end() };

		EXPECT_NE( it_begin, it_end );
		EXPECT_EQ( it_end - it_begin, 5 );
		EXPECT_EQ( const_it_end - const_it_begin, 5 );
	}

	TEST( DynamicStringBufferIteration, BeginEndIterators )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto& buffer{ lease.buffer() };
		buffer.append( "iterator" );

		// Test mutable iterators
		auto it_begin{ buffer.begin() };
		auto it_end{ buffer.end() };
		EXPECT_NE( it_begin, it_end );
		EXPECT_EQ( it_end - it_begin, 8 );

		// Test const iterators
		const auto& constBuffer{ buffer };
		auto const_it_begin{ constBuffer.begin() };
		auto const_it_end{ constBuffer.end() };
		EXPECT_NE( const_it_begin, const_it_end );
		EXPECT_EQ( const_it_end - const_it_begin, 8 );
	}

	TEST( DynamicStringBufferIteration, RangeBasedFor )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto& buffer{ lease.buffer() };
		buffer.append( "range" );

		std::string result;
		for ( char c : buffer )
		{
			result += c;
		}

		EXPECT_EQ( result, "range" );
	}

	//----------------------------------------------
	// DynamicStringBuffer advanced
	//----------------------------------------------

	TEST( DynamicStringBufferAdvanced, STLAlgorithmIntegration )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto& buffer{ lease.buffer() };
		auto builder{ lease.create() };

		builder.append( "algorithm" );

		// Test std::find
		auto it{ std::find( buffer.begin(), buffer.end(), 'o' ) };
		EXPECT_NE( it, buffer.end() );
		EXPECT_EQ( *it, 'o' );

		// Test std::count
		auto count{ std::count( buffer.begin(), buffer.end(), 'a' ) };
		EXPECT_EQ( count, 1 );

		// Test std::distance
		auto distance{ std::distance( buffer.begin(), buffer.end() ) };
		EXPECT_EQ( distance, 9 );

		// Test std::reverse
		std::reverse( buffer.begin(), buffer.end() );
		EXPECT_EQ( lease.toString(), "mhtirogla" );
	}

	TEST( DynamicStringBufferAdvanced, SpecialCharacterHandling )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto& buffer{ lease.buffer() };
		auto builder{ lease.create() };

		// Test special characters including null
		builder.append( "Special: \n\t\r\"\\'" );
		builder.push_back( '\0' ); // Null character
		builder.append( " End" );

		EXPECT_EQ( buffer.size(), 20 ); // 15 + 1 + 4

		// Check specific special characters
		EXPECT_EQ( builder[9], '\n' );
		EXPECT_EQ( builder[10], '\t' );
		EXPECT_EQ( builder[11], '\r' );
		EXPECT_EQ( builder[15], '\0' );

		// Verify the end is accessible
		std::string_view view{ buffer.toStringView() };
		EXPECT_EQ( view.size(), 20 );
	}

	TEST( DynamicStringBufferAdvanced, CapacityGrowthPattern )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto& buffer{ lease.buffer() };
		auto builder{ lease.create() };

		size_t initialCapacity{ buffer.capacity() };

		// Gradually grow content beyond initial capacity
		std::string expectedContent;
		const size_t iterations{ static_cast<size_t>( initialCapacity / 5 ) + 50 }; // Ensure we exceed initial capacity
		for ( size_t i{ 0 }; i < iterations; ++i )
		{
			builder.append( "grows" );
			expectedContent += "grows";
		}

		size_t expectedSize{ iterations * 5 };
		EXPECT_EQ( buffer.size(), expectedSize );
		EXPECT_GE( buffer.capacity(), expectedSize );
		EXPECT_GE( buffer.capacity(), initialCapacity ); // Should be at least initial capacity
		EXPECT_EQ( lease.toString(), expectedContent );
	}

	TEST( DynamicStringBufferAdvanced, MemoryConsistencyValidation )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto& buffer{ lease.buffer() };
		auto builder{ lease.create() };

		builder.append( "consistency" );

		// Data pointer should remain stable during read operations
		const char* data1{ buffer.data() };
		std::string toString1{ lease.toString() };
		std::string_view toView1{ buffer.toStringView() };
		const char* data2{ buffer.data() };

		EXPECT_EQ( data1, data2 );
		EXPECT_EQ( toString1, "consistency" );
		EXPECT_EQ( toView1, "consistency" );
		EXPECT_EQ( toView1.data(), data1 );
	}

	TEST( DynamicStringBufferAdvanced, ResizeOperations )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto& buffer{ lease.buffer() };
		auto builder{ lease.create() };

		builder.append( "Hello World" );
		EXPECT_EQ( buffer.size(), 11 );

		// Resize larger
		buffer.resize( 20 );
		EXPECT_EQ( buffer.size(), 20 );
		EXPECT_GE( buffer.capacity(), 20 );

		// Resize smaller
		buffer.resize( 5 );
		EXPECT_EQ( buffer.size(), 5 );
		EXPECT_EQ( lease.toString(), "Hello" );

		// Resize to zero
		buffer.resize( 0 );
		EXPECT_EQ( buffer.size(), 0 );
		EXPECT_TRUE( buffer.isEmpty() );
	}

	TEST( DynamicStringBufferAdvanced, ClearOperations )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto& buffer{ lease.buffer() };

		buffer.append( "Clear test data" );
		size_t originalCapacity{ buffer.capacity() };

		EXPECT_EQ( buffer.size(), 15 );
		EXPECT_FALSE( buffer.isEmpty() );

		buffer.clear();

		EXPECT_EQ( buffer.size(), 0 );
		EXPECT_TRUE( buffer.isEmpty() );
		EXPECT_EQ( buffer.capacity(), originalCapacity ); // Capacity preserved
		EXPECT_EQ( lease.toString(), "" );
	}

	TEST( DynamicStringBufferAdvanced, LargeContentHandling )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto& buffer{ lease.buffer() };

		// Create large content (exceeds initial capacity)
		std::string largeContent( 5000, 'L' );
		buffer.append( largeContent );

		EXPECT_EQ( buffer.size(), 5000 );
		EXPECT_GE( buffer.capacity(), 5000 );
		EXPECT_EQ( lease.toString(), largeContent );
	}

	TEST( DynamicStringBufferAdvanced, ToStringViewOperations )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto& buffer{ lease.buffer() };

		// Empty buffer
		std::string_view emptyView{ buffer.toStringView() };
		EXPECT_EQ( emptyView.size(), 0 );
		EXPECT_TRUE( emptyView.empty() );

		// With content
		buffer.append( "StringView test" );
		std::string_view view{ buffer.toStringView() };
		EXPECT_EQ( view.size(), 15 );
		EXPECT_EQ( view, "StringView test" );

		// View should reference same memory as buffer
		EXPECT_EQ( view.data(), buffer.data() );
	}

	TEST( DynamicStringBufferAdvanced, PerformanceValidation )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto& buffer{ lease.buffer() };

		// Performance test: many small appends
		for ( int i{ 0 }; i < 1000; ++i )
		{
			buffer.append( "perf" );
		}

		EXPECT_EQ( buffer.size(), 4000 );

		// Verify content pattern
		std::string result{ lease.toString() };
		EXPECT_EQ( result.substr( 0, 8 ), "perfperf" );
		EXPECT_EQ( result.substr( 3992, 8 ), "perfperf" );
	}

	TEST( DynamicStringBufferAdvanced, CopySemantics )
	{
		auto lease1{ string::StringBuilderPool::lease() };
		auto& buffer1{ lease1.buffer() };
		buffer1.append( "Copy test data" );

		auto lease2{ string::StringBuilderPool::lease() };
		auto& buffer2{ lease2.buffer() };

		// Copy assignment
		buffer2 = buffer1;
		EXPECT_EQ( buffer2.size(), 14 );
		EXPECT_EQ( lease2.toString(), "Copy test data" );
		EXPECT_EQ( lease1.toString(), "Copy test data" ); // Original unchanged

		// Modify copy
		buffer2.append( " modified" );
		EXPECT_EQ( lease2.toString(), "Copy test data modified" );
		EXPECT_EQ( lease1.toString(), "Copy test data" ); // Original still unchanged
	}

	TEST( DynamicStringBufferAdvanced, ReserveOperations )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto& buffer{ lease.buffer() };

		size_t initialCapacity{ buffer.capacity() };

		// Reserve larger capacity
		buffer.reserve( 1024 );
		EXPECT_GE( buffer.capacity(), 1024 );
		EXPECT_GE( buffer.capacity(), initialCapacity ); // Should be at least initial capacity
		EXPECT_EQ( buffer.size(), 0 );					 // Size unchanged

		// Reserve smaller capacity (should not shrink)
		buffer.reserve( 512 );
		EXPECT_GE( buffer.capacity(), 1024 ); // Capacity should not decrease

		// Add content within reserved capacity
		std::string testContent( 800, 'R' );
		buffer.append( testContent );
		EXPECT_EQ( buffer.size(), 800 );
		EXPECT_GE( buffer.capacity(), 1024 );
	}

	TEST( DynamicStringBufferAdvanced, NullPointerHandling )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto builder{ lease.create() };

		// Should handle null C-string gracefully
		builder.append( static_cast<const char*>( nullptr ) );
		EXPECT_EQ( builder.length(), 0 );
		EXPECT_EQ( lease.toString(), "" );

		// Stream operator should also handle null
		builder << static_cast<const char*>( nullptr );
		EXPECT_EQ( builder.length(), 0 );
		EXPECT_EQ( lease.toString(), "" );
	}

	TEST( DynamicStringBufferAdvanced, EmptyOperations )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto& buffer{ lease.buffer() };

		// Empty string operations
		buffer.append( "" );
		EXPECT_EQ( buffer.size(), 0 );
		EXPECT_TRUE( buffer.isEmpty() );

		buffer.append( std::string() );
		EXPECT_EQ( buffer.size(), 0 );

		buffer.append( std::string_view() );
		EXPECT_EQ( buffer.size(), 0 );
	}

	TEST( DynamicStringBufferAdvanced, LargeContent )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto& buffer{ lease.buffer() };

		// Create large content (exceeds initial capacity)
		std::string largeContent( 5000, 'L' );
		buffer.append( largeContent );

		EXPECT_EQ( buffer.size(), 5000 );
		EXPECT_GE( buffer.capacity(), 5000 );
		EXPECT_EQ( lease.toString(), largeContent );
	}

	TEST( DynamicStringBufferAdvanced, SpecialCharacters )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto& buffer{ lease.buffer() };

		// Test special characters
		buffer.append( "Special: \n\t\r\"\\'" ); // 8 + 1 + 1 + 1 + 1 + 1 + 1 + 1 = 15
		buffer.push_back( '\0' );				 // Null character = 1
		buffer.append( " End" );				 // 1 + 3 = 4

		EXPECT_EQ( buffer.size(), 20 ); // 15 + 1 + 4

		// Check specific characters
		EXPECT_EQ( buffer[9], '\n' );
		EXPECT_EQ( buffer[10], '\t' );
		EXPECT_EQ( buffer[11], '\r' );
		EXPECT_EQ( buffer[15], '\0' );
	}

	TEST( DynamicStringBufferAdvanced, CapacityGrowth )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto& buffer{ lease.buffer() };

		size_t initialCapacity{ buffer.capacity() };

		// Gradually grow content
		std::string content;
		for ( int i{ 0 }; i < 100; ++i )
		{
			content += "growth";
			buffer.append( "growth" );
		}

		EXPECT_EQ( buffer.size(), 600 ); // 100 * 6
		EXPECT_GE( buffer.capacity(), 600 );
		EXPECT_GT( buffer.capacity(), initialCapacity );
		EXPECT_EQ( lease.toString(), content );
	}

	TEST( DynamicStringBufferAdvanced, Performance )
	{
		auto lease{ string::StringBuilderPool::lease() };
		auto& buffer{ lease.buffer() };

		// Performance test: many small appends
		for ( int i{ 0 }; i < 1000; ++i )
		{
			buffer.append( "perf" );
		}

		EXPECT_EQ( buffer.size(), 4000 );

		// Verify content pattern
		std::string result{ lease.toString() };
		EXPECT_EQ( result.substr( 0, 8 ), "perfperf" );
		EXPECT_EQ( result.substr( 3992, 8 ), "perfperf" );
	}

	TEST( DynamicStringBufferAdvanced, Assignment )
	{
		auto lease1{ string::StringBuilderPool::lease() };
		auto& buffer1{ lease1.buffer() };
		buffer1.append( "Source buffer" );

		auto lease2{ string::StringBuilderPool::lease() };
		auto& buffer2{ lease2.buffer() };
		buffer2.append( "Target buffer" );

		// Copy assignment
		buffer2 = buffer1;
		EXPECT_EQ( buffer2.toString(), "Source buffer" );
		EXPECT_EQ( buffer1.toString(), "Source buffer" ); // Source unchanged

		// Modify copy
		buffer2.append( " modified" );
		EXPECT_EQ( buffer2.toString(), "Source buffer modified" );
		EXPECT_EQ( buffer1.toString(), "Source buffer" ); // Source still unchanged
	}
} // namespace nfx::string::test
