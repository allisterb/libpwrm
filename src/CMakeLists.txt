# avoid warnings for project commands w/o VERSION
cmake_policy(SET CMP0048 NEW)

project(libpwrm VERSION "0.1.0" LANGUAGES C CXX)

cmake_minimum_required(VERSION 3.9 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads REQUIRED)

include(cmake/pciutils.cmake)

if(NOT ${PCIUTILS_FOUND})
message(FATAL_ERROR "pciutils library not found.")
endif()

add_subdirectory(traceevent)
add_subdirectory(perf)
add_subdirectory(measurement)
add_subdirectory(parameters)
add_subdirectory(process)
add_subdirectory(devices)
add_subdirectory(cpu)

set(pwrm_headers      
    ${traceevent_headers}
    ${measurement_headers}
    ${parameters_headers}
    ${perf_headers}
    ${process_headers}
    ${devices_headers}
    ${cpu_headers}
    devlist.h
    lib.h
)

add_library(pwrm STATIC
    ${pwrm_headers}
    ${measurement_sources}
    ${parameters_sources}
    ${perf_sources}
    ${process_sources}
    ${devices_sources}
    ${cpu_sources}
    devlist.cpp
    lib.cpp
)

target_compile_options(pwrm PUBLIC -Wall -Wformat -Wshadow -fno-omit-frame-pointer -fstack-protector -pthread)

target_include_directories (pwrm PUBLIC ${PCIUTILS_INCLUDE_DIR})

target_link_libraries(pwrm PUBLIC traceevent ${PCIUTILS_LIBRARIES})

target_link_libraries(pwrm PRIVATE Threads::Threads)

add_executable(pwrm_cli main.cpp ${pwrm_headers})

target_compile_options(pwrm_cli PUBLIC -Wall -Wformat -Wshadow -fno-omit-frame-pointer -fstack-protector)

target_link_libraries(pwrm_cli PUBLIC pwrm)