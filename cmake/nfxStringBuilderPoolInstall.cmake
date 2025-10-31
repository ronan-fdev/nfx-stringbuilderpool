#==============================================================================
# nfx-stringbuilderpool - Library installation
#==============================================================================

#----------------------------------------------
# Installation condition check
#----------------------------------------------

if(NOT NFX_STRINGBUILDERPOOL_INSTALL_PROJECT)
	message(STATUS "Installation disabled, skipping...")
	return()
endif()

#----------------------------------------------
# Installation paths configuration
#----------------------------------------------

include(GNUInstallDirs)

message(STATUS "System installation paths:")
message(STATUS "  Headers      : ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}")
message(STATUS "  Library      : ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
message(STATUS "  Binaries     : ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}")
message(STATUS "  CMake configs: ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/cmake/nfx-stringbuilderpool")
message(STATUS "  Documentation: ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DOCDIR}")

#----------------------------------------------
# Install headers
#----------------------------------------------

install(
	DIRECTORY "${NFX_STRINGBUILDERPOOL_INCLUDE_DIR}/"
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
	COMPONENT Development
	FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp" PATTERN "*.inl"
)

#----------------------------------------------
# Install library targets
#----------------------------------------------

set(INSTALL_TARGETS)

if(NFX_STRINGBUILDERPOOL_BUILD_SHARED)
	list(APPEND INSTALL_TARGETS ${PROJECT_NAME})
endif()

if(NFX_STRINGBUILDERPOOL_BUILD_STATIC)
	list(APPEND INSTALL_TARGETS ${PROJECT_NAME}-static)
endif()

if(INSTALL_TARGETS)
	install(
		TARGETS ${INSTALL_TARGETS}
		EXPORT nfx-stringbuilderpool-targets
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
			COMPONENT Development
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
			COMPONENT Runtime
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
			COMPONENT Runtime
		INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
	)
	
	# Install PDB files for debug builds on Windows
	if(MSVC AND NFX_STRINGBUILDERPOOL_BUILD_SHARED AND TARGET ${PROJECT_NAME})
		install(
			FILES $<TARGET_PDB_FILE:${PROJECT_NAME}>
			DESTINATION ${CMAKE_INSTALL_BINDIR}
			CONFIGURATIONS Debug RelWithDebInfo
			COMPONENT Development
			OPTIONAL
		)
	endif()
endif()

#----------------------------------------------
# Install CMake config files
#----------------------------------------------

install(
	EXPORT nfx-stringbuilderpool-targets
	FILE nfx-stringbuilderpool-targets.cmake
	NAMESPACE nfx-stringbuilderpool::
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/nfx-stringbuilderpool
	COMPONENT Development
)

# Install separate target files for each configuration (multi-config generators)
if(CMAKE_CONFIGURATION_TYPES)
	foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
		install(
			EXPORT nfx-stringbuilderpool-targets
			FILE nfx-stringbuilderpool-targets-${CONFIG}.cmake
			NAMESPACE nfx-stringbuilderpool::
			DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/nfx-stringbuilderpool
			CONFIGURATIONS ${CONFIG}
			COMPONENT Development
		)
	endforeach()
endif()

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
	"${CMAKE_CURRENT_BINARY_DIR}/nfx-stringbuilderpool-config-version.cmake"
	VERSION ${PROJECT_VERSION}
	COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/nfx-stringbuilderpool-config.cmake.in"
	"${CMAKE_CURRENT_BINARY_DIR}/nfx-stringbuilderpool-config.cmake"
	INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/nfx-stringbuilderpool
)

install(
	FILES
		"${CMAKE_CURRENT_BINARY_DIR}/nfx-stringbuilderpool-config.cmake"
		"${CMAKE_CURRENT_BINARY_DIR}/nfx-stringbuilderpool-config-version.cmake"
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/nfx-stringbuilderpool
	COMPONENT Development
)

#----------------------------------------------
# Install license files
#----------------------------------------------

install(
	FILES "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt"
	DESTINATION "${CMAKE_INSTALL_DOCDIR}/licenses"
)

install(
	DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/licenses/"
	DESTINATION "${CMAKE_INSTALL_DOCDIR}/licenses"
	FILES_MATCHING PATTERN "LICENSE.txt-*"
)

#----------------------------------------------
# Install documentation
#----------------------------------------------

if(NFX_STRINGBUILDERPOOL_BUILD_DOCUMENTATION)
	install(
		DIRECTORY "${CMAKE_BINARY_DIR}/doc/html"
		DESTINATION ${CMAKE_INSTALL_DOCDIR}
		OPTIONAL
		COMPONENT Documentation
	)
	
	# Install index.html shortcut
	if(WIN32)
		# Install Windows .lnk shortcut
		install(
			FILES "${CMAKE_BINARY_DIR}/doc/index.html.lnk"
			DESTINATION ${CMAKE_INSTALL_DOCDIR}
			OPTIONAL
			COMPONENT Documentation
		)
	else()
		# Install Unix symlink
		install(
			FILES "${CMAKE_BINARY_DIR}/doc/index.html"
			DESTINATION ${CMAKE_INSTALL_DOCDIR}
			OPTIONAL
			COMPONENT Documentation
		)
	endif()
endif()

message(STATUS "Installation configured for targets: ${INSTALL_TARGETS}")
