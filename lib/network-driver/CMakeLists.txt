project (QAicNetworkDriver)

add_library(QAicNetworkDriver STATIC src/QKmdDevice.cpp
                           src/QKmdDeviceFactory.cpp
                           src/QRuntime.cpp
                           src/QNeuralNetwork.cpp
                           src/QNNConstants.cpp
                           src/QNNImage.cpp
                           src/QImageParser.cpp
                           src/QKmdRuntime.cpp
                           src/QRuntimeFunc.cpp
                           src/QRuntimeManager.cpp)

target_include_directories(QAicNetworkDriver PUBLIC inc)

target_link_libraries(QAicNetworkDriver PRIVATE module-elfio)
target_link_libraries(QAicNetworkDriver PRIVATE RuntimePlatform)
target_link_libraries(QAicNetworkDriver PRIVATE AICMetadata)
target_link_libraries(QAicNetworkDriver PRIVATE AICMetadataWriter)
target_link_libraries(QAicNetworkDriver PRIVATE AICMetadataFlatbuffer)
target_link_libraries(QAicNetworkDriver PRIVATE module-flatbuffers)
target_link_libraries(QAicNetworkDriver PRIVATE module-json)
target_link_libraries(QAicNetworkDriver PRIVATE AICMetadataReader)
target_link_libraries(QAicNetworkDriver PRIVATE QAicQpc)

set_target_compiler_warnings(QAicNetworkDriver)
