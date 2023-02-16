cmake_minimum_required(VERSION 3.13.5)
set_compiler_warnings()

project(AICMetadataFlatbuffer)

add_library(AICMetadataFlatbuffer STATIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_include_directories(AICMetadataFlatbuffer
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/deps
)
target_compile_options(AICMetadataFlatbuffer PRIVATE -fPIC )
target_link_libraries(AICMetadataFlatbuffer PRIVATE module-flatbuffers)

project(flatbuff_writer)

add_library(metadataFlatbufferWriter STATIC ${CMAKE_CURRENT_SOURCE_DIR}/src/metadataFlatbufferWriter.cpp)
target_include_directories(metadataFlatbufferWriter PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_options(metadataFlatbufferWriter PRIVATE -fPIC )
target_link_libraries(metadataFlatbufferWriter PRIVATE AICMetadata module-flatbuffers)

target_include_directories(metadataFlatbufferWriter PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/deps
)
