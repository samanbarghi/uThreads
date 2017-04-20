macro(add_example name)
  add_executable(${name} ${CMAKE_CURRENT_SOURCE_DIR}/src/${name}.cpp)

  if(${name} STREQUAL "webserver")
    target_sources(${name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/http-parser/http_parser.c)
  endif()
  target_link_libraries(${name} uThreads ${LD_FLAGS})

endmacro()

include_directories(${LIBUTHREADS_INCLUDE_DIRS} .)
SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_SOURCE_DIR}/http-parser/http_parser.c PROPERTIES LANGUAGE CXX )

add_example(CExample)
add_example(ConditionVariable)
add_example(EchoClient)
add_example(EchoServer)
add_example(Fibonacci)
add_example(FibonacciFJ)
add_example(IdCheck)
add_example(Joinable)
add_example(Mutex)
add_example(OwnerLock)
add_example(Semaphore)
add_example(webserver)

