set(cpu_headers
  ${CMAKE_CURRENT_SOURCE_DIR}/rapl/rapl_interface.h
  ${CMAKE_CURRENT_SOURCE_DIR}/cpu_rapl_device.h
  ${CMAKE_CURRENT_SOURCE_DIR}/cpu.h
  ${CMAKE_CURRENT_SOURCE_DIR}/cpudevice.h
  ${CMAKE_CURRENT_SOURCE_DIR}/dram_rapl_device.h
  ${CMAKE_CURRENT_SOURCE_DIR}/intel_cpus.h
  CACHE INTERNAL "")

set(cpu_sources
${CMAKE_CURRENT_SOURCE_DIR}/rapl/rapl_interface.cpp
${CMAKE_CURRENT_SOURCE_DIR}/abstract_cpu.cpp
${CMAKE_CURRENT_SOURCE_DIR}/cpu_core.cpp
${CMAKE_CURRENT_SOURCE_DIR}/cpu_linux.cpp
${CMAKE_CURRENT_SOURCE_DIR}/cpu_package.cpp
${CMAKE_CURRENT_SOURCE_DIR}/cpu_rapl_device.cpp
${CMAKE_CURRENT_SOURCE_DIR}/cpu.cpp
${CMAKE_CURRENT_SOURCE_DIR}/cpudevice.cpp
${CMAKE_CURRENT_SOURCE_DIR}/dram_rapl_device.cpp
${CMAKE_CURRENT_SOURCE_DIR}/intel_cpus.cpp
${CMAKE_CURRENT_SOURCE_DIR}/intel_gpu.cpp
  CACHE INTERNAL "")

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${libpwrm_includes})