// Pruebas de la evaluación de recorridos y energía (sección 2.3 del enunciado).

#include <optional>
#include <string>
#include <vector>

#include "../test_check.hpp"
#include "circuit_escape/map_generation/map_validator.hpp"

using namespace circuit_escape;

namespace {

// Costos del mapa de ejemplo del enunciado (seccion 3.3):
// inicio->B->A->salida cuesta 2 + 6 + 2 = 10; inicio->A->B->salida cuesta 8 + 6 + 3 = 17.
SegmentCosts example_costs() {
    SegmentCosts costs;
    costs.start_to_a = 8;
    costs.start_to_b = 2;
    costs.a_to_b = 6;
    costs.b_to_a = 6;
    costs.a_to_exit = 2;
    costs.b_to_exit = 3;
    return costs;
}

struct EnergyCase {
    std::string name;
    int energy;
    bool has_recharges;
    RouteEnergy expected_status;
    bool expected_accepted;
};

}  // namespace

int main() {
    // Energia contra un recorrido de costo 10 (ejemplo del enunciado).
    const std::vector<EnergyCase> cases = {
        {"energia 12: quedan 2", 12, false, RouteEnergy::Enough, true},
        {"energia 10: llega en cero, no gana", 10, false, RouteEnergy::NotEnough, false},
        {"energia 9 sin baterias", 9, false, RouteEnergy::NotEnough, false},
        {"energia 10 con baterias", 10, true, RouteEnergy::NeedsRecharge, true},
        {"energia 9 con baterias", 9, true, RouteEnergy::NeedsRecharge, true},
    };

    for (const EnergyCase& test : cases) {
        const ValidationResult result =
            evaluate_routes(1, example_costs(), test.energy, test.has_recharges);
        check(result.min_cost == 10, test.name + ": costo minimo 10");
        check(result.best_order == RouteOrder::BThenA, test.name + ": recorrido B->A");
        check(result.energy_status == test.expected_status, test.name + ": diagnostico");
        check(result.accepted == test.expected_accepted, test.name + ": aceptado");
    }

    // Si inicio->B no tiene camino, solo queda el recorrido A->B (costo 17).
    SegmentCosts only_ab = example_costs();
    only_ab.start_to_b = std::nullopt;
    const ValidationResult one_route = evaluate_routes(2, only_ab, 20, false);
    check(one_route.best_order == RouteOrder::AThenB, "solo un recorrido posible");
    check(one_route.min_cost == 17, "costo del unico recorrido");

    // Ningun recorrido posible: objetivo inaccesible.
    SegmentCosts blocked = example_costs();
    blocked.start_to_a = std::nullopt;
    blocked.start_to_b = std::nullopt;
    const ValidationResult unreachable = evaluate_routes(3, blocked, 50, true);
    check(unreachable.energy_status == RouteEnergy::Unreachable, "inaccesible");
    check(!unreachable.accepted, "inaccesible se rechaza");
    check(!unreachable.best_order.has_value(), "inaccesible sin recorrido");

    // Empate entre recorridos: se elige A->B (regla fija y reproducible).
    SegmentCosts tie;
    tie.start_to_a = 5; tie.a_to_b = 5; tie.b_to_exit = 5;
    tie.start_to_b = 5; tie.b_to_a = 5; tie.a_to_exit = 5;
    check(evaluate_routes(4, tie, 30, false).best_order == RouteOrder::AThenB, "empate elige A->B");

    // sum_route: un tramo sin camino anula el recorrido.
    check(sum_route(1, 2, 3) == 6, "suma de tramos");
    check(!sum_route(1, std::nullopt, 3).has_value(), "tramo sin camino");

    return test_exit_code();
}
