cmake_minimum_required(VERSION 3.20)

project(escape_circuit_suite LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

FetchContent_Declare(
        ftxui
        GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI
        GIT_TAG v7.0.3
)

FetchContent_MakeAvailable(ftxui)

# Hilos (std::jthread) para la preparacion concurrente de mapas
find_package(Threads REQUIRED)

# -----------------------------------------------------------------------------
# escape_core: biblioteca compartida por los dos ejecutables (sin FTXUI)
# -----------------------------------------------------------------------------
add_library(escape_core STATIC
        src/position.cpp
        src/controllers.cpp
        src/environment.cpp
        src/game_rules.cpp
        src/scenario.cpp
        src/disjoint_set.cpp
        src/maze_generator.cpp
        src/map_generation/map_validator.cpp
        src/map_generation/map_selector.cpp
        src/map_generation/map_batch_runner.cpp
        src/map_generation/real_map_tasks.cpp
        src/map_generation/level_preparation.cpp
)

target_include_directories(escape_core PUBLIC include)
target_link_libraries(escape_core PUBLIC Threads::Threads)

# Capa de presentacion (interfaz de consola con FTXUI), solo la usa el juego
add_library(escape_ui STATIC
        src/console_ui.cpp
        src/game_application.cpp
)

target_include_directories(escape_ui PUBLIC include)
target_link_libraries(escape_ui PUBLIC
        escape_core
        ftxui::ftxui
)

# -----------------------------------------------------------------------------
# Ejecutables
# -----------------------------------------------------------------------------
# escape_circuit: carga el nivel elegido y ejecuta la partida
add_executable(escape_circuit app/main.cpp)
target_link_libraries(escape_circuit PRIVATE escape_ui)

# map_generator: prepara, valida y guarda los mapas por nivel
add_executable(map_generator app/map_generator_main.cpp)
target_link_libraries(map_generator PRIVATE escape_core)

if (MINGW)
    target_link_options(escape_circuit PRIVATE -static)
    target_link_options(map_generator PRIVATE -static)
endif()

# -----------------------------------------------------------------------------
# Instalacion (cmake --install build --prefix <carpeta>)
# -----------------------------------------------------------------------------
install(TARGETS escape_circuit map_generator RUNTIME DESTINATION bin)
install(DIRECTORY assets/maps DESTINATION assets FILES_MATCHING PATTERN "mapa_*.txt")
install(FILES README.md DESTINATION .)

# -----------------------------------------------------------------------------
# Pruebas automaticas (CTest). Se ejecutan desde la raiz del proyecto para
# encontrar assets/maps. Usan assert, por eso se compilan sin NDEBUG
# (tambien en Release).
# -----------------------------------------------------------------------------
enable_testing()

function(add_project_test name source library)
    add_executable(${name} ${source})
    target_link_libraries(${name} PRIVATE ${library})
    target_compile_options(${name} PRIVATE $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-UNDEBUG>)
    add_test(NAME ${name} COMMAND ${name} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endfunction()

# Proyecto 1 (conservadas)
add_project_test(grid_test tests/grid_test.cpp escape_core)
add_project_test(environment_test tests/environment_test.cpp escape_core)
add_project_test(interactions_test tests/interactions_test.cpp escape_core)
add_project_test(controllers_test tests/controllers_test.cpp escape_ui)
add_project_test(maze_test tests/maze_test.cpp escape_core)

# Proyecto 2: grafo, Kruskal y mapas
add_project_test(kruskal_openings_test tests/map_generation/kruskal_openings_test.cpp escape_core)

# Proyecto 2: busquedas (Dijkstra, BFS, ayuda H)
add_project_test(busquedas_test tests/busquedas_test.cpp escape_core)
add_project_test(help_test tests/pathfinding/help_test.cpp escape_core)

# Proyecto 2: validacion, dificultad y concurrencia
add_project_test(route_evaluation_test tests/map_generation/route_evaluation_test.cpp escape_core)
add_project_test(map_selector_test tests/map_generation/map_selector_test.cpp escape_core)
add_project_test(batch_runner_test tests/concurrency/batch_runner_test.cpp escape_core)
add_project_test(real_batch_test tests/concurrency/real_batch_test.cpp escape_core)

# Proyecto 2: reglas del juego, niveles guardados y dimensiones configurables
add_project_test(mission_test tests/game/mission_test.cpp escape_core)
add_project_test(level_io_test tests/game/level_io_test.cpp escape_core)
