# avoid warnings for project commands w/o VERSION
cmake_policy(SET CMP0048 NEW)

project(libpwrm VERSION "0.1.0" LANGUAGES C CXX)

cmake_minimum_required(VERSION 3.17 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads REQUIRED)

include(src/cmake/pciutils.cmake)

if(NOT ${PCIUTILS_FOUND})
    message(FATAL_ERROR "pciutils library not found.")
endif()

find_package(CUDAToolkit)

find_package(OpenSSL REQUIRED)

if(${CUDAToolkit_FOUND})
    add_compile_definitions(CUDAToolkit_FOUND=${CUDAToolkit_FOUND})
    set(nvmlpower_headers
        ${CUDAToolkit_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}/src/nvidia/nvml_power.h
    CACHE INTERNAL "")

    set(nvmlpower_sources
        ${CMAKE_CURRENT_SOURCE_DIR}/src/nvidia/nvml_power.cpp
    CACHE INTERNAL "")
endif()

set (SPDLOG_SRC 
    ext/spdlog/src/async.cpp
    ext/spdlog/src/cfg.cpp
    ext/spdlog/src/color_sinks.cpp
    ext/spdlog/src/file_sinks.cpp
    ext/spdlog/src/fmt.cpp
    ext/spdlog/src/spdlog.cpp
    ext/spdlog/src/stdout_sinks.cpp
)
add_library (spdlog STATIC ${SPDLOG_SRC})
target_include_directories(spdlog PRIVATE ext/spdlog/include)
target_compile_definitions(spdlog PRIVATE SPDLOG_COMPILED_LIB=1)

set (FIGLET_SRC 
    ext/embedFiglet/src/Figlet.cc
    ext/embedFiglet/src/Figlet_Font_standard.cc
    ext/embedFiglet/src/Figlet_Font_small.cc
)

add_subdirectory(src/traceevent)
add_subdirectory(src/perf)
add_subdirectory(src/measurement)
add_subdirectory(src/parameters)
add_subdirectory(src/process)
add_subdirectory(src/devices)
add_subdirectory(src/cpu)

set(pwrm_headers      
    ${traceevent_headers}
    ${measurement_headers}
    ${parameters_headers}
    ${perf_headers}
    ${process_headers}
    ${devices_headers}
    ${cpu_headers}
    ${nvmlpower_headers}
    src/httplib.h
    src/reporting/reporting.h
    src/devlist.h
    src/lib.h
)

add_library(pwrm SHARED
    ${pwrm_headers}
    ${measurement_sources}
    ${parameters_sources}
    ${perf_sources}
    ${process_sources}
    ${devices_sources}
    ${cpu_sources}
    ${nvmlpower_sources}
    src/reporting/reporting.cpp
    src/devlist.cpp
    src/json.hpp
    src/lib.cpp
)

target_compile_options(pwrm PUBLIC -Wall -Wformat -Wshadow -fno-omit-frame-pointer -fstack-protector -pthread)

target_include_directories (pwrm PUBLIC 
    ${PCIUTILS_INCLUDE_DIR}
    ${CUDAToolkit_INCLUDE_DIRS}
    ext/spdlog/include 
    src
)

if(${CUDAToolkit_FOUND})
    target_compile_definitions(pwrm PRIVATE CUDAToolkit_FOUND=${CUDAToolkit_FOUND})
    target_link_libraries(pwrm PUBLIC Threads::Threads spdlog traceevent CUDA::nvml ${PCIUTILS_LIBRARIES})
else()
    target_link_libraries(pwrm PUBLIC Threads::Threads spdlog traceevent ${PCIUTILS_LIBRARIES})
endif()

add_executable(pwrm_cli 
    src/main.cpp 
    ${FIGLET_SRC}
)

target_compile_options(pwrm_cli PUBLIC -Wall -Wformat -Wshadow -fno-omit-frame-pointer -fstack-protector)

if(${CUDAToolkit_FOUND})
    target_compile_definitions(pwrm_cli PRIVATE CUDAToolkit_FOUND=${CUDAToolkit_FOUND})
endif()

target_include_directories (pwrm_cli PUBLIC 
    ${OPENSSL_INCLUDE_DIR}
    ext/spdlog/include
    ext/embedFiglet/src
    ext/tclap-1.2.3/include 
    src
)
target_compile_definitions(pwrm_cli PUBLIC CPPHTTPLIB_OPENSSL_SUPPORT)
target_link_libraries(pwrm_cli PUBLIC pwrm OpenSSL::Crypto OpenSSL::SSL Threads::Threads)