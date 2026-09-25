cmake_minimum_required(VERSION 3.20)

project(Proyecto1PG3)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(Proyecto1PG3

        app/main.cpp

        src/position.cpp
        src/controllers.cpp

        include/circuit_escape/position.h
        include/circuit_escape/cells.h
        include/circuit_escape/grid.h
        include/circuit_escape/grid_algorithms.h
        include/circuit_escape/overloaded.h
        include/circuit_escape/scenario.h

        include/circuit_escape/observation.h
        include/circuit_escape/controllers.h
)

target_include_directories(Proyecto1PG3 PRIVATE
        include
)