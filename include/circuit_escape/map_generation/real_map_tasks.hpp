#ifndef CIRCUIT_ESCAPE_REAL_MAP_TASKS_HPP
#define CIRCUIT_ESCAPE_REAL_MAP_TASKS_HPP

// Construccion y validacion REALES de un mapa (reemplazan a fake_map_tasks.hpp).
//   build_map    : Kruskal (generateMaze) + colocacion de interruptores A, B y vigilante.
//   validate_map : Dijkstra desde inicio, A y B + evaluacion de recorridos + partida de prueba.

#include <cstddef>
#include <cstdint>

#include "circuit_escape/game_rules.h"
#include "circuit_escape/map_generation/map_types.hpp"

namespace circuit_escape {

struct MapSize {
    std::size_t rows{15};
    std::size_t columns{21};
};

// Construye el mapa 'id' con la semilla dada. Misma semilla -> mismo mapa.
MapCandidate build_map(int id, std::uint32_t seed, MapSize size);

// Valida un mapa con las reglas de la dificultad indicada.
ValidationResult validate_map(const MapCandidate& map, const GameRules& rules);

}  // namespace circuit_escape

#endif  // CIRCUIT_ESCAPE_REAL_MAP_TASKS_HPP
