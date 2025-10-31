#==============================================================================
# nfx-stringbuilderpool - Benchmark suite CMake configuration
#==============================================================================

#----------------------------------------------
# Benchmark condition check
#----------------------------------------------

if(NOT NFX_STRINGBUILDERPOOL_BUILD_BENCHMARKS)
	message(STATUS "Benchmarks disabled, skipping...")
	return()
endif()

#----------------------------------------------
# Benchmarks source files
#----------------------------------------------

set(BENCHMARK_SOURCES)

list(APPEND BENCHMARK_SOURCES
	BM_StringBuilderPool.cpp
)

#----------------------------------------------
# Configure benchmark executables
#----------------------------------------------

foreach(benchmark_source ${BENCHMARK_SOURCES})
	get_filename_component(benchmark_target_name ${benchmark_source} NAME_WE)
	if(NOT TARGET ${benchmark_target_name})
		add_executable(${benchmark_target_name} ${benchmark_source})

		#----------------------------------------------
		# Target linking
		#----------------------------------------------

		target_link_libraries(${benchmark_target_name} PRIVATE
			nfx-stringbuilderpool::static
			benchmark::benchmark
		)

		#----------------------------------------------
		# Properties
		#----------------------------------------------

		set_target_properties(${benchmark_target_name} PROPERTIES
			CXX_STANDARD 20
			CXX_STANDARD_REQUIRED ON
			CXX_EXTENSIONS OFF
			POSITION_INDEPENDENT_CODE ON
			DEBUG_POSTFIX "-d"
			RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/benchmarks"
			RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/benchmarks"
			RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/benchmarks"
		)
	endif()
endforeach()
