project (QAicRuntimePlatform)

add_subdirectory(platform-mutex)
add_subdirectory(src)

target_include_directories(RuntimePlatform PUBLIC inc/dev/common)
target_include_directories(RuntimePlatform PUBLIC inc/dev/aic100)
target_include_directories(RuntimePlatform PUBLIC inc)
target_include_directories(RuntimePlatform PUBLIC inc/os/common)
target_include_directories(RuntimePlatform PUBLIC inc/os/linux)

set_target_compiler_warnings(RuntimePlatform)
