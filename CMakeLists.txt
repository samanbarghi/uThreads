cmake_minimum_required(VERSION 2.8)
project(uThreads)

include_directories(${CMAKE_SOURCE_DIR}/include)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y -O3 -g -ggdb -m64 -fpermissive -mtls-direct-seg-refs -fno-extern-tls-init -pthread")

set(SOURCE_FILES
    src/io/EpollIOHandler.cpp
    src/io/IOHandler.cpp
    src/BlockingSync.cpp
    src/Cluster.cpp
    src/kThread.cpp
    src/Pthread.cpp
    src/Shim.cpp
    src/uThread.cpp
    src/uThreadPool.cpp,
    src/Stack.S)

set(SPIKE_FILES
    spike/ConditionVariableSpike.cpp
    spike/MessageQueueSpike.cpp
    spike/MutexSpike.cpp
    spike/OwnerLockSpike.cpp
    spike/SemaphoreSpike.cpp
    spike/SyscallSpike.cpp)

SET(CMAKE_C++_CREATE_SHARED_LIBRARY 1)
add_library(uThread SHARED ${SORUCE_FILES})
SET_TARGET_PROPERTIES(uThread PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(uThread PROPERTIES IBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
