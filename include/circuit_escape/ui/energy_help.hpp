#pragma once

#include <cstdint>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "circuit_escape/pathfinding/busquedas.hpp"

namespace circuit_escape {

enum class HelpOrder {
    unreachable,
    a_then_b,
    b_then_a,
    only_a,
    only_b,
    exit_only
};

enum class EnergyStatus {
    unreachable,
    enough,
    zero_at_exit,
    insufficient
};

struct EnergyHelp {
    bool a_activated{};
    bool b_activated{};

    std::optional<std::int64_t> cost_a;
    std::optional<std::int64_t> cost_b;
    std::optional<std::int64_t> cost_exit;

    std::optional<std::int64_t> total_cost;
    std::optional<std::int64_t> remaining;

    std::vector<Position> path_a;
    std::vector<Position> path_b;
    std::vector<Position> complete_path;

    HelpOrder order{HelpOrder::unreachable};
    EnergyStatus status{EnergyStatus::unreachable};

    int energy{};
};

struct RoutePlan {
    bool found{};
    std::int64_t cost{};
    HelpOrder order{HelpOrder::unreachable};
    std::vector<Position> path;
};

// Une los tramos de una ruta completa.
inline RoutePlan combine_segments(
    HelpOrder order,
    const std::vector<SearchResult>& segments) {

    RoutePlan plan;
    plan.found = true;
    plan.order = order;

    for (const auto& segment : segments) {
        if (!segment.found || !segment.cost)
            return {};

        plan.cost += *segment.cost;

        std::size_t start =
            plan.path.empty() ? 0 : 1;

        for (std::size_t i = start;
             i < segment.path.size(); ++i) {
            plan.path.push_back(segment.path[i]);
        }
    }

    return plan;
}

// Recalcula desde la posicion actual del jugador.
inline EnergyHelp compute_energy_help(
    const MovementGraph& graph,
    Position player,
    Position a,
    Position b,
    Position exit,
    bool activated_a,
    bool activated_b,
    int current_energy) {

    if (!graph.traversable(player) ||
        !graph.traversable(a) ||
        !graph.traversable(b) ||
        !graph.traversable(exit)) {
        throw std::invalid_argument("Objetivos invalidos");
    }

    EnergyHelp help;
    help.a_activated = activated_a;
    help.b_activated = activated_b;
    help.energy = current_energy;

    auto from_player = dijkstra_from(graph, player);

    // Mostrar costos desde la posicion actual.
    if (!activated_a) {
        auto route = from_player.result_to(a);
        help.cost_a = route.cost;
        help.path_a = route.path;
    }

    if (!activated_b) {
        auto route = from_player.result_to(b);
        help.cost_b = route.cost;
        help.path_b = route.path;
    }

    RoutePlan best;

    if (activated_a && activated_b) {
        auto direct = from_player.result_to(exit);
        help.cost_exit = direct.cost;

        best = combine_segments(
            HelpOrder::exit_only, {direct});
    }
    else if (activated_a) {
        best = combine_segments(HelpOrder::only_b, {
            from_player.result_to(b),
            dijkstra_from(graph, b).result_to(exit)
        });
    }
    else if (activated_b) {
        best = combine_segments(HelpOrder::only_a, {
            from_player.result_to(a),
            dijkstra_from(graph, a).result_to(exit)
        });
    }
    else {
        auto from_a = dijkstra_from(graph, a);
        auto from_b = dijkstra_from(graph, b);

        // Primer orden: jugador -> A -> B -> salida
        auto ab = combine_segments(HelpOrder::a_then_b, {
            from_player.result_to(a),
            from_a.result_to(b),
            from_b.result_to(exit)
        });

        // Segundo orden: jugador -> B -> A -> salida
        auto ba = combine_segments(HelpOrder::b_then_a, {
            from_player.result_to(b),
            from_b.result_to(a),
            from_a.result_to(exit)
        });

        if (ab.found && (!ba.found || ab.cost <= ba.cost))
            best = ab;
        else
            best = ba;
    }

    // Sin recorrido completo.
    if (!best.found)
        return help;

    help.order = best.order;
    help.total_cost = best.cost;
    help.complete_path = best.path;
    help.remaining = std::int64_t(current_energy) - best.cost;

    if (current_energy > best.cost)
        help.status = EnergyStatus::enough;
    else if (current_energy == best.cost)
        help.status = EnergyStatus::zero_at_exit;
    else
        help.status = EnergyStatus::insufficient;

    return help;
}

// Convierte los resultados en texto para la interfaz.
inline std::string format_energy_help(const EnergyHelp& help) {

    auto show_cost = [](auto value) {
        return value
            ? std::to_string(*value)
            : std::string("--");
    };

    std::ostringstream out;

    out << "=== AYUDA H: ENERGIA ===\n\n";

    out << "Interruptor A: "
        << (help.a_activated ? "Activado" : "Pendiente")
        << " | Gasto: " << show_cost(help.cost_a) << '\n';

    out << "Interruptor B: "
        << (help.b_activated ? "Activado" : "Pendiente")
        << " | Gasto: " << show_cost(help.cost_b) << '\n';

    out << "Salida: ";

    if (help.a_activated && help.b_activated)
        out << "Habilitada";
    else if (!help.a_activated && !help.b_activated)
        out << "Requiere A y B";
    else if (!help.a_activated)
        out << "Requiere A";
    else
        out << "Requiere B";

    out << " | Gasto: " << show_cost(help.cost_exit) << '\n';

    out << "\nRecorrido: ";

    switch (help.order) {
        case HelpOrder::a_then_b:
            out << "Jugador -> A -> B -> Salida"; break;
        case HelpOrder::b_then_a:
            out << "Jugador -> B -> A -> Salida"; break;
        case HelpOrder::only_a:
            out << "Jugador -> A -> Salida"; break;
        case HelpOrder::only_b:
            out << "Jugador -> B -> Salida"; break;
        case HelpOrder::exit_only:
            out << "Jugador -> Salida"; break;
        case HelpOrder::unreachable:
            out << "Destino inaccesible"; break;
    }

    out << "\nEnergia disponible: " << help.energy;
    out << "\nGasto total: " << show_cost(help.total_cost);
    out << "\nQuedarian: " << show_cost(help.remaining);

    out << "\n\nEstado: ";

    switch (help.status) {
        case EnergyStatus::enough:
            out << "ALCANZA SIN RECARGAS"; break;
        case EnergyStatus::zero_at_exit:
            out << "ENERGIA CERO AL LLEGAR"; break;
        case EnergyStatus::insufficient:
            out << "NO ALCANZA SIN RECARGAS"; break;
        case EnergyStatus::unreachable:
            out << "DESTINO INACCESIBLE"; break;
    }

    out << "\nNo anticipa al vigilante ni las recargas.\n";

    return out.str();
}

} // namespace circuit_escape
