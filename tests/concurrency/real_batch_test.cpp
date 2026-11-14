// Prueba de la preparacion concurrente con Kruskal y Dijkstra reales.

#include <variant>

#include "../test_check.hpp"
#include "circuit_escape/map_generation/map_batch_runner.hpp"
#include "circuit_escape/map_generation/map_selector.hpp"
#include "circuit_escape/map_generation/real_map_tasks.hpp"

using namespace circuit_escape;

int main() {
    const GameRules rules = rulesFor(Difficulty::standard);
    const MapSize size{15, 21};
    const BuildFunction build = [size](int id, std::uint32_t seed) { return build_map(id, seed, size); };
    const ValidateFunction validate = [&rules](const MapCandidate& map) { return validate_map(map, rules); };

    BatchConfig config;
    config.map_count = 10;
    config.base_seed = 2026;

    config.workers = 1;
    const BatchReport single = run_map_batch(config, build, validate);
    config.workers = 4;
    const BatchReport parallel = run_map_batch(config, build, validate);

    check(single.errors.empty(), "sin errores con 1 hilo");
    check(same_results(single, parallel), "mismos resultados con 1 y 4 hilos (Kruskal y Dijkstra reales)");

    const LevelSelection first = select_levels(single.results);
    const LevelSelection second = select_levels(parallel.results);
    check(first.ok, "el lote permite elegir tres niveles");
    check(first.easy_id == second.easy_id && first.medium_id == second.medium_id &&
              first.hard_id == second.hard_id,
          "mismos niveles con 1 y 4 hilos");

    // Cada mapa tiene A, B y vigilante en casillas libres y distintas.
    for (const MapCandidate& map : single.maps) {
        const Position cells[] = {map.start, map.exit, map.switch_a, map.switch_b, map.guard};
        bool free_and_distinct = true;
        for (int i = 0; i < 5; ++i) {
            if (std::holds_alternative<Wall>(map.grid.at(cells[i]))) free_and_distinct = false;
            for (int j = i + 1; j < 5; ++j) {
                if (cells[i] == cells[j]) free_and_distinct = false;
            }
        }
        check(free_and_distinct, "mapa " + std::to_string(map.id) + ": inicio, salida, A, B y vigilante distintos y libres");
    }

    // La misma semilla produce el mismo mapa.
    check(build_map(1, 77, size).grid == build_map(1, 77, size).grid, "misma semilla, mismo mapa");

    return test_exit_code();
}
