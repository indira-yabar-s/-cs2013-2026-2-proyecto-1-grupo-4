// Pruebas de la preparación concurrente de mapas (secciones 6 y 7 del enunciado).

#include <atomic>
#include <set>
#include <stdexcept>
#include <vector>

#include "../test_check.hpp"
#include "circuit_escape/map_generation/fake_map_tasks.hpp"
#include "circuit_escape/map_generation/map_batch_runner.hpp"
#include "circuit_escape/map_generation/map_selector.hpp"

using namespace circuit_escape;

int main() {
    // 1) Mismos resultados con 1 hilo y con varios hilos.
    BatchConfig config;
    config.map_count = 10;
    config.base_seed = 2026;

    config.workers = 1;
    const BatchReport single = run_map_batch(config, fake_build_map, fake_validate_map);
    for (int workers : {2, 3, 4, 8}) {
        config.workers = workers;
        const BatchReport parallel = run_map_batch(config, fake_build_map, fake_validate_map);
        check(same_results(single, parallel),
              "mismos resultados con 1 y " + std::to_string(workers) + " hilos");

        const LevelSelection a = select_levels(single.results);
        const LevelSelection b = select_levels(parallel.results);
        check(a.ok == b.ok && a.easy_id == b.easy_id && a.medium_id == b.medium_id &&
                  a.hard_id == b.hard_id,
              "mismos niveles con " + std::to_string(workers) + " hilos");
    }

    // 2) Cada mapa se construye una sola vez (ni repetidos ni perdidos) y en orden de id.
    check(single.maps.size() == 10, "se construyen 10 mapas");
    for (std::size_t i = 0; i < single.maps.size(); ++i) {
        check(single.maps[i].id == static_cast<int>(i) + 1, "mapas en orden de id");
        check(single.results[i].map_id == single.maps[i].id, "resultado en la posicion de su mapa");
        check(single.maps[i].seed == 2026 + i, "semilla preparada antes de los hilos");
    }

    // 3) run_in_parallel ejecuta cada tarea exactamente una vez.
    std::vector<int> times_run(50, 0);
    run_in_parallel(50, 4, [&](int i) { times_run[i] += 1; });
    bool each_once = true;
    for (int value : times_run) {
        if (value != 1) each_once = false;
    }
    check(each_once, "cada tarea se ejecuta una sola vez");

    // 4) Lote vacio.
    config.map_count = 0;
    config.workers = 3;
    const BatchReport empty = run_map_batch(config, fake_build_map, fake_validate_map);
    check(empty.maps.empty() && empty.results.empty() && empty.errors.empty(), "lote vacio");

    // 5) Fallo de una tarea: el mapa 3 falla al construirse y los demas siguen.
    auto failing_build = [](int id, std::uint32_t seed) {
        if (id == 3) {
            throw std::runtime_error("fallo simulado");
        }
        return fake_build_map(id, seed);
    };
    config.map_count = 5;
    const BatchReport with_error = run_map_batch(config, failing_build, fake_validate_map);
    check(with_error.maps.size() == 4, "fallo de construccion: quedan 4 mapas");
    check(with_error.errors.size() == 1, "fallo de construccion: se registra 1 error");

    // Fallo al validar el mapa 2: se marca rechazado y se registra el error.
    auto failing_validate = [](const MapCandidate& map) {
        if (map.id == 2) {
            throw std::runtime_error("fallo simulado");
        }
        return fake_validate_map(map);
    };
    const BatchReport validate_error = run_map_batch(config, fake_build_map, failing_validate);
    check(validate_error.results.size() == 5, "fallo de validacion: 5 resultados");
    check(!validate_error.results[1].accepted, "fallo de validacion: mapa 2 rechazado");
    check(validate_error.errors.size() == 1, "fallo de validacion: se registra 1 error");

    // 6) Configuraciones invalidas.
    bool rejected_workers = false;
    try {
        BatchConfig bad;
        bad.workers = 0;
        run_map_batch(bad, fake_build_map, fake_validate_map);
    } catch (const std::invalid_argument&) {
        rejected_workers = true;
    }
    check(rejected_workers, "0 trabajadores se rechaza");

    bool rejected_count = false;
    try {
        BatchConfig bad;
        bad.map_count = 11;
        run_map_batch(bad, fake_build_map, fake_validate_map);
    } catch (const std::invalid_argument&) {
        rejected_count = true;
    }
    check(rejected_count, "mas de 10 mapas se rechaza");

    // 7) Se miden las dos fases por separado.
    check(single.build_ms > 0.0 && single.validate_ms > 0.0, "se miden ambas fases");

    return test_exit_code();
}
