// map_generator: prepara, valida y elige los mapas por nivel.
// TEMPORAL: usa construcción y validación de prueba hasta que estén Kruskal y Dijkstra.
//
// Uso:
//   map_generator --maps 10 --workers 4 --seed 2026
//   map_generator --maps 10 --workers 4 --compare     (compara 1 hilo contra N hilos)

#include <exception>
#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>

#include "circuit_escape/map_generation/fake_map_tasks.hpp"
#include "circuit_escape/map_generation/map_batch_runner.hpp"
#include "circuit_escape/map_generation/map_selector.hpp"
#include "circuit_escape/map_generation/map_validator.hpp"

using namespace circuit_escape;

namespace {

void print_report(const BatchReport& report) {
    std::cout << "Construccion: " << std::fixed << std::setprecision(1) << report.build_ms << " ms"
              << " | Validacion: " << report.validate_ms << " ms\n\n";

    std::cout << "Mapa | Costo | Energia           | Recorrido\n";
    for (const ValidationResult& result : report.results) {
        std::cout << std::setw(4) << result.map_id << " | "
                  << std::setw(5) << result.min_cost << " | "
                  << std::left << std::setw(17) << to_text(result.energy_status) << std::right << " | "
                  << (result.best_order ? to_text(*result.best_order) : "-") << '\n';
    }
    for (const std::string& error : report.errors) {
        std::cout << "ERROR: " << error << '\n';
    }
}

void print_selection(const LevelSelection& selection) {
    std::cout << '\n' << selection.message << '\n';
    if (!selection.ok) {
        return;
    }
    std::cout << "Promedio: " << std::setprecision(2) << selection.average_cost << '\n'
              << "Facil:   mapa " << selection.easy_id << " (costo " << selection.easy_cost << ")\n"
              << "Medio:   mapa " << selection.medium_id << " (costo " << selection.medium_cost << ")\n"
              << "Dificil: mapa " << selection.hard_id << " (costo " << selection.hard_cost << ")\n";
}

}  // namespace

int main(int argc, char* argv[]) {
    BatchConfig config;
    bool compare = false;

    try {
        for (int i = 1; i < argc; ++i) {
            const std::string_view arg = argv[i];
            if (arg == "--maps" && i + 1 < argc) {
                config.map_count = std::stoi(argv[++i]);
            } else if (arg == "--workers" && i + 1 < argc) {
                config.workers = std::stoi(argv[++i]);
            } else if (arg == "--seed" && i + 1 < argc) {
                config.base_seed = static_cast<std::uint32_t>(std::stoul(argv[++i]));
            } else if (arg == "--compare") {
                compare = true;
            } else {
                std::cerr << "Opcion desconocida: " << arg << '\n';
                return 1;
            }
        }

        std::cout << "Preparando " << config.map_count << " mapas con "
                  << config.workers << " trabajador(es), semilla " << config.base_seed << "\n\n";

        const BatchReport report = run_map_batch(config, fake_build_map, fake_validate_map);
        print_report(report);
        print_selection(select_levels(report.results));

        if (compare) {
            BatchConfig single = config;
            single.workers = 1;
            const BatchReport single_report = run_map_batch(single, fake_build_map, fake_validate_map);
            std::cout << "\n--- Comparacion ---\n"
                      << "1 hilo:  construccion " << single_report.build_ms
                      << " ms, validacion " << single_report.validate_ms << " ms\n"
                      << config.workers << " hilos: construccion " << report.build_ms
                      << " ms, validacion " << report.validate_ms << " ms\n"
                      << "Mismos resultados: " << (same_results(report, single_report) ? "SI" : "NO") << '\n';
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
