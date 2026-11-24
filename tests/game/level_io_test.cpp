// Niveles guardados, dimensiones configurables y partida ganadora reproducible
// (secciones 2.2, 2.5 y 7 del enunciado).

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../test_check.hpp"
#include "circuit_escape/controllers/guard_controller.hpp"
#include "circuit_escape/environment.hpp"
#include "circuit_escape/game_level.h"
#include "circuit_escape/map_generation/level_preparation.hpp"

using namespace circuit_escape;

namespace {

// Reproduce la partida ganadora guardada con las reglas completas.
bool replay_winning_game(const GameLevel& level) {
    NavigationEnvironment env(level.scenario.grid, level.scenario.start, level.rules);
    enable_guarded_mission(env, level.switch_a, level.switch_b, level.guard);
    for (Action action : level.solution) {
        if (env.isFinished()) {
            return false;  // termino antes de gastar todas las acciones
        }
        env.step(action);
    }
    return env.isFinished() && env.endReason() == EndReason::goalReached;
}

bool same_level(const GameLevel& a, const GameLevel& b) {
    return a.scenario.grid == b.scenario.grid && a.scenario.start == b.scenario.start &&
           a.scenario.exit == b.scenario.exit && a.switch_a == b.switch_a && a.switch_b == b.switch_b &&
           a.guard == b.guard && a.seed == b.seed && a.id == b.id && a.difficulty == b.difficulty &&
           a.min_cost == b.min_cost && a.solution == b.solution &&
           a.rules.initialEnergy == b.rules.initialEnergy && a.rules.turnLimit == b.rules.turnLimit &&
           a.rules.roughTerrainCost == b.rules.roughTerrainCost &&
           a.rules.batteryRecharge == b.rules.batteryRecharge &&
           a.rules.resourcePoints == b.rules.resourcePoints &&
           a.rules.trapEnergyPenalty == b.rules.trapEnergyPenalty;
}

// Busca una semilla para la que el lote ofrece los tres niveles (el enunciado pide
// otro lote con otra semilla cuando no hay tres costos distintos).
PreparedLevels prepare_with_some_seed(MapSize size, int workers) {
    BatchConfig config;
    config.map_count = 10;
    config.workers = workers;
    for (std::uint32_t seed = 1; seed <= 60; ++seed) {
        config.base_seed = seed * 100;
        PreparedLevels prepared = prepare_levels(size, config);
        if (prepared.selection.ok) {
            return prepared;
        }
    }
    throw std::runtime_error("ninguna semilla permitio los tres niveles");
}

void test_save_load_and_replay(const std::filesystem::path& folder) {
    const PreparedLevels prepared = prepare_with_some_seed(MapSize{15, 21}, 4);
    check(prepared.easy && prepared.medium && prepared.hard, "se arman los tres niveles");

    const std::vector<std::pair<std::string, const GameLevel*>> levels{
        {"mapa_facil.txt", &*prepared.easy},
        {"mapa_medio.txt", &*prepared.medium},
        {"mapa_dificil.txt", &*prepared.hard}};

    for (const auto& [file, level] : levels) {
        const auto path = folder / file;
        saveGameLevel(path, *level);
        const GameLevel loaded = loadGameLevel(path);
        check(same_level(*level, loaded), file + ": guardar y cargar conserva todos los datos");
        check(!loaded.solution.empty(), file + ": trae una partida ganadora guardada");
        check(replay_winning_game(loaded), file + ": la partida guardada gana al reproducirse");
        check(loaded.min_cost > 0, file + ": guarda el costo minimo");
        check(loaded.rules.initialEnergy > 0 && loaded.rules.turnLimit > 0, file + ": guarda energia y turnos");
    }

    check(prepared.easy->difficulty == "facil" && prepared.medium->difficulty == "medio" &&
              prepared.hard->difficulty == "dificil",
          "cada archivo guarda su nivel");
    check(prepared.easy->min_cost < prepared.medium->min_cost && prepared.medium->min_cost < prepared.hard->min_cost,
          "los costos crecen de facil a dificil");
}

void test_configurable_dimensions(const std::filesystem::path& folder) {
    for (const MapSize size : {MapSize{10, 15}, MapSize{20, 30}, MapSize{40, 60}}) {
        const PreparedLevels prepared = prepare_with_some_seed(size, 4);
        const std::string name = std::to_string(size.rows) + "x" + std::to_string(size.columns);

        const auto path = folder / ("nivel_" + name + ".txt");
        saveGameLevel(path, *prepared.medium);
        const GameLevel loaded = loadGameLevel(path);  // el tablero se crea con las filas/columnas del archivo

        check(loaded.scenario.grid.rows() == size.rows && loaded.scenario.grid.columns() == size.columns,
              name + ": el archivo carga con sus dimensiones");
        check(replay_winning_game(loaded), name + ": se puede jugar y ganar");
    }
}

void test_invalid_dimensions_are_rejected() {
    auto rejects = [](auto&& action) {
        try {
            action();
        } catch (const std::invalid_argument&) {
            return true;
        }
        return false;
    };

    check(rejects([] { (void)parse_dimension("0", "filas"); }), "dimension 0 rechazada");
    check(rejects([] { (void)parse_dimension("-5", "filas"); }), "dimension negativa rechazada");
    check(rejects([] { (void)parse_dimension("abc", "filas"); }), "dimension no numerica rechazada");
    check(rejects([] { (void)parse_dimension("", "filas"); }), "dimension vacia rechazada");
    check(rejects([] { validate_map_size(MapSize{5, 5}); }), "tablero que no permite colocar los elementos");
    check(rejects([] { validate_map_size(MapSize{1000, 1000}); }), "tablero que excede los recursos");
    check(rejects([] { (void)prepare_levels(MapSize{3, 3}, BatchConfig{}); }),
          "prepare_levels rechaza tamanos invalidos");
    check(!rejects([] { validate_map_size(MapSize{10, 15}); }), "10x15 es valido");
    check(!rejects([] { validate_map_size(MapSize{40, 60}); }), "40x60 es valido");
}

void test_invalid_level_files(const std::filesystem::path& folder) {
    // Sin A, B, vigilante y semilla.
    const auto path = folder / "incompleto.txt";
    {
        std::ofstream out(path);
        out << "#####\n#@.S#\n#####\n";
    }
    bool rejected = false;
    try {
        (void)loadGameLevel(path);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    check(rejected, "un nivel sin metadatos se rechaza con mensaje claro");

    bool missing = false;
    try {
        (void)loadGameLevel(folder / "no_existe.txt");
    } catch (const std::runtime_error&) {
        missing = true;
    }
    check(missing, "un archivo inexistente se informa");
}

void test_shipped_levels() {
    // Los mapas que se entregan en assets/maps deben cargarse y tener una victoria guardada.
    for (const char* file : {"mapa_facil.txt", "mapa_medio.txt", "mapa_dificil.txt"}) {
        const auto path = std::filesystem::path("assets/maps") / file;
        const GameLevel level = loadGameLevel(path);
        check(!level.solution.empty(), std::string(file) + ": tiene partida ganadora guardada");
        check(replay_winning_game(level), std::string(file) + ": la victoria guardada se reproduce");
    }
    const GameLevel easy = loadGameLevel("assets/maps/mapa_facil.txt");
    const GameLevel medium = loadGameLevel("assets/maps/mapa_medio.txt");
    const GameLevel hard = loadGameLevel("assets/maps/mapa_dificil.txt");
    check(easy.min_cost < medium.min_cost && medium.min_cost < hard.min_cost,
          "los mapas entregados tienen tres costos distintos y crecientes");
}

}  // namespace

int main() {
    const auto folder = std::filesystem::temp_directory_path() / "escape_circuit_level_io_test";
    std::filesystem::create_directories(folder);

    test_save_load_and_replay(folder);
    test_configurable_dimensions(folder);
    test_invalid_dimensions_are_rejected();
    test_invalid_level_files(folder);
    test_shipped_levels();

    std::error_code ignored;
    std::filesystem::remove_all(folder, ignored);
    return test_exit_code();
}
