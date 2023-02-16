project(QAicQpc VERSION 0.1 LANGUAGES CXX)

add_library(QAicQpc STATIC src/QAicQpc.cpp)
target_include_directories(QAicQpc PUBLIC inc/)

target_link_libraries(QAicQpc PRIVATE elfio)

SET_TARGET_PROPERTIES(QAicQpc PROPERTIES LINK_FLAGS -Wl,-Bsymbolic)
set_target_compiler_warnings(QAicQpc)