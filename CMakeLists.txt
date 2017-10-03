cmake_minimum_required(VERSION 3.5.2)

project(thread_pool)
set(CMAKE_CXX_STANDARD 14)

add_executable(
    ${PROJECT_NAME}
    thread_pool.hpp
    main.cpp
)
