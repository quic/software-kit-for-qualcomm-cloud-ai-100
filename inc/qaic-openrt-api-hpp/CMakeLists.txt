add_library(QAicApiHpp INTERFACE)

target_include_directories(QAicApiHpp  INTERFACE
	${CMAKE_CURRENT_SOURCE_DIR}
	)

#Add common API definitions
add_subdirectory(qaic-api-common)
target_link_libraries(QAicApiHpp INTERFACE
		QAicApiCommon
		QAicNetworkDriver
		)

# #TODO:
target_link_libraries(QAicApiHpp INTERFACE
		QAicCore
		)

