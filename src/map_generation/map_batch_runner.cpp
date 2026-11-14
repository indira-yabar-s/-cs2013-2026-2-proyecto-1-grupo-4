#include "circuit_escape/map_generation/map_batch_runner.hpp"

#include <atomic>
#include <chrono>
#include <exception>
#include <optional>
#include <stdexcept>
#include <thread>

namespace circuit_escape {

namespace {

using Clock = std::chrono::steady_clock;

double elapsed_ms(Clock::time_point start, Clock::time_point end) {
    return std::chrono::duration<double, std::milli>(end - start).count();
}

}  // namespace

void run_in_parallel(int task_count, int workers, const std::function<void(int)>& task) {
    // Contador compartido: indica cuál es la siguiente tarea libre.
    // Es atómico, así que dos hilos nunca reciben el mismo índice.
    std::atomic<int> next_task{0};

    {
        std::vector<std::jthread> threads;
        for (int w = 0; w < workers; ++w) {
            threads.emplace_back([&]() {
                while (true) {
                    const int index = next_task++;   // tomar una tarea (operación atómica)
                    if (index >= task_count) {
                        break;                       // ya no quedan tareas
                    }
                    task(index);
                }
            });
        }
    }  // Aquí se destruye el vector: cada std::jthread hace join() automáticamente.
       // Por eso, al salir de este bloque, todas las tareas ya terminaron.
}

BatchReport run_map_batch(const BatchConfig& config,
                          const BuildFunction& build_map,
                          const ValidateFunction& validate_map) {
    if (config.workers < 1) {
        throw std::invalid_argument("Se necesita al menos un trabajador.");
    }
    if (config.map_count < 0 || config.map_count > 10) {
        throw std::invalid_argument("La cantidad de mapas debe estar entre 0 y 10.");
    }

    BatchReport report;
    const int count = config.map_count;
    if (count == 0) {
        return report;  // lote vacío: no hay nada que construir
    }

    // Identificadores y semillas se preparan ANTES de lanzar los hilos.
    // Así el mapa 3 siempre usa la misma semilla, sin importar qué hilo lo construya.
    std::vector<int> ids(count);
    std::vector<std::uint32_t> seeds(count);
    for (int i = 0; i < count; ++i) {
        ids[i] = i + 1;
        seeds[i] = config.base_seed + static_cast<std::uint32_t>(i);
    }

    // ---------------- Fase 1: construcción en paralelo ----------------
    // Los resultados se reservan antes: cada tarea escribe SOLO en su posición i.
    // Como ninguna posición es compartida, no hace falta mutex para escribir.
    std::vector<std::optional<MapCandidate>> built(count);
    std::vector<std::string> build_errors(count);

    const Clock::time_point build_start = Clock::now();
    run_in_parallel(count, config.workers, [&](int i) {
        try {
            built[i] = build_map(ids[i], seeds[i]);
        } catch (const std::exception& error) {
            build_errors[i] = "Mapa " + std::to_string(ids[i]) + " (construccion): " + error.what();
        }
    });
    const Clock::time_point build_end = Clock::now();
    report.build_ms = elapsed_ms(build_start, build_end);

    // Reunir resultados desde el hilo principal, en orden de id.
    for (int i = 0; i < count; ++i) {
        if (built[i]) {
            report.maps.push_back(*built[i]);
        } else if (!build_errors[i].empty()) {
            report.errors.push_back(build_errors[i]);
        }
    }

    // ---------------- Fase 2: validación en paralelo ----------------
    // Empieza solo cuando terminó toda la construcción.
    // Los hilos solo LEEN report.maps (no lo modifican), así que pueden compartirlo.
    const int built_count = static_cast<int>(report.maps.size());
    std::vector<ValidationResult> validated(built_count);
    std::vector<std::string> validate_errors(built_count);

    const Clock::time_point validate_start = Clock::now();
    run_in_parallel(built_count, config.workers, [&](int i) {
        const MapCandidate& map = report.maps[i];
        try {
            validated[i] = validate_map(map);
        } catch (const std::exception& error) {
            validated[i].map_id = map.id;
            validated[i].accepted = false;
            validated[i].message = std::string("Error al validar: ") + error.what();
            validate_errors[i] = "Mapa " + std::to_string(map.id) + " (validacion): " + error.what();
        }
    });
    const Clock::time_point validate_end = Clock::now();
    report.validate_ms = elapsed_ms(validate_start, validate_end);

    report.results = validated;
    for (const std::string& error : validate_errors) {
        if (!error.empty()) {
            report.errors.push_back(error);
        }
    }
    return report;
}

bool same_results(const BatchReport& first, const BatchReport& second) {
    if (first.maps.size() != second.maps.size() ||
        first.results.size() != second.results.size() ||
        first.errors != second.errors) {
        return false;
    }
    for (std::size_t i = 0; i < first.maps.size(); ++i) {
        const MapCandidate& a = first.maps[i];
        const MapCandidate& b = second.maps[i];
        if (a.id != b.id || a.seed != b.seed || !(a.grid == b.grid)) {
            return false;
        }
    }
    for (std::size_t i = 0; i < first.results.size(); ++i) {
        const ValidationResult& a = first.results[i];
        const ValidationResult& b = second.results[i];
        if (a.map_id != b.map_id || a.accepted != b.accepted ||
            a.energy_status != b.energy_status || a.best_order != b.best_order ||
            a.min_cost != b.min_cost || a.route != b.route ||
            a.winning_game_verified != b.winning_game_verified) {
            return false;
        }
    }
    return true;
}

}  // namespace circuit_escape
