#include "circuit_escape/map_generation/map_validator.hpp"

namespace circuit_escape {

std::optional<int> sum_route(std::optional<int> first,
                             std::optional<int> second,
                             std::optional<int> third) {
    if (!first || !second || !third) {
        return std::nullopt;
    }
    return *first + *second + *third;
}

namespace {

// Lista los tramos que no tienen camino, para explicar por qué se rechaza el mapa.
std::string missing_segments(const SegmentCosts& costs) {
    std::string text;
    if (!costs.start_to_a) text += " inicio->A";
    if (!costs.start_to_b) text += " inicio->B";
    if (!costs.a_to_b)     text += " A->B";
    if (!costs.b_to_a)     text += " B->A";
    if (!costs.a_to_exit)  text += " A->salida";
    if (!costs.b_to_exit)  text += " B->salida";
    return text;
}

}  // namespace

ValidationResult evaluate_routes(int map_id,
                                 const SegmentCosts& costs,
                                 int initial_energy,
                                 bool has_recharges) {
    ValidationResult result;
    result.map_id = map_id;

    const std::optional<int> route_ab = sum_route(costs.start_to_a, costs.a_to_b, costs.b_to_exit);
    const std::optional<int> route_ba = sum_route(costs.start_to_b, costs.b_to_a, costs.a_to_exit);

    // 1) Si ningún recorrido es posible, el mapa se rechaza.
    if (!route_ab && !route_ba) {
        result.accepted = false;
        result.energy_status = EnergyStatus::Unreachable;
        result.message = "Objetivo inaccesible. Tramos sin camino:" + missing_segments(costs);
        return result;
    }

    // 2) Elegir el recorrido más barato. Si cuestan lo mismo, se elige A->B (regla fija).
    if (route_ab && (!route_ba || *route_ab <= *route_ba)) {
        result.best_order = RouteOrder::AThenB;
        result.min_cost = *route_ab;
    } else {
        result.best_order = RouteOrder::BThenA;
        result.min_cost = *route_ba;
    }

    // 3) Comparar con la energía inicial. Llegar con energía 0 no permite ganar.
    if (initial_energy > result.min_cost) {
        result.accepted = true;
        result.energy_status = EnergyStatus::Enough;
        result.message = "Alcanza sin recargas.";
    } else if (has_recharges) {
        result.accepted = true;
        result.energy_status = EnergyStatus::NeedsRecharge;
        result.message = "No alcanza sin recargas: falta demostrar una partida ganadora.";
    } else {
        result.accepted = false;
        result.energy_status = EnergyStatus::NotEnough;
        result.message = "Energia insuficiente y el mapa no tiene recargas.";
    }
    return result;
}

std::string to_text(EnergyStatus status) {
    switch (status) {
        case EnergyStatus::Enough:        return "alcanza";
        case EnergyStatus::NeedsRecharge: return "requiere recargas";
        case EnergyStatus::NotEnough:     return "insuficiente";
        case EnergyStatus::Unreachable:   return "inaccesible";
    }
    return "desconocido";
}

std::string to_text(RouteOrder order) {
    return order == RouteOrder::AThenB ? "inicio->A->B->salida" : "inicio->B->A->salida";
}

}  // namespace circuit_escape
