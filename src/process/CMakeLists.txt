set(process_headers
  ${CMAKE_CURRENT_SOURCE_DIR}/interrupt.h
  ${CMAKE_CURRENT_SOURCE_DIR}/powerconsumer.h
  ${CMAKE_CURRENT_SOURCE_DIR}/process.h
  ${CMAKE_CURRENT_SOURCE_DIR}/processdevice.h
  ${CMAKE_CURRENT_SOURCE_DIR}/timer.h
  ${CMAKE_CURRENT_SOURCE_DIR}/work.h
  CACHE INTERNAL "")

set(process_sources
${CMAKE_CURRENT_SOURCE_DIR}/interrupt.cpp
${CMAKE_CURRENT_SOURCE_DIR}/powerconsumer.cpp
${CMAKE_CURRENT_SOURCE_DIR}/do_process.cpp
${CMAKE_CURRENT_SOURCE_DIR}/process.cpp
${CMAKE_CURRENT_SOURCE_DIR}/processdevice.cpp
${CMAKE_CURRENT_SOURCE_DIR}/timer.cpp
${CMAKE_CURRENT_SOURCE_DIR}/work.cpp
  CACHE INTERNAL "")

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${libpwrm_includes})