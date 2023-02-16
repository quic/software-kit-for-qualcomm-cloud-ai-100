add_executable(qaic-device-partition QAicDevicePartition.cpp)

target_link_libraries(qaic-device-partition
                      QAicApiHpp
                      RuntimePlatform
                      pthread
                      pci
                      module-json
                      module-elfio
                      QAicQpc
                      AICMetadataFlatbuffer
                      AICMetadata
                      module-flatbuffers
                      )
