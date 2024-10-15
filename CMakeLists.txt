## Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
## SPDX-License-Identifier: BSD-3-Clause-Clear

cmake_minimum_required (VERSION 3.24)
include(FetchContent)
include(${CMAKE_SOURCE_DIR}/cmake/compiler_warnings.cmake)

set(CMAKE_CXX_STANDARD 17)

project (AicOpenRuntime
	  LANGUAGES CXX
	  VERSION 1.0
	  )

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

add_subdirectory(inc)
add_subdirectory(thirdparty-modules)
add_subdirectory(test)
add_subdirectory(lib)
add_subdirectory(utils)

#enable_testing()


