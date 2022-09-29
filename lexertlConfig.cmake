cmake_minimum_required(VERSION @CMAKE_MAJOR_VERSION@.@CMAKE_MINOR_VERSION@)
include(CMakeFindDependencyMacro)
set(lexertl_VERSION @lexertl_VERSION@)

@PACKAGE_INIT@

include("${CMAKE_CURRENT_LIST_DIR}/lexertlTargets.cmake")
check_required_components(lexertl)
