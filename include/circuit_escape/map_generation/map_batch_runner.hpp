#ifndef CIRCUIT_ESCAPE_MAP_BATCH_RUNNER_HPP
#define CIRCUIT_ESCAPE_MAP_BATCH_RUNNER_HPP

// Preparación concurrente de mapas (sección 6 del enunciado).
//   Fase 1: construir los mapas en paralelo  -> esperar a que terminen todos.
//   Fase 2: validar los mapas en paralelo    -> esperar a que terminen todos.
//   Después, la clasificación se hace en orden (select_levels).

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "circuit_escape/map_generation/map_types.hpp"

namespace circuit_escape {

// Funciones que se le pasan al runner (llamables, std::function).
// Así el runner no depende de cómo se construye ni de cómo se valida un mapa.
using BuildFunction = std::function<MapCandidate(int id, std::uint32_t seed)>;
using ValidateFunction = std::function<ValidationResult(const MapCandidate& map)>;

struct BatchConfig {
    int map_count{10};            // cuántas alternativas preparar (0 a 10)
    int workers{1};               // cuántos hilos usar (al menos 1)
    std::uint32_t base_seed{2026};// el mapa i usa la semilla base_seed + i
};

struct BatchReport {
    std::vector<MapCandidate> maps;          // mapas construidos sin error, en orden de id
    std::vector<ValidationResult> results;   // un resultado por mapa construido, en orden de id
    std::vector<std::string> errors;         // errores por mapa (construcción o validación)
    double build_ms{0.0};                    // tiempo de la fase 1
    double validate_ms{0.0};                 // tiempo de la fase 2
};

// Ejecuta task(0), task(1), ..., task(task_count - 1) repartidas entre 'workers' hilos.
// Cada hilo toma el siguiente índice libre de un contador atómico.
// La función termina cuando todos los hilos terminaron.
void run_in_parallel(int task_count, int workers, const std::function<void(int)>& task);

// Prepara el lote completo: fase 1 (construir) y fase 2 (validar).
BatchReport run_map_batch(const BatchConfig& config,
                          const BuildFunction& build_map,
                          const ValidateFunction& validate_map);

// true si dos lotes tienen los mismos mapas, costos, recorridos y diagnósticos.
// Sirve para comprobar que 1 hilo y varios hilos producen lo mismo.
bool same_results(const BatchReport& first, const BatchReport& second);

}  // namespace circuit_escape

#endif  // CIRCUIT_ESCAPE_MAP_BATCH_RUNNER_HPP
