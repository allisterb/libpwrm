set(traceevent_headers
  ${CMAKE_CURRENT_SOURCE_DIR}/event-parse.h
  ${CMAKE_CURRENT_SOURCE_DIR}/event-utils.h
  CACHE INTERNAL "")

set(traceevent_sources
  ${CMAKE_CURRENT_SOURCE_DIR}/event-parse.c
  ${CMAKE_CURRENT_SOURCE_DIR}/parse-filter.c
  ${CMAKE_CURRENT_SOURCE_DIR}/parse-utils.c
  ${CMAKE_CURRENT_SOURCE_DIR}/trace-seq.c
  CACHE INTERNAL "")

add_library(traceevent SHARED
            ${traceevent_sources}
            ${traceevent_headers})
