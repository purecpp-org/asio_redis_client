cmake_minimum_required(VERSION 3.15)
project(asio_redis_client)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_CXX_FLAGS "-fprofile-instr-generate -fcoverage-mapping")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -Wextra")

ADD_DEFINITIONS(-DUSE_FUTURE)

include_directories(
        /usr/local/include
        /Users/yu/code/future
)

link_directories(
        /usr/local/lib
)

add_executable(asio_redis_client
        main.cpp
)

target_link_libraries(asio_redis_client -labsl_bad_variant_access)

add_executable(asio_redis_client_test
        asio_redis_client_test.cc
)

target_link_libraries(asio_redis_client_test gtest -labsl_bad_variant_access)