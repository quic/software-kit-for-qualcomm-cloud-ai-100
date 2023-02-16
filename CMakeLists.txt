cmake_minimum_required (VERSION 3.24)
include(FetchContent)
include(${CMAKE_SOURCE_DIR}/cmake/compiler_warnings.cmake)

set(CMAKE_CXX_STANDARD 17)

project (AicOpenRuntime
	  LANGUAGES CXX
	  VERSION 1.0
	  )






add_subdirectory(inc)
add_subdirectory(thirdparty-modules)
add_subdirectory(test)
add_subdirectory(lib)
add_subdirectory(utils)

#enable_testing()


