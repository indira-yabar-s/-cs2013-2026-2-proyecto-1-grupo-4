#ifndef CIRCUIT_ESCAPE_MAP_SELECTOR_HPP
#define CIRCUIT_ESCAPE_MAP_SELECTOR_HPP

#include <string>
#include <vector>

#include "circuit_escape/map_generation/map_types.hpp"

namespace circuit_escape {

// Resultado de elegir un mapa por nivel.
struct LevelSelection {
    bool ok{false};
    std::string message;
    int easy_id{0};
    int medium_id{0};
    int hard_id{0};
    int easy_cost{0};
    int medium_cost{0};
    int hard_cost{0};
    double average_cost{0.0};
};

// Sección 2.4 del enunciado:
//   - solo cuentan los mapas aceptados con victoria comprobada,
//   - fácil = menor costo, difícil = mayor costo,
//   - medio = el más cercano al promedio, sin contar el mínimo ni el máximo,
//   - empates: gana el id menor,
//   - se necesitan al menos 3 costos distintos.
LevelSelection select_levels(const std::vector<ValidationResult>& results);

}  // namespace circuit_escape

#endif  // CIRCUIT_ESCAPE_MAP_SELECTOR_HPP
