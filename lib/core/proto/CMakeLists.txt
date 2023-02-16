project(QAicProto)

#set_compiler_warnings()
include(FindProtobuf)
find_package(protobuf CONFIG REQUIRED)

protobuf_generate_cpp(QAICAPI_PROTO_SRCS  QAICAPI_PROTO_HDRS  QAicApi.proto)

get_filename_component(COMPILED_PROTO_INCLUDE_PARENT_DIR ${QAICAPI_PROTO_HDRS} DIRECTORY)

add_library(QAicApiProto STATIC ${QAICAPI_PROTO_SRCS})

target_link_libraries(QAicApiProto module-protobuf)

target_compile_options(QAicApiProto PRIVATE -fPIC)
target_include_directories(QAicApiProto PUBLIC ${COMPILED_PROTO_INCLUDE_PARENT_DIR})