cmake_minimum_required(VERSION 3.5)

project(AICMetadata
          VERSION 0.0
          LANGUAGES C CXX)

if(NOT DEFINED WARNINGS_ENABLED)
  set(WARNINGS_ENABLED "-Wall" "-Wextra" "-Wno-missing-field-initializers" "-Wunused-variable" "-Wunused-parameter" "-Wsign-compare")
  if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(WARNINGS_ENABLED ${WARNINGS_ENABLED} "-Wunused-but-set-variable")
  endif()
endif()

add_compile_options(${WARNINGS_ENABLED})
add_library(AICMetadata STATIC src/AICMetadata.c)
target_include_directories(AICMetadata PUBLIC inc)
target_compile_options(AICMetadata PRIVATE -fPIC -std=c11)

add_library(AICMetadataWriter STATIC src/AICMetadataWriter.cpp)
target_include_directories(AICMetadataWriter PUBLIC inc)
target_compile_options(AICMetadataWriter PRIVATE -fPIC -std=c++11)

add_library(AICMetadataReader STATIC src/AICMetadataReader.c)
target_include_directories(AICMetadataReader PUBLIC inc)
target_compile_options(AICMetadataReader PRIVATE -fPIC -std=c11)



