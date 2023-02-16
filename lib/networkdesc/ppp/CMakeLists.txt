project(AICPrePostProc
          VERSION 0.1
	  LANGUAGES CXX)

add_library(AICPrePostProc STATIC src/PrePostProc.cpp)

target_include_directories(AICPrePostProc PUBLIC inc/)
target_compile_options(AICPrePostProc PRIVATE -fPIC)

target_link_libraries(AICPrePostProc PRIVATE libprotobuf AICNetworkDescProto)