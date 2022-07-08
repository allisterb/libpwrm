set(devices_headers
  ${CMAKE_CURRENT_SOURCE_DIR}/ahci.h
  ${CMAKE_CURRENT_SOURCE_DIR}/alsa.h
  ${CMAKE_CURRENT_SOURCE_DIR}/device.h
  ${CMAKE_CURRENT_SOURCE_DIR}/devfreq.h
  ${CMAKE_CURRENT_SOURCE_DIR}/gpu_rapl_device.h
  ${CMAKE_CURRENT_SOURCE_DIR}/network.h
  ${CMAKE_CURRENT_SOURCE_DIR}/runtime_pm.h
  ${CMAKE_CURRENT_SOURCE_DIR}/usb.h
  CACHE INTERNAL "")

set(devices_sources
  ${CMAKE_CURRENT_SOURCE_DIR}/ahci.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/alsa.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/device.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/devfreq.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/gpu_rapl_device.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/network.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/runtime_pm.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/usb.cpp
  CACHE INTERNAL "")

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${libpwrm_includes})