set(src_files
  QWorkqueueElement.cpp
  QWorkqueueProcessor.cpp
  QWorkthread.cpp
  QMonitorDeviceObserver.cpp
  QDeviceStateInfo.cpp
  QRuntimePlatformKmdDevice.cpp
  QRuntimePlatformKmdDeviceFactory.cpp
  QRuntimePlatform.cpp
  QRuntimePlatformManager.cpp
  QUtil.cpp
  QRuntimePlatformApi.cpp
)

set(dev_files
  dev/common/QDevInterface.cpp
  dev/aic100/QRuntimePlatformDeviceAic100.cpp
  dev/aic100/QDevInterfaceAic100.cpp
)

#Define Platfrom Specific Files
set(os_files
  os/linux/QOsal.cpp
  os/linux/QDeviceStateMonitor.cpp
  os/linux/QOsBuffer.cpp
)

add_library(RuntimePlatform STATIC
  ${src_files}
  ${dev_files}
  ${os_files}
)

#target_link_libraries(RuntimePlatform PUBLIC platform-mutex)

target_link_libraries(RuntimePlatform PRIVATE
                      pci
                      qlog
                      )


target_link_libraries(RuntimePlatform PRIVATE
                      rt
                      udev
                      )

target_link_libraries(RuntimePlatform PUBLIC platform-mutex)
target_link_libraries(RuntimePlatform PRIVATE QAicApiCommon)

target_compile_options(RuntimePlatform PRIVATE
                    -Werror
                    -Wall
                    -Wextra
                    -fPIC
                    -Wno-deprecated-declarations
                    -Wno-missing-field-initializers
                    -Wno-unused-variable
                    -Wno-unused-parameter
)
