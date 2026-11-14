#ifndef CIRCUIT_ESCAPE_FAKE_MAP_TASKS_HPP
#define CIRCUIT_ESCAPE_FAKE_MAP_TASKS_HPP

// TEMPORAL: construcción y validación de prueba.
// Sirven para probar la concurrencia mientras no existen Kruskal (bloque 1) y Dijkstra (bloque 2).
// Cuando estén listos, se reemplazan por las funciones reales; el runner no cambia.

#include <chrono>
#include <cstdint>
#include <random>
#include <thread>

#include "circuit_escape/map_generation/map_types.hpp"
#include "circuit_escape/map_generation/map_validator.hpp"

namespace circuit_escape {

inline constexpr int kFakeInitialEnergy = 30;

// Simula construir un mapa: tarda un poco y devuelve un mapa fijo con su id y semilla.
inline MapCandidate fake_build_map(int id, std::uint32_t seed) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));  // simula trabajo

    MapCandidate map;
    map.id = id;
    map.seed = seed;
    map.rows = {
        "#######",
        "#@.A..#",
        "#.###.#",
        "#B.V.S#",
        "#######",
    };
    map.start = {1, 1};
    map.switch_a = {1, 3};
    map.switch_b = {3, 1};
    map.guard = {3, 3};
    map.exit = {3, 5};
    map.has_recharges = (seed % 2 == 0);
    return map;
}

// Simula validar: inventa los costos de cada tramo a partir de la semilla
// (misma semilla -> mismos costos) y usa la evaluación real de recorridos.
inline ValidationResult fake_validate_map(const MapCandidate& map) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));  // simula trabajo

    std::mt19937 generator(map.seed);
    std::uniform_int_distribution<int> cost(3, 12);

    SegmentCosts costs;
    costs.start_to_a = cost(generator);
    costs.start_to_b = cost(generator);
    costs.a_to_b = cost(generator);
    costs.b_to_a = cost(generator);
    costs.a_to_exit = cost(generator);
    costs.b_to_exit = cost(generator);

    ValidationResult result = evaluate_routes(map.id, costs, kFakeInitialEnergy, map.has_recharges);
    // TEMPORAL: la victoria se comprobará jugando una partida real con el motor.
    result.winning_game_verified = (result.energy_status == EnergyStatus::Enough);
    return result;
}

}  // namespace circuit_escape

#endif  // CIRCUIT_ESCAPE_FAKE_MAP_TASKS_HPP
