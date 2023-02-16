
add_library(qlog STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src/QLog.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/QLogger.cpp
)

target_compile_options(qlog PRIVATE
                    -fPIC -DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_ERROR
)


set_target_compiler_warnings(qlog)

target_link_libraries(qlog
	PUBLIC module-spdlog
	)

target_include_directories(qlog PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)
target_include_directories(qlog PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/spdlog/include)
