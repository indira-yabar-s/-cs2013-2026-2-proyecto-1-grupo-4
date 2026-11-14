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

# Hilos (std::jthread) para la preparacion concurrente de mapas
find_package(Threads REQUIRED)

# Núcleo del motor (sin dependencia de FTXUI, totalmente portable)
add_library(circuit_escape_core STATIC
        src/position.cpp
        src/controllers.cpp
        src/environment.cpp
        src/game_rules.cpp
        src/map_generation/map_validator.cpp
        src/map_generation/map_selector.cpp
        src/map_generation/map_batch_runner.cpp
)

target_include_directories(circuit_escape_core PUBLIC
        include
)

target_link_libraries(circuit_escape_core PUBLIC
        Threads::Threads
)

# Capa de presentación (interfaz de consola con FTXUI)
add_library(circuit_escape_ui STATIC
        src/console_ui.cpp
        src/game_application.cpp
)

target_include_directories(circuit_escape_ui PUBLIC
        include
)

target_link_libraries(circuit_escape_ui PUBLIC
        circuit_escape_core
        ftxui::ftxui
)

# Ejecutable principal
add_executable(Proyecto1PG3
        app/main.cpp
)
if (MINGW)
    target_link_options(Proyecto1PG3 PRIVATE -static)
endif()

target_link_libraries(Proyecto1PG3 PRIVATE
        circuit_escape_ui
)

# Generador de mapas (preparacion concurrente)
add_executable(map_generator
        app/map_generator_main.cpp
)
if (MINGW)
    target_link_options(map_generator PRIVATE -static)
endif()

target_link_libraries(map_generator PRIVATE
        circuit_escape_core
)

# -----------------------------------------------------------------------------
# Pruebas automáticas (CTest)
# -----------------------------------------------------------------------------
enable_testing()

add_executable(grid_test tests/grid_test.cpp)
target_link_libraries(grid_test PRIVATE circuit_escape_core)
add_test(NAME grid_test COMMAND grid_test)

add_executable(environment_test tests/environment_test.cpp)
target_link_libraries(environment_test PRIVATE circuit_escape_core)
add_test(NAME environment_test COMMAND environment_test)

add_executable(interactions_test tests/interactions_test.cpp)
target_link_libraries(interactions_test PRIVATE circuit_escape_core)
add_test(NAME interactions_test COMMAND interactions_test)

add_executable(controllers_test tests/controllers_test.cpp)
target_link_libraries(controllers_test PRIVATE circuit_escape_ui)
add_test(NAME controllers_test COMMAND controllers_test)
add_executable(map_selector_test tests/map_generation/map_selector_test.cpp)
target_link_libraries(map_selector_test PRIVATE circuit_escape_core)
add_test(NAME map_selector_test COMMAND map_selector_test)

add_executable(route_evaluation_test tests/map_generation/route_evaluation_test.cpp)
target_link_libraries(route_evaluation_test PRIVATE circuit_escape_core)
add_test(NAME route_evaluation_test COMMAND route_evaluation_test)

add_executable(batch_runner_test tests/concurrency/batch_runner_test.cpp)
target_link_libraries(batch_runner_test PRIVATE circuit_escape_core)
add_test(NAME batch_runner_test COMMAND batch_runner_test)
