#ifndef CIRCUIT_ESCAPE_REAL_MAP_TASKS_HPP
#define CIRCUIT_ESCAPE_REAL_MAP_TASKS_HPP

// Construccion y validacion REALES de un mapa (reemplazan a fake_map_tasks.hpp).
//   build_map    : Kruskal (generateMaze) + colocacion de interruptores A, B y vigilante.
//   validate_map : Dijkstra desde inicio, A y B + evaluacion de recorridos + partida de prueba.

#include <cstddef>
#include <cstdint>
#include <string>

#include "circuit_escape/game_rules.h"
#include "circuit_escape/map_generation/map_types.hpp"

namespace circuit_escape {

struct MapSize {
    std::size_t rows{15};
    std::size_t columns{21};
};

// Limites del tablero (seccion 2.2): se rechazan tamanos que no permiten colocar
// los elementos o que exceden los recursos disponibles.
inline constexpr std::size_t kMinMapSide = 7;
inline constexpr std::size_t kMaxMapSide = 200;
inline constexpr std::size_t kMaxMapCells = 20000;

// Lanza std::invalid_argument con un mensaje claro si el tamano no es valido.
void validate_map_size(MapSize size);

// Convierte un texto en una dimension. Rechaza vacios, letras, ceros y negativos.
std::size_t parse_dimension(const std::string& text, const std::string& name);

// Reglas de un lote: las mismas para todos sus mapas. La energia y el limite de
// turnos crecen con el tamano del tablero.
GameRules rules_for_size(MapSize size);

// Construye el mapa 'id' con la semilla dada. Misma semilla -> mismo mapa.
MapCandidate build_map(int id, std::uint32_t seed, MapSize size);

// Valida un mapa con las reglas de la dificultad indicada.
ValidationResult validate_map(const MapCandidate& map, const GameRules& rules);

}  // namespace circuit_escape

#endif  // CIRCUIT_ESCAPE_REAL_MAP_TASKS_HPP
