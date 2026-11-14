#ifndef CIRCUIT_ESCAPE_MAP_VALIDATOR_HPP
#define CIRCUIT_ESCAPE_MAP_VALIDATOR_HPP

#include <optional>
#include <string>

#include "circuit_escape/map_generation/map_types.hpp"

namespace circuit_escape {

// Suma los tres tramos de un recorrido.
// Si algún tramo no tiene camino, el recorrido completo es inviable (std::nullopt).
std::optional<int> sum_route(std::optional<int> first,
                             std::optional<int> second,
                             std::optional<int> third);

// Sección 2.3 del enunciado:
//   1) compara Inicio->A->B->Salida con Inicio->B->A->Salida,
//   2) elige el de menor costo,
//   3) lo compara con la energía inicial.
// Los costos de cada tramo los calcula Dijkstra.
ValidationResult evaluate_routes(int map_id,
                                 const SegmentCosts& costs,
                                 int initial_energy,
                                 bool has_recharges);

// Texto para mostrar el diagnóstico en consola.
std::string to_text(EnergyStatus status);
std::string to_text(RouteOrder order);

}  // namespace circuit_escape

#endif  // CIRCUIT_ESCAPE_MAP_VALIDATOR_HPP
