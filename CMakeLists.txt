cmake_minimum_required(VERSION 4.2)
project(os1)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(os1

        Thread.cpp
        Thread.h
        ThreadIDManager.cpp
        ThreadIDManager.h
        uthreads.cpp
        uthreads.h
        utils.h
        tests/test_6_cross_terminate_and_quantums.cpp)
