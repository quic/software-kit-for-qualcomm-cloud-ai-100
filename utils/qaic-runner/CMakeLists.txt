add_executable(qaic-runner QAicRunnerMain.cpp QAicRunner.cpp)

target_link_libraries(qaic-runner
                     QAicApiHpp
                     qlog
                     RuntimePlatform
                     QAicQpc
                     AICNetworkDescProto
                     QAicApiProto
                     module-qpcpp
                     AICMetadataFlatbuffer
                     AICMetadata
                     module-flatbuffers
                     AICPrePostProc)
