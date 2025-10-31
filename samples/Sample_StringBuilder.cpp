/**
 * @file Sample_StringBuilder.cpp
 * @brief Demonstrates high-performance string building with StringBuilderPool
 * @details This sample shows how to use StringBuilderPool for zero-allocation string operations,
 *          including pooled buffers, efficient concatenation, streaming operations,
 *          and enterprise-grade string building patterns for maximum performance
 */

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <nfx/string/StringBuilderPool.h>

using namespace nfx::string;

int main()
{
	std::cout << "=== NFX C++ Core - StringBuilderPool Usage ===" << std::endl;
	std::cout << std::endl;

	//=========================================================================
	// Basic StringBuilderPool usage
	//=========================================================================

	std::cout << "--- Basic StringBuilder Operations ---" << std::endl;

	// Acquire a lease from the pool
	auto lease{ StringBuilderPool::lease() };
	auto builder{ lease.create() };

	// Basic string building
	builder.append( "Hello" );
	builder.append( ", " );
	builder.append( "World" );
	builder.push_back( '!' );

	std::cout << "Basic concatenation: " << lease.toString() << std::endl;

	// Clear and reuse the same buffer
	lease.buffer().clear();
	std::cout << "Buffer is empty after clear: " << ( lease.buffer().isEmpty() ? "Yes" : "No" ) << std::endl;

	// Stream operators for fluent interface
	builder << "Stream " << "operators " << "are " << "convenient!";
	std::cout << "Stream operators: " << lease.toString() << std::endl;

	std::cout << "Buffer capacity: " << lease.buffer().capacity() << " characters" << std::endl;
	std::cout << "Buffer size: " << lease.buffer().size() << " characters" << std::endl;
	std::cout << std::endl;

	//=========================================================================
	// Performance comparison with std::string
	//=========================================================================

	std::cout << "--- Performance Comparison ---" << std::endl;

	const int iterations{ 1000 };
	const std::string segment{ "Performance test segment " };

	// StringBuilder performance test
	auto startSb{ std::chrono::high_resolution_clock::now() };
	{
		auto perfLease{ StringBuilderPool::lease() };
		auto perfBuilder{ perfLease.create() };

		// Reserve capacity for better performance
		perfLease.buffer().reserve( iterations * segment.size() );

		for ( int i{ 0 }; i < iterations; ++i )
		{
			perfBuilder << segment << std::to_string( i ) << " ";
		}

		auto result{ perfLease.toString() };
		std::cout << "StringBuilder result length: " << result.size() << " characters" << std::endl;
	}
	auto endSb{ std::chrono::high_resolution_clock::now() };
	auto durationSb{ std::chrono::duration_cast<std::chrono::microseconds>( endSb - startSb ) };

	// std::string performance test
	auto startStr{ std::chrono::high_resolution_clock::now() };
	{
		std::string result;
		result.reserve( iterations * ( segment.size() + 10 ) );

		for ( int i{ 0 }; i < iterations; ++i )
		{
			result += segment + std::to_string( i ) + " ";
		}

		std::cout << "std::string result length: " << result.size() << " characters" << std::endl;
	}
	auto endStr{ std::chrono::high_resolution_clock::now() };
	auto durationStr{ std::chrono::duration_cast<std::chrono::microseconds>( endStr - startStr ) };

	// std::ostringstream performance test
	auto startOss{ std::chrono::high_resolution_clock::now() };
	{
		std::ostringstream oss;

		for ( int i{ 0 }; i < iterations; ++i )
		{
			oss << segment << i << " ";
		}

		auto result{ oss.str() };
		std::cout << "std::ostringstream result length: " << result.size() << " characters" << std::endl;
	}
	auto endOss{ std::chrono::high_resolution_clock::now() };
	auto durationOss{ std::chrono::duration_cast<std::chrono::microseconds>( endOss - startOss ) };

	std::cout << std::endl
			  << "Performance Results (" << iterations << " iterations):" << std::endl;
	std::cout << "  StringBuilder:     " << std::setw( 6 ) << durationSb.count() << " μs" << std::endl;
	std::cout << "  std::string:       " << std::setw( 6 ) << durationStr.count() << " μs" << std::endl;
	std::cout << "  std::ostringstream:" << std::setw( 6 ) << durationOss.count() << " μs" << std::endl;

	if ( durationSb.count() > 0 )
	{
		std::cout << "  StringBuilder speedup vs std::string: "
				  << std::fixed << std::setprecision( 1 )
				  << ( static_cast<double>( durationStr.count() ) / static_cast<double>( durationSb.count() ) ) << "x" << std::endl;
		std::cout << "  StringBuilder speedup vs ostringstream: "
				  << std::fixed << std::setprecision( 1 )
				  << ( static_cast<double>( durationOss.count() ) / static_cast<double>( durationSb.count() ) ) << "x" << std::endl;
	}
	std::cout << std::endl;

	//=========================================================================
	// Pool statistics and reuse demonstration
	//=========================================================================

	std::cout << "--- Pool Statistics and Reuse ---" << std::endl;

	// Reset statistics for clean measurement
	StringBuilderPool::resetStats();

	// Create several leases to demonstrate pooling
	{
		auto lease1{ StringBuilderPool::lease() };
		lease1.create().append( "First lease content" );
		std::cout << "Lease 1: " << lease1.toString() << std::endl;
	} // lease1 returns to pool

	{
		auto lease2{ StringBuilderPool::lease() };
		lease2.create().append( "Second lease content" );
		std::cout << "Lease 2: " << lease2.toString() << std::endl;
	} // lease2 returns to pool

	{
		auto lease3{ StringBuilderPool::lease() };
		lease3.create().append( "Third lease content" );
		std::cout << "Lease 3: " << lease3.toString() << std::endl;
	} // lease3 returns to pool

	// Display pool statistics
	auto stats{ StringBuilderPool::stats() };
	std::cout << std::endl
			  << "Pool Statistics:" << std::endl;
	std::cout << "  Total requests: " << stats.totalRequests << std::endl;
	std::cout << "  Thread-local hits: " << stats.threadLocalHits << std::endl;
	std::cout << "  Shared pool hits: " << stats.dynamicStringBufferPoolHits << std::endl;
	std::cout << "  New allocations: " << stats.newAllocations << std::endl;
	std::cout << "  Hit rate: " << std::fixed << std::setprecision( 1 ) << ( stats.hitRate * 100.0 ) << "%" << std::endl;
	std::cout << "  Current pool size: " << StringBuilderPool::size() << std::endl;
	std::cout << std::endl;

	//=========================================================================
	// Advanced string building patterns
	//=========================================================================

	std::cout << "--- Advanced String Building Patterns ---" << std::endl;

	// JSON-like object construction
	{
		auto jsonLease{ StringBuilderPool::lease() };
		auto jsonBuilder{ jsonLease.create() };

		jsonBuilder << "{\n";
		jsonBuilder << "  \"name\": \"StringBuilderPool\",\n";
		jsonBuilder << "  \"version\": \"1.0\",\n";
		jsonBuilder << "  \"performance\": {\n";
		jsonBuilder << "    \"fast\": true,\n";
		jsonBuilder << "    \"memory_efficient\": true\n";
		jsonBuilder << "  },\n";
		jsonBuilder << "  \"features\": [\"pooling\", \"streaming\", \"zero-copy\"]\n";
		jsonBuilder << "}";

		std::cout << "JSON construction:" << std::endl;
		std::cout << jsonLease.toString() << std::endl;
	}
	std::cout << std::endl;

	// SQL query building
	{
		auto sqlLease{ StringBuilderPool::lease() };
		auto sqlBuilder{ sqlLease.create() };

		std::vector<std::string> columns{ "id", "name", "email", "created_at" };
		std::vector<std::string> conditions{ "active = 1", "age > 18", "country = 'US'" };

		sqlBuilder << "SELECT ";
		for ( size_t i{ 0 }; i < columns.size(); ++i )
		{
			if ( i > 0 )
				sqlBuilder << ", ";
			sqlBuilder << columns[i];
		}

		sqlBuilder << " FROM users WHERE ";
		for ( size_t i{ 0 }; i < conditions.size(); ++i )
		{
			if ( i > 0 )
				sqlBuilder << " AND ";
			sqlBuilder << conditions[i];
		}

		sqlBuilder << " ORDER BY created_at DESC LIMIT 100";

		std::cout << "SQL query building:" << std::endl;
		std::cout << sqlLease.toString() << std::endl;
	}
	std::cout << std::endl;

	// Log message formatting
	{
		auto logLease{ StringBuilderPool::lease() };
		auto logBuilder{ logLease.create() };

		logBuilder << "[2025-08-31 14:30:00 UTC] ";
		logBuilder << "INFO: StringBuilderPool sample running successfully. ";
		logBuilder << "Memory usage optimized, performance enhanced.";

		std::cout << "Log message formatting:" << std::endl;
		std::cout << logLease.toString() << std::endl;
	}
	std::cout << std::endl;

	//=========================================================================
	// Iterator and enumeration examples
	//=========================================================================

	std::cout << "--- Iterator and Enumeration Examples ---" << std::endl;

	{
		auto iterLease{ StringBuilderPool::lease() };
		auto iterBuilder{ iterLease.create() };
		iterBuilder.append( "Iterator Demo" );

		std::cout << "Original content: " << iterLease.toString() << std::endl;

		// Range-based for loop
		std::cout << "Characters via range-based for : ";
		for ( char c : iterBuilder )
		{
			std::cout << c << " ";
		}
		std::cout << std::endl;

		std::cout << "Characters via manual iteration: ";
		std::for_each( iterBuilder.begin(), iterBuilder.end(), []( char c ) {
			std::cout << c << " ";
		} );
		std::cout << std::endl;

		// Enumerator pattern
		std::cout << "Characters via enumerator      : ";
		StringBuilder::Enumerator enumerator{ iterBuilder };
		while ( enumerator.next() )
		{
			std::cout << enumerator.current() << " ";
		}
		std::cout << std::endl;
	}
	std::cout << std::endl;

	//=========================================================================
	// Memory management and capacity demonstration
	//=========================================================================

	std::cout << "--- Memory Management and Capacity ---" << std::endl;

	{
		auto memLease{ StringBuilderPool::lease() };
		auto& memBuffer{ memLease.buffer() };
		auto memBuilder{ memLease.create() };

		std::cout << "Initial capacity: " << memBuffer.capacity() << std::endl;

		// Reserve larger capacity
		memBuffer.reserve( 2048 );
		std::cout << "After reserve(2048): " << memBuffer.capacity() << std::endl;

		// Add content
		for ( int i{ 0 }; i < 10; ++i )
		{
			memBuilder << "Content block " << std::to_string( i ) << " - ";
		}

		std::cout << "Content size: " << memBuffer.size() << std::endl;
		std::cout << "Capacity after content: " << memBuffer.capacity() << std::endl;
		std::cout << "Content preview: " << memLease.toString().substr( 0, 50 ) << "..." << std::endl;

		// Array access
		if ( memBuffer.size() > 10 )
		{
			std::cout << "Character at position 8: '" << memBuilder[8] << "'" << std::endl;
		}

		// Resize operations
		size_t original_size{ memBuffer.size() };
		memBuilder.resize( 20 );
		std::cout << "After resize to 20: \"" << memLease.toString() << "\"" << std::endl;

		memBuilder.resize( original_size );
		std::cout << "After resize back to original: size = " << memBuffer.size() << std::endl;
	}
	std::cout << std::endl;

	//=========================================================================
	// Thread safety demonstration
	//=========================================================================

	std::cout << "--- Thread Safety Demonstration ---" << std::endl;

	std::vector<std::thread> threads;
	std::vector<std::string> results{ 4 };

	// Reset pool statistics
	StringBuilderPool::resetStats();

	for ( size_t t{ 0 }; t < 4; ++t )
	{
		threads.emplace_back( [t, &results]() {
			auto threadLease{ StringBuilderPool::lease() };
			auto threadBuilder{ threadLease.create() };

			threadBuilder << "Thread " << std::to_string( t ) << " processing: ";
			for ( int i{ 0 }; i < 10; ++i )
			{
				threadBuilder << "[" << std::to_string( i ) << "]";
			}

			results[t] = threadLease.toString();
		} );
	}

	// Wait for all threads to complete
	for ( auto& thread : threads )
	{
		thread.join();
	}

	std::cout << "Thread results:" << std::endl;
	for ( size_t i{ 0 }; i < results.size(); ++i )
	{
		std::cout << "  " << results[i] << std::endl;
	}

	auto threadStats{ StringBuilderPool::stats() };
	std::cout << std::endl
			  << "Multi-threaded statistics:" << std::endl;
	std::cout << "  Total requests: " << threadStats.totalRequests << std::endl;
	std::cout << "  Hit rate: " << std::fixed << std::setprecision( 1 ) << ( threadStats.hitRate * 100.0 ) << "%" << std::endl;
	std::cout << std::endl;

	//=========================================================================
	// Real-world use case: CSV generation
	//=========================================================================

	std::cout << "--- Real-World Use Case: CSV Generation ---" << std::endl;

	struct Product
	{
		std::string name;
		double price;
		int quantity;
		std::string category;
	};

	std::vector<Product> products{
		{ "Laptop", 999.99, 50, "Electronics" },
		{ "Mouse", 29.99, 200, "Electronics" },
		{ "Keyboard", 79.99, 100, "Electronics" },
		{ "Monitor", 299.99, 75, "Electronics" },
		{ "Desk Chair", 199.99, 25, "Furniture" } };

	auto csvLease{ StringBuilderPool::lease() };
	auto csvBuilder{ csvLease.create() };

	// CSV header
	csvBuilder << "Name,Price,Quantity,Category,Total Value\n";

	// CSV data rows
	double grandTotal{ 0.0 };
	for ( const auto& product : products )
	{
		double totalValue{ product.price * product.quantity };
		grandTotal += totalValue;

		csvBuilder << product.name << ",";
		csvBuilder << std::to_string( product.price ) << ",";
		csvBuilder << std::to_string( product.quantity ) << ",";
		csvBuilder << product.category << ",";
		csvBuilder << std::to_string( totalValue ) << "\n";
	}

	// Summary row
	csvBuilder << "TOTAL,,,," << std::to_string( grandTotal );

	std::cout << "Generated CSV:" << std::endl;
	std::cout << csvLease.toString() << std::endl;
	std::cout << std::endl;

	//=========================================================================
	// Clean up and final statistics
	//=========================================================================

	std::cout << "--- Final Pool Statistics ---" << std::endl;

	auto finalStats{ StringBuilderPool::stats() };
	std::cout << "Session summary:" << std::endl;
	std::cout << "  Total pool requests: " << finalStats.totalRequests << std::endl;
	std::cout << "  Cache efficiency: " << std::fixed << std::setprecision( 1 ) << ( finalStats.hitRate * 100.0 ) << "%" << std::endl;
	std::cout << "  Current pool size: " << StringBuilderPool::size() << std::endl;

	// Clear the pool
	size_t cleared{ StringBuilderPool::clear() };
	std::cout << "  Cleared " << cleared << " buffers from pool" << std::endl;
	std::cout << "  Final pool size: " << StringBuilderPool::size() << std::endl;

	return 0;
}
