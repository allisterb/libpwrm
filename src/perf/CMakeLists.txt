set(perf_headers
  ${CMAKE_CURRENT_SOURCE_DIR}/perf_event.h
  ${CMAKE_CURRENT_SOURCE_DIR}/perf_bundle.h
  ${CMAKE_CURRENT_SOURCE_DIR}/perf.h
  CACHE INTERNAL "")

set(perf_sources
  ${CMAKE_CURRENT_SOURCE_DIR}/perf_bundle.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/perf.cpp
  CACHE INTERNAL "")

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${libpwrm_includes})

add_library(libpwrm_perf OBJECT
            ${perf_sources}
            ${perf_headers})
