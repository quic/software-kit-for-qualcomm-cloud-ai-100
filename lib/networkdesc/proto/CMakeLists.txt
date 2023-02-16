project(AICNetworkDesc)

include(FindProtobuf)
find_package(protobuf CONFIG REQUIRED)

protobuf_generate_cpp(QNETWORK_DESC_PROTO_SRCS QNETWORK_DESC_PROTO_HRDS AICNetworkDesc.proto)

get_filename_component(COMPILED_PROTO_INCLUDE_PARENT_DIR ${QNETWORK_DESC_PROTO_HRDS} DIRECTORY)

add_library(AICNetworkDescProto STATIC ${QNETWORK_DESC_PROTO_SRCS})

target_link_libraries(AICNetworkDescProto module-protobuf)

target_compile_options(AICNetworkDescProto PRIVATE -fPIC)

target_include_directories(AICNetworkDescProto PUBLIC ${COMPILED_PROTO_INCLUDE_PARENT_DIR})