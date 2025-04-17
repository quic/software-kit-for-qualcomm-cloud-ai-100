#--------------------------------------------
#Get SpdLog Module
FetchContent_Declare(spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG        v1.10.0
)
FetchContent_MakeAvailable(spdlog)

#Expose interface library for SPDLOG Module
add_library(module-spdlog INTERFACE)
target_include_directories(module-spdlog INTERFACE
	${spdlog_SOURCE_DIR}/include)

#--------------------------------------------
#Get Json Module
FetchContent_Declare(json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG        v3.11.2
)
FetchContent_MakeAvailable(json)

#Expose interface library for JSON Module
add_library(module-json INTERFACE)
target_include_directories(module-json INTERFACE
${json_SOURCE_DIR}/include/)

#--------------------------------------------
#Get ElfIo Module
FetchContent_Declare(elfio
  GIT_REPOSITORY https://github.com/serge1/ELFIO.git
  GIT_TAG        Release_3.10
)
FetchContent_MakeAvailable(elfio)

#Expose interface library for Elfio Module
add_library(module-elfio INTERFACE)
target_include_directories(module-elfio INTERFACE
${elfio_SOURCE_DIR})

#--------------------------------------------
#Get Protobuf Module
set(protobuf_BUILD_TESTS OFF CACHE INTERNAL "Turn off tests")
FetchContent_Declare(protobuf
  GIT_REPOSITORY https://github.com/protocolbuffers/protobuf.git
  GIT_TAG        v3.11.4
  SOURCE_SUBDIR  cmake
  FIND_PACKAGE_ARGS NAMES protobuf
)
FetchContent_MakeAvailable(protobuf)

#Expose interface library for Protobuf Module
add_library(module-protobuf INTERFACE)
target_include_directories(module-protobuf INTERFACE
${protobuf_SOURCE_DIR}/src)

#--------------------------------------------
#Get flatcc Module
FetchContent_Declare(flatcc
  GIT_REPOSITORY https://github.com/dvidelabs/flatcc.git
  GIT_TAG        v0.6.1
)
FetchContent_MakeAvailable(flatcc)

#Expose interface library for FLATCC Module
add_library(module-flatcc INTERFACE)
target_include_directories(module-flatcc INTERFACE
        ${flatcc_SOURCE_DIR}/include)

#--------------------------------------------
#Get flatbuffers Module
FetchContent_Declare(flatbuffers
  GIT_REPOSITORY https://github.com/google/flatbuffers
  GIT_TAG        v2.0.8
)
FetchContent_MakeAvailable(flatbuffers)

#Expose library for FLATBUFFERS Module
add_library(module-flatbuffers STATIC
  ${flatbuffers_SOURCE_DIR}/src/idl_gen_text.cpp
  ${flatbuffers_SOURCE_DIR}/src/idl_parser.cpp
  ${flatbuffers_SOURCE_DIR}/src/util.cpp
)

target_include_directories(module-flatbuffers PUBLIC
        ${flatbuffers_SOURCE_DIR}/include)

#--------------------------------------------
#Get qpcpp Module
FetchContent_Declare(qpcpp
  GIT_REPOSITORY https://github.com/QuantumLeaps/qpcpp.git
  GIT_TAG        v6.9.3
)
FetchContent_MakeAvailable(qpcpp)

add_library(module-qpcpp STATIC
  ${qpcpp_SOURCE_DIR}/src/qf/qf_act.cpp
  ${qpcpp_SOURCE_DIR}/src/qf/qf_defer.cpp
  ${qpcpp_SOURCE_DIR}/src/qf/qf_mem.cpp
  ${qpcpp_SOURCE_DIR}/src/qf/qf_qact.cpp
  ${qpcpp_SOURCE_DIR}/src/qf/qf_qmact.cpp
  ${qpcpp_SOURCE_DIR}/src/qf/qf_actq.cpp
  ${qpcpp_SOURCE_DIR}/src/qf/qf_dyn.cpp
  ${qpcpp_SOURCE_DIR}/src/qf/qf_ps.cpp
  ${qpcpp_SOURCE_DIR}/src/qf/qf_qeq.cpp
  ${qpcpp_SOURCE_DIR}/src/qf/qf_time.cpp
  ${qpcpp_SOURCE_DIR}/src/qf/qep_hsm.cpp
  ${qpcpp_SOURCE_DIR}/src/qf/qep_msm.cpp
  ${qpcpp_SOURCE_DIR}/ports/posix/qf_port.cpp
)

#Disable asserts in qpcpp code
target_compile_definitions(module-qpcpp PRIVATE Q_NASSERT)

target_include_directories(module-qpcpp PUBLIC
        ${qpcpp_SOURCE_DIR}/include
        ${qpcpp_SOURCE_DIR}/ports/posix
        ${qpcpp_SOURCE_DIR}/src
)
#--------------------------------------------
#Get gtest
FetchContent_Declare(googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        release-1.12.0
  SOURCE_SUBDIR  googletest
  FIND_PACKAGE_ARGS NAMES googletest
)
set(GOOGLETEST_VERSION 1.12.0)
FetchContent_MakeAvailable(googletest)
