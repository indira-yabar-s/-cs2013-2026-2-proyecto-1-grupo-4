#ifndef CIRCUIT_ESCAPE_MAP_TYPES_HPP
#define CIRCUIT_ESCAPE_MAP_TYPES_HPP

// Tipos que se pasan entre los bloques de la preparación de mapas:
//   construcción (Kruskal, bloque 1) -> validación (Dijkstra, bloque 2/3) -> selección (bloque 3).

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "circuit_escape/position.h"

namespace circuit_escape {

// Mapa construido por un trabajador en la fase 1.
// Por ahora el tablero se guarda como filas de texto ('#', '.', 'A', ...)
// para no depender del Grid de tamaño variable. Se ajusta cuando el bloque 1 lo termine.
struct MapCandidate {
    int id{};                       // 1, 2, 3... se asigna antes de generar; sirve para desempatar
    std::uint32_t seed{};           // semilla usada para construir este mapa
    std::vector<std::string> rows;  // tablero en texto
    Position start{};
    Position switch_a{};
    Position switch_b{};
    Position exit{};
    Position guard{};
    bool has_recharges{false};      // true si el mapa tiene baterías
};

// Menor gasto de energía de cada tramo (lo calcula Dijkstra).
// std::nullopt significa que ese tramo no tiene camino.
// Los costos no son simétricos (A->B puede costar distinto que B->A),
// porque se paga la casilla a la que se entra.
struct SegmentCosts {
    std::optional<int> start_to_a;
    std::optional<int> start_to_b;
    std::optional<int> a_to_b;
    std::optional<int> b_to_a;
    std::optional<int> a_to_exit;
    std::optional<int> b_to_exit;
};

enum class RouteOrder {
    AThenB,  // Inicio -> A -> B -> Salida
    BThenA   // Inicio -> B -> A -> Salida
};

enum class EnergyStatus {
    Enough,         // energía > costo: escapa sin recargas
    NeedsRecharge,  // energía <= costo, pero hay baterías: falta demostrar una victoria
    NotEnough,      // energía <= costo y sin baterías: se rechaza
    Unreachable     // ningún recorrido completo es posible: se rechaza
};

// Resultado de validar un mapa (fase 2).
struct ValidationResult {
    int map_id{};
    bool accepted{false};
    EnergyStatus energy_status{EnergyStatus::Unreachable};
    std::optional<RouteOrder> best_order;
    int min_cost{0};                     // costo del recorrido más barato
    std::vector<Position> route;         // casillas del recorrido (lo llena Dijkstra)
    std::string message;                 // diagnóstico o motivo del rechazo
    bool winning_game_verified{false};   // true cuando se comprobó una partida ganadora
};

}  // namespace circuit_escape

#endif  // CIRCUIT_ESCAPE_MAP_TYPES_HPP
