include(FetchContent)

set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)
FetchContent_Declare(ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
  GIT_TAG v3.0.0
)

FetchContent_GetProperties(ftxui)
if(NOT ftxui_POPULATED)
  FetchContent_Populate(ftxui)
  add_subdirectory(${ftxui_SOURCE_DIR} ${ftxui_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()


add_executable(qaic-util  QaicUtil.cpp QaicUtilTxUI.cpp)

target_link_libraries(qaic-util
QAicApiHpp
                      RuntimePlatform
                      pthread
                      pci
                      ftxui::screen
                      ftxui::dom
                      ftxui::component
                      )