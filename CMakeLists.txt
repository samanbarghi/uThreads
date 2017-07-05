# At LEAST 3.2 but newer is better
cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
project(uThreads VERSION 0.3.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# External 3rd party libs that we include
#add_subdirectory(libs/libcuckoo EXCLUDE_FROM_ALL)

# Set include dirs and libraries for the executable.
#include_directories(libs/libcuckoo)

set(LIBUTHREADS_INCLUDE_DIRS
     "${CMAKE_CURRENT_SOURCE_DIR}/libuThreads")

# Targets that we develop
enable_testing()
add_subdirectory(libuThreads)
add_subdirectory(examples)