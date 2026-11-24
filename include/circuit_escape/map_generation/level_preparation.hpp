#ifndef CIRCUIT_ESCAPE_LEVEL_PREPARATION_HPP
#define CIRCUIT_ESCAPE_LEVEL_PREPARATION_HPP

// Prepara un lote completo y arma los tres niveles listos para guardar o jugar:
//   run_map_batch (Kruskal en paralelo -> Dijkstra en paralelo) + select_levels (en orden).

#include <cstdint>
#include <optional>

#include "circuit_escape/game_level.h"
#include "circuit_escape/map_generation/map_batch_runner.hpp"
#include "circuit_escape/map_generation/map_selector.hpp"
#include "circuit_escape/map_generation/real_map_tasks.hpp"

namespace circuit_escape {

struct PreparedLevels {
    BatchReport report;          // mapas, resultados, errores y tiempos de cada fase
    LevelSelection selection;    // ok = se pudieron elegir los tres niveles
    GameRules rules;             // reglas comunes del lote
    std::optional<GameLevel> easy;
    std::optional<GameLevel> medium;
    std::optional<GameLevel> hard;
};

// Valida el tamano (std::invalid_argument si no sirve) y prepara el lote.
PreparedLevels prepare_levels(MapSize size, const BatchConfig& config);

}  // namespace circuit_escape

#endif  // CIRCUIT_ESCAPE_LEVEL_PREPARATION_HPP
