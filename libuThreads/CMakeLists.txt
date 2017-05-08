# Define library. Only source files here!
project(libuThreads VERSION 0.3.0 LANGUAGES CXX)
enable_language(ASM)

file(GLOB_RECURSE LIBUTHREADS_SRCS src  "*.cpp")
file(GLOB_RECURSE LIBUTHREADS_HDRS uThreads  "*.h")

add_library(uThreads SHARED STATIC ${LIBUTHREADS_SRCS} src/runtime/Stack.S ${LIBUTHREADS_SRCS})

target_include_directories(uThreads PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/uThreads/generic>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/uThreads/io>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/uThreads/runtime>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/uThreads>
    $<INSTALL_INTERFACE:include/uThreads>
    PRIVATE src)

# If we have compiler requirements for this library, list them
# here
target_compile_features(uThreads
    PRIVATE cxx_range_for)
#    PUBLIC cxx_auto_type
#    PRIVATE cxx_variadic_templates)

# Depend on a library that we defined in the top-level file
target_link_libraries(uThreads pthread)

# 'make install' to the correct location
install(TARGETS uThreads EXPORT uThreadsConfig
    ARCHIVE  DESTINATION lib
    LIBRARY  DESTINATION lib
    RUNTIME  DESTINATION bin)  # This is for Windows
install(DIRECTORY uThreads/ DESTINATION include/uThreads)

# This makes the project importable from the install directory
# Put config file in per-project dir (name MUST match), can also
# just go into <prefix>/cmake.
install(EXPORT uThreadsConfig DESTINATION share/uThreads/cmake)

# This makes the project importable from the build directory
export(TARGETS uThreads FILE uThreadsConfig.cmake)

# Every library has unit tests, of course
#add_executable(testlib
#    test/testlib.cpp)

#target_link_libraries(testlib
#    lib)

#add_test(testlib testlib)