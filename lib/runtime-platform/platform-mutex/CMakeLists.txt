project (platform-mutex)

set(os-utils-src src/linux/QOsalUtils.cpp)

set(qmonitor-named-mutex-src src/QMonitorNamedMutex.cpp)

add_library(platform-mutex STATIC ${os-utils-src}
                      ${qmonitor-named-mutex-src})

target_link_libraries(platform-mutex PUBLIC qlog QAicApiCommon)


target_include_directories(platform-mutex PUBLIC inc)

#Keep the compile options same as that of Runtime-platform
#TODO : revisit and clean the compile options.
target_compile_options(platform-mutex PRIVATE
                    -Werror
                    -Wall
                    -Wextra
                    -fPIC
                    -Wno-deprecated-declarations
                    -Wno-missing-field-initializers
                    -Wno-unused-variable
                    -Wno-unused-parameter
)
