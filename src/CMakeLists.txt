# avoid warnings for project commands w/o VERSION
cmake_policy(SET CMP0048 NEW)

project(libpwrm VERSION "0.0.1" LANGUAGES C CXX)

cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

### Add perf
add_subdirectory(perf)
add_subdirectory(measurement)

