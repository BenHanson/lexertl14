#
# Part of CMake configuration for lexertl library
#
# Copyright (c) 2013 Mateusz Loskot <mateusz@loskot.net>
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#
cmake_minimum_required (VERSION 3.5)
project(lexertl VERSION 0.1.0)

# Define project
add_library(lexertl INTERFACE)
target_include_directories(lexertl INTERFACE 
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:include>
)

if (BUILD_EXAMPLES)
	# Add the examples
	add_subdirectory(examples)
endif ()

if (BUILD_TESTING)
	enable_testing()
	add_subdirectory(test)
endif()

# Generate install
install(TARGETS lexertl EXPORT lexertlTargets
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	RUNTIME DESTINATION bin
	PUBLIC_HEADER DESTINATION include
)
install(DIRECTORY include/lexertl DESTINATION include)
install(EXPORT lexertlTargets
	FILE lexertlTargets.cmake
	NAMESPACE lexertl::
	DESTINATION lib/cmake/lexertl
)
include(CMakePackageConfigHelpers)
write_basic_package_version_file("lexertlConfigVersion.cmake"
	VERSION ${lexertl_VERSION}
	COMPATIBILITY SameMajorVersion
)
install(FILES "lexertlConfig.cmake" "${CMAKE_CURRENT_BINARY_DIR}/lexertlConfigVersion.cmake"
	DESTINATION lib/cmake/lexertl
)

