set(measurement_headers
  ${CMAKE_CURRENT_SOURCE_DIR}/acpi.h
  ${CMAKE_CURRENT_SOURCE_DIR}/extech.h
  ${CMAKE_CURRENT_SOURCE_DIR}/measurement.h
  ${CMAKE_CURRENT_SOURCE_DIR}/opal-sensors.h
  ${CMAKE_CURRENT_SOURCE_DIR}/sysfs.h
  CACHE INTERNAL "")

set(measurement_sources
  ${CMAKE_CURRENT_SOURCE_DIR}/acpi.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/extech.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/measurement.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/opal-sensors.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/sysfs.cpp
  CACHE INTERNAL "")