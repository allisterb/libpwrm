set(nvml_power_headers
  ${CUDAToolkit_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}/nvml_power.h
  CACHE INTERNAL "")

set(nvml_power_sources
  ${CMAKE_CURRENT_SOURCE_DIR}/nvml_power.cpp
  CACHE INTERNAL "")

add_library(nvml_power SHARED
            ${nvml_power_sources}
            ${nvml_power_headers}
)

target_link_libraries(nvml_power PUBLIC CUDA::nvml)

