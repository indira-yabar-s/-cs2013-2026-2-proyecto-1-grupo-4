// map_generator: prepara, valida y guarda los mapas de los tres niveles.
//   Fase 1: Kruskal construye las alternativas (en paralelo, std::jthread).
//   Fase 2: Dijkstra las valida (en paralelo) y se comprueba una partida ganadora.
//   Despues, en orden: se elige un mapa por nivel (facil, medio, dificil).
//
// Uso:
//   map_generator --maps 10 --workers 4 --seed 2026 [--rows 15 --cols 21] [--save assets/maps] [--compare]

#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>

#include "circuit_escape/game_level.h"
#include "circuit_escape/game_rules.h"
#include "circuit_escape/map_generation/level_preparation.hpp"
#include "circuit_escape/map_generation/map_batch_runner.hpp"
#include "circuit_escape/map_generation/map_selector.hpp"
#include "circuit_escape/map_generation/map_validator.hpp"
#include "circuit_escape/map_generation/real_map_tasks.hpp"

using namespace circuit_escape;

namespace {

void print_help() {
    std::cout << "map_generator: prepara los mapas de los tres niveles\n"
              << "  --maps N      alternativas a preparar (0 a 10, por defecto 10)\n"
              << "  --workers W   hilos de trabajo (al menos 1, por defecto 1)\n"
              << "  --seed S      semilla base; el mapa i usa S + i (por defecto 2026)\n"
              << "  --rows F --cols C   tamano del tablero (minimo " << kMinMapSide << "x" << kMinMapSide
              << ", por defecto 15x21)\n"
              << "  --save CARPETA      guarda mapa_facil.txt, mapa_medio.txt y mapa_dificil.txt\n"
              << "  --compare     repite con 1 hilo y compara resultados y tiempos\n";
}

void print_report(const BatchReport& report) {
    std::cout << "Construccion: " << std::fixed << std::setprecision(1) << report.build_ms << " ms"
              << " | Validacion: " << report.validate_ms << " ms\n\n";

    std::cout << "Mapa | Costo | Energia           | Victoria | Recorrido\n";
    for (const ValidationResult& result : report.results) {
        std::cout << std::setw(4) << result.map_id << " | "
                  << std::setw(5) << result.min_cost << " | "
                  << std::left << std::setw(17) << to_text(result.energy_status) << " | "
                  << std::setw(8) << (result.winning_game_verified ? "si" : "no") << " | "
                  << std::right << (result.best_order ? to_text(*result.best_order) : "-") << '\n';
        if (!result.accepted) {
            std::cout << "       rechazado: " << result.message << '\n';
        }
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

void save_level(const std::filesystem::path& folder, const std::string& file, const GameLevel& level) {
    const auto path = folder / file;
    saveGameLevel(path, level);
    std::cout << "Guardado: " << path.string() << '\n';
}

}  // namespace

int main(int argc, char* argv[]) {
    BatchConfig config;
    MapSize size;
    bool compare = false;
    std::string save_folder;

    try {
        for (int i = 1; i < argc; ++i) {
            const std::string_view arg = argv[i];
            auto value = [&]() -> std::string {
                if (i + 1 >= argc) {
                    throw std::invalid_argument("Falta el valor de " + std::string(arg));
                }
                return argv[++i];
            };
            if (arg == "--help") {
                print_help();
                return 0;
            } else if (arg == "--maps") {
                config.map_count = std::stoi(value());
            } else if (arg == "--workers") {
                config.workers = std::stoi(value());
            } else if (arg == "--seed") {
                config.base_seed = static_cast<std::uint32_t>(std::stoul(value()));
            } else if (arg == "--rows") {
                size.rows = parse_dimension(value(), "--rows");
            } else if (arg == "--cols") {
                size.columns = parse_dimension(value(), "--cols");
            } else if (arg == "--save") {
                save_folder = value();
            } else if (arg == "--compare") {
                compare = true;
            } else {
                throw std::invalid_argument("Opcion desconocida: " + std::string(arg) + " (usa --help)");
            }
        }

        // Se valida antes de empezar: mensaje claro si el tablero no sirve.
        validate_map_size(size);
        const GameRules rules = rules_for_size(size);

        std::cout << "Preparando " << config.map_count << " mapas de " << size.rows << "x" << size.columns
                  << " con " << config.workers << " trabajador(es), semilla " << config.base_seed
                  << ", energia " << rules.initialEnergy << ", limite " << rules.turnLimit << " turnos\n\n";

        const PreparedLevels prepared = prepare_levels(size, config);
        const BatchReport& report = prepared.report;
        print_report(report);
        print_selection(prepared.selection);

        if (!prepared.selection.ok) {
            return 2;  // no se pueden ofrecer los tres niveles: generar otro lote
        }

        if (!save_folder.empty()) {
            std::filesystem::create_directories(save_folder);
            save_level(save_folder, "mapa_facil.txt", *prepared.easy);
            save_level(save_folder, "mapa_medio.txt", *prepared.medium);
            save_level(save_folder, "mapa_dificil.txt", *prepared.hard);
        }

        if (compare) {
            BatchConfig single = config;
            single.workers = 1;
            const BatchReport single_report = prepare_levels(size, single).report;
            std::cout << "\n--- Comparacion ---\n"
                      << "1 hilo:  construccion " << single_report.build_ms << " ms, validacion "
                      << single_report.validate_ms << " ms\n"
                      << config.workers << " hilos: construccion " << report.build_ms << " ms, validacion "
                      << report.validate_ms << " ms\n"
                      << "Mismos resultados: " << (same_results(report, single_report) ? "SI" : "NO") << '\n';
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
