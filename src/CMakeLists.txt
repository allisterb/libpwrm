# avoid warnings for project commands w/o VERSION
cmake_policy(SET CMP0048 NEW)

project(libpwrm VERSION "0.0.1" LANGUAGES C CXX)

cmake_minimum_required(VERSION 3.9 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

### Add perf
add_subdirectory(traceevent)
add_subdirectory(perf)
add_subdirectory(measurement)
add_subdirectory(parameters)

add_library(pwrm STATIC
    ${measurement_headers}
    ${parameters_headers}
    ${perf_headers}
    ${measurement_sources}
    ${parameters_sources}
    ${perf_sources}
)

target_link_libraries(pwrm PUBLIC ${traceevent})