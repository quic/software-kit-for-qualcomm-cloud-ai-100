## Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
## SPDX-License-Identifier: BSD-3-Clause-Clear

project (QAicCore)

add_subdirectory(proto)

add_library(QAicCore STATIC src/QContext.cpp
                     src/QBindingsParser.cpp
                     src/QProgramContainer.cpp
                     src/QProgram.cpp
                     src/QProgramDevice.cpp
                     src/QExecObj.cpp
                     src/QComponent.cpp
                     src/QPrePostProc.cpp
)

target_include_directories(QAicCore PUBLIC inc/)

target_link_libraries(QAicCore PRIVATE qlog)
target_link_libraries(QAicCore PRIVATE RuntimePlatform)
target_link_libraries(QAicCore PRIVATE QAicNetworkDriver)
target_link_libraries(QAicCore PRIVATE QAicQpc)
target_link_libraries(QAicCore PRIVATE AICPrePostProc)
target_link_libraries(QAicCore PRIVATE AICNetworkDescProto)
target_link_libraries(QAicCore PRIVATE QAicApiProto)
target_link_libraries(QAicCore PRIVATE AICMetadataFlatbuffer)
target_link_libraries(QAicCore PRIVATE AICMetadata)
target_link_libraries(QAicCore PRIVATE module-flatbuffers)
target_link_libraries(QAicCore PRIVATE module-qpcpp)
target_link_libraries(QAicCore PRIVATE module-elfio)
target_link_libraries(QAicCore PRIVATE module-json)
target_link_libraries(QAicCore PRIVATE protobuf::libprotobuf)

set_target_compiler_warnings(QAicCore)
