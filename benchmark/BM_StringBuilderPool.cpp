/**
 * @file BM_StringBuilderPool.cpp
 * @brief Benchmark StringBuilderPool performance vs standard alternatives
 */

#include <benchmark/benchmark.h>

#include <sstream>
#include <string>
#include <vector>

#include <nfx/string/StringBuilderPool.h>

namespace nfx::string::benchmark
{
	//=====================================================================
	// StringBuilderPool benchmark suite
	//=====================================================================

	//----------------------------------------------
	// Test data
	//----------------------------------------------

	static const std::vector<std::string_view> small_strings = {
		"Hello", "World", "Test", "String", "Builder" };

	static const std::vector<std::string_view> medium_strings = {
		"This is a medium length string for testing purposes",
		"Another medium string with different content and length",
		"Performance testing requires consistent data patterns",
		"Medium strings help measure real-world usage scenarios" };

	static const std::vector<std::string_view> large_strings = {
		"This is a much longer string that contains significantly more characters and is designed to test the performance characteristics of string building operations when dealing with larger amounts of text content that might be more representative of real-world applications",
		"Large strings like this one are important for benchmarking because they help us understand how the string builder performs when concatenating substantial amounts of text data, which is common in applications that generate reports, logs, or formatted output",
		"Performance analysis of string concatenation with longer strings reveals different behavior patterns compared to small string operations, particularly in terms of memory allocation patterns and buffer management strategies" };

	//----------------------------------------------
	// StringBuilderPool vs std::string concatenation
	//----------------------------------------------

	//----------------------------
	// Small strings
	//----------------------------

	static void BM_StdString_SmallStrings( ::benchmark::State& state )
	{
		for ( auto _ : state )
		{
			std::string result;

			for ( const auto& str : small_strings )
			{
				result += str;
			}

			::benchmark::DoNotOptimize( result );
		}
	}

	static void BM_StringStream_SmallStrings( ::benchmark::State& state )
	{
		for ( auto _ : state )
		{
			std::ostringstream oss;

			for ( const auto& str : small_strings )
			{
				oss << str;
			}

			std::string result = oss.str();
			::benchmark::DoNotOptimize( result );
		}
	}

	static void BM_StringBuilderPool_SmallStrings( ::benchmark::State& state )
	{
		for ( auto _ : state )
		{
			auto lease = StringBuilderPool::lease();
			auto builder = lease.create();

			for ( const auto& str : small_strings )
			{
				builder << str;
			}

			std::string result = lease.toString();
			::benchmark::DoNotOptimize( result );
		}
	}

	//----------------------------
	// Medium strings
	//----------------------------

	static void BM_StdString_MediumStrings( ::benchmark::State& state )
	{
		for ( auto _ : state )
		{
			std::string result;

			for ( const auto& str : medium_strings )
			{
				result += str;
				result += " ";
			}

			::benchmark::DoNotOptimize( result );
		}
	}

	static void BM_StringStream_MediumStrings( ::benchmark::State& state )
	{
		for ( auto _ : state )
		{
			std::ostringstream oss;

			for ( const auto& str : medium_strings )
			{
				oss << str << " ";
			}

			std::string result = oss.str();
			::benchmark::DoNotOptimize( result );
		}
	}

	static void BM_StringBuilderPool_MediumStrings( ::benchmark::State& state )
	{
		for ( auto _ : state )
		{
			auto lease = StringBuilderPool::lease();
			auto builder = lease.create();

			for ( const auto& str : medium_strings )
			{
				builder << str << " ";
			}

			std::string result = lease.toString();
			::benchmark::DoNotOptimize( result );
		}
	}

	//----------------------------
	// Large strings
	//----------------------------

	static void BM_StdString_LargeStrings( ::benchmark::State& state )
	{
		for ( auto _ : state )
		{
			std::string result;

			for ( const auto& str : large_strings )
			{
				result += str;
				result += "\n";
			}

			::benchmark::DoNotOptimize( result );
		}
	}

	static void BM_StringStream_LargeStrings( ::benchmark::State& state )
	{
		for ( auto _ : state )
		{
			std::ostringstream oss;

			for ( const auto& str : large_strings )
			{
				oss << str << "\n";
			}

			std::string result = oss.str();
			::benchmark::DoNotOptimize( result );
		}
	}

	static void BM_StringBuilderPool_LargeStrings( ::benchmark::State& state )
	{
		for ( auto _ : state )
		{
			auto lease = StringBuilderPool::lease();
			auto builder = lease.create();

			for ( const auto& str : large_strings )
			{
				builder << str << "\n";
			}

			std::string result = lease.toString();
			::benchmark::DoNotOptimize( result );
		}
	}

	//----------------------------------------------
	// Pool efficiency - rapid lease/return cycles
	//----------------------------------------------

	static void BM_StdString_RapidCycles( ::benchmark::State& state )
	{
		// Equivalent rapid string building without pooling
		for ( auto _ : state )
		{
			for ( int i = 0; i < 10; ++i )
			{
				std::string result = "Iteration ";
				result += std::to_string( i );
				result += ": ";
				result += small_strings[i % small_strings.size()];
				::benchmark::DoNotOptimize( result );
			}
		}
	}

	static void BM_StringBuilderPool_PoolEfficiency( ::benchmark::State& state )
	{
		// Test pool efficiency with rapid lease/return cycles
		for ( auto _ : state )
		{
			for ( int i = 0; i < 10; ++i )
			{
				auto lease = StringBuilderPool::lease();
				auto builder = lease.create();
				builder << "Iteration " << std::to_string( i ) << ": " << small_strings[i % small_strings.size()];
				std::string result = lease.toString();
				::benchmark::DoNotOptimize( result );
			}
		}
	}

	//----------------------------------------------
	// Mixed operations - realistic usage patterns
	//----------------------------------------------

	static void BM_StdString_MixedOperations( ::benchmark::State& state )
	{
		for ( auto _ : state )
		{
			std::string result = "Header: ";
			result += medium_strings[0];
			result += "\n";

			for ( size_t i = 0; i < small_strings.size(); ++i )
			{
				result += "Item ";
				result += std::to_string( i );
				result += ": ";
				result += small_strings[i];
				result += "\n";
			}

			result += "Footer: ";
			result += medium_strings[1];

			::benchmark::DoNotOptimize( result );
		}
	}

	static void BM_StringStream_MixedOperations( ::benchmark::State& state )
	{
		for ( auto _ : state )
		{
			std::ostringstream oss;

			// Simulate realistic string building patterns
			oss << "Header: " << medium_strings[0] << "\n";

			for ( size_t i = 0; i < small_strings.size(); ++i )
			{
				oss << "Item " << std::to_string( i ) << ": " << small_strings[i] << "\n";
			}

			oss << "Footer: " << medium_strings[1];

			std::string result = oss.str();
			::benchmark::DoNotOptimize( result );
		}
	}

	static void BM_StringBuilderPool_MixedOperations( ::benchmark::State& state )
	{
		for ( auto _ : state )
		{
			auto lease = StringBuilderPool::lease();
			auto builder = lease.create();

			// Simulate realistic string building patterns
			builder << "Header: " << medium_strings[0] << "\n";

			for ( size_t i = 0; i < small_strings.size(); ++i )
			{
				builder << "Item " << std::to_string( i ) << ": " << small_strings[i] << "\n";
			}

			builder << "Footer: " << medium_strings[1];

			std::string result = lease.toString();
			::benchmark::DoNotOptimize( result );
		}
	}

	//----------------------------------------------
	// Advanced
	//----------------------------------------------

	//----------------------------
	// Buffer reuse
	//----------------------------

	static void BM_StringBuilderPool_BufferReuse( ::benchmark::State& state )
	{
		// Test buffer reuse efficiency
		for ( auto _ : state )
		{
			// Multiple consecutive uses to test pooling
			for ( int cycle = 0; cycle < 5; ++cycle )
			{
				auto lease = StringBuilderPool::lease();
				auto builder = lease.create();

				for ( const auto& str : medium_strings )
				{
					builder << "Cycle " << std::to_string( cycle ) << ": " << str << " ";
				}

				std::string result = lease.toString();
				::benchmark::DoNotOptimize( result );
				// Buffer returned to pool here when lease destructs
			}
		}
	}

	//----------------------------
	// Zero-allocation
	//----------------------------

	static void BM_StringBuilderPool_ZeroAlloc( ::benchmark::State& state )
	{
		// Test pure builder operations without final string conversion
		for ( auto _ : state )
		{
			auto lease = StringBuilderPool::lease();
			auto builder = lease.create();

			for ( const auto& str : small_strings )
			{
				builder << str << " ";
			}

			// Just access the buffer without creating string
			auto view = lease.buffer().toStringView();
			::benchmark::DoNotOptimize( view );
		}
	}

	//----------------------------
	// Memory pressure
	//----------------------------

	static void BM_StringBuilderPool_MemoryPressure( ::benchmark::State& state )
	{
		// Simulate memory pressure with large strings
		for ( auto _ : state )
		{
			auto lease = StringBuilderPool::lease();
			auto builder = lease.create();

			// Build a large result
			for ( int i = 0; i < 20; ++i )
			{
				for ( const auto& str : large_strings )
				{
					builder << str << " ";
				}
			}

			std::string result = lease.toString();
			::benchmark::DoNotOptimize( result );
		}
	}
} // namespace nfx::string::benchmark

//=====================================================================
// Benchmarks registration
//=====================================================================

//----------------------------------------------
// StringBuilderPool vs std::string concatenation
//----------------------------------------------

//----------------------------
// Small strings
//----------------------------

BENCHMARK( nfx::string::benchmark::BM_StdString_SmallStrings )
	->MinTime( 1.0 )
	->Unit( benchmark::kNanosecond );

BENCHMARK( nfx::string::benchmark::BM_StringStream_SmallStrings )
	->MinTime( 1.0 )
	->Unit( benchmark::kNanosecond );

BENCHMARK( nfx::string::benchmark::BM_StringBuilderPool_SmallStrings )
	->MinTime( 1.0 )
	->Unit( benchmark::kNanosecond );

//----------------------------
// Medium strings
//----------------------------

BENCHMARK( nfx::string::benchmark::BM_StdString_MediumStrings )
	->MinTime( 1.0 )
	->Unit( benchmark::kNanosecond );

BENCHMARK( nfx::string::benchmark::BM_StringStream_MediumStrings )
	->MinTime( 1.0 )
	->Unit( benchmark::kNanosecond );

BENCHMARK( nfx::string::benchmark::BM_StringBuilderPool_MediumStrings )
	->MinTime( 1.0 )
	->Unit( benchmark::kNanosecond );

//----------------------------
// Large strings
//----------------------------

BENCHMARK( nfx::string::benchmark::BM_StdString_LargeStrings )
	->MinTime( 1.0 )
	->Unit( benchmark::kNanosecond );

BENCHMARK( nfx::string::benchmark::BM_StringStream_LargeStrings )
	->MinTime( 1.0 )
	->Unit( benchmark::kNanosecond );

BENCHMARK( nfx::string::benchmark::BM_StringBuilderPool_LargeStrings )
	->MinTime( 1.0 )
	->Unit( benchmark::kNanosecond );

//----------------------------------------------
// Pool efficiency
//----------------------------------------------

BENCHMARK( nfx::string::benchmark::BM_StdString_RapidCycles )
	->MinTime( 1.0 )
	->Unit( benchmark::kNanosecond );

BENCHMARK( nfx::string::benchmark::BM_StringBuilderPool_PoolEfficiency )
	->MinTime( 1.0 )
	->Unit( benchmark::kNanosecond );

//----------------------------------------------
// Mixed operations
//----------------------------------------------

BENCHMARK( nfx::string::benchmark::BM_StdString_MixedOperations )
	->MinTime( 1.0 )
	->Unit( benchmark::kNanosecond );

BENCHMARK( nfx::string::benchmark::BM_StringStream_MixedOperations )
	->MinTime( 1.0 )
	->Unit( benchmark::kNanosecond );

BENCHMARK( nfx::string::benchmark::BM_StringBuilderPool_MixedOperations )
	->MinTime( 1.0 )
	->Unit( benchmark::kNanosecond );

//----------------------------------------------
// Advanced
//----------------------------------------------

//----------------------------
// Buffer reuse
//----------------------------

BENCHMARK( nfx::string::benchmark::BM_StringBuilderPool_BufferReuse )
	->MinTime( 1.0 )
	->Unit( benchmark::kNanosecond );

//----------------------------
// Zero-allocation
//----------------------------

BENCHMARK( nfx::string::benchmark::BM_StringBuilderPool_ZeroAlloc )
	->MinTime( 1.0 )
	->Unit( benchmark::kNanosecond );

//----------------------------
// Memory pressure
//----------------------------

BENCHMARK( nfx::string::benchmark::BM_StringBuilderPool_MemoryPressure )
	->MinTime( 1.0 )
	->Unit( benchmark::kNanosecond );

BENCHMARK_MAIN();
