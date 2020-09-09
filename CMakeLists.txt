cmake_minimum_required(VERSION 3.15)
project(asio_redis_client)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_CXX_FLAGS "-fprofile-instr-generate -fcoverage-mapping")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")


include_directories(
        /usr/local/include
)

link_directories(
        /usr/local/lib
)

add_executable(asio_redis_client
        main.cpp
)