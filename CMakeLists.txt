cmake_minimum_required(VERSION 3.20)

project(Proyecto1PG3)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(Proyecto1PG3
        app/main.cpp
)

target_include_directories(Proyecto1PG3 PRIVATE
        include
)