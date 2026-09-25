cmake_minimum_required(VERSION 3.20)

project(Proyecto1PG3)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
        ftxui
        GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI
        GIT_TAG v7.0.3
)

FetchContent_MakeAvailable(ftxui)

add_executable(Proyecto1PG3

        app/main.cpp

        src/position.cpp
        src/controllers.cpp
        src/environment.cpp
        src/game_rules.cpp
        src/console_ui.cpp
        src/game_application.cpp

        include/circuit_escape/position.h
        include/circuit_escape/cells.h
        include/circuit_escape/grid.h
        include/circuit_escape/grid_algorithms.h
        include/circuit_escape/overloaded.h
        include/circuit_escape/scenario.h
        include/circuit_escape/observation.h
        include/circuit_escape/controllers.h
        include/circuit_escape/environment.hpp
        include/circuit_escape/game_rules.h
        include/circuit_escape/console_ui.h
        include/circuit_escape/game_application.h
)

target_include_directories(Proyecto1PG3 PRIVATE
        include
)

target_link_libraries(Proyecto1PG3 PRIVATE
        ftxui::ftxui
)