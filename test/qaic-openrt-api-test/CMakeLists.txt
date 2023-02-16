set(OS_GTEST_MAIN "gtest_main")

find_package(googletest CONFIG REQUIRED)

add_executable(qaic-openrt-api-unit-test
    src/QAicOpenRtUnitTestBase.cpp
    src/QAicOpenRtUnitTest.cpp
    src/QAicOpenRtApiContextUnitTest.cpp
    src/QAicOpenRtApiUtilUnitTest.cpp
    src/QAicOpenRtApiQpcUnitTest.cpp
    src/QAicOpenRtApiProgramUnitTest.cpp
    src/QAicOpenRtApiExecObjUnitTest.cpp
    src/QAicOpenRtInferenceVectorUnitTest.cpp
)

target_link_libraries(qaic-openrt-api-unit-test
        QAicApiHpp
        RuntimePlatform
        qlog
        ${OS_GTEST_MAIN}
        pthread
        QAicQpc
        AICNetworkDescProto
        QAicApiProto
        module-qpcpp
        AICMetadata
        AICMetadataFlatbuffer
        module-flatbuffers
        AICPrePostProc
)

target_include_directories(qaic-openrt-api-unit-test PUBLIC inc/)
