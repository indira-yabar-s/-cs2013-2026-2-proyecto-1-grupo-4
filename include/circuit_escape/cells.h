//
// Created by Oriana Manrique Romero on 23/09/26.
//

#ifndef PROYECTO1PG3_CELLS_H
#define PROYECTO1PG3_CELLS_H


#pragma once
// Tipclass cells {
//};os de celda del tablero (enunciado §5.5 y §5.7) y rasgos asociados (§6.3).
//
// Las celdas NO imprimen ni leen de consola: solo describen datos. Los costos
// de energía y las recompensas efectivas los decide GameRules en el entorno;
// los valores por defecto de estos structs son los del perfil "standard".

#include <optional>
#include <string_view>
#include <type_traits>
#include <variant>

#include "circuit_escape/overloaded.h"
namespace circuit_escape {

// ---------------------------------------------------------------------------
// Tipos pequeños de celda
// ---------------------------------------------------------------------------
// operator== por defecto: permite comparar celdas y tableros en las pruebas.
struct Empty {
    friend bool operator==(const Empty&, const Empty&) = default;
};

struct Wall {
    friend bool operator==(const Wall&, const Wall&) = default;
};

struct RoughTerrain {
    int energyCost{2};
    friend bool operator==(const RoughTerrain&, const RoughTerrain&) = default;
};

template <typename Reward>
struct ResourceCell {
    Reward reward{};
    bool collected{false};
    friend bool operator==(const ResourceCell&, const ResourceCell&) = default;
};

struct Battery {
    int energy{3};
    bool consumed{false};
    friend bool operator==(const Battery&, const Battery&) = default;
};

struct Trap {
    int energyPenalty{2};
    int scorePenalty{1};
    friend bool operator==(const Trap&, const Trap&) = default;
};

struct Exit {
    friend bool operator==(const Exit&, const Exit&) = default;
};

using Cell = std::variant<Empty, Wall, RoughTerrain, ResourceCell<int>, Battery, Trap, Exit>;

// ---------------------------------------------------------------------------
// CellTraits: propiedades de cada tipo de celda conocidas en compilación.
//   - traversable: el agente puede ocupar la celda.
//   - consumable : la celda se activa solo la primera vez (recurso, batería).
// ---------------------------------------------------------------------------

// Plantilla primaria: caso general (Empty, RoughTerrain, Trap, Exit).
template <typename CellType>
struct CellTraits {
    static constexpr bool traversable = true;
    static constexpr bool consumable  = false;
};

// Especialización TOTAL: el muro es el único caso que no puede atravesarse.
template <>
struct CellTraits<Wall> {
    static constexpr bool traversable = false;
    static constexpr bool consumable  = false;
};

// Especialización TOTAL: la batería se consume tras la primera recarga.
template <>
struct CellTraits<Battery> {
    static constexpr bool traversable = true;
    static constexpr bool consumable  = true;
};

// Especialización PARCIAL: reconoce toda la familia ResourceCell<Reward>,
// sin importar cómo se represente la recompensa (int, double, struct...).
template <typename Reward>
struct CellTraits<ResourceCell<Reward>> {
    static constexpr bool traversable = true;
    static constexpr bool consumable  = true;
};

// Atajos estilo _v de <type_traits> (Semana 4: type traits).
template <typename CellType>
inline constexpr bool is_traversable_v = CellTraits<std::remove_cvref_t<CellType>>::traversable;

template <typename CellType>
inline constexpr bool is_consumable_v = CellTraits<std::remove_cvref_t<CellType>>::consumable;

static_assert(!is_traversable_v<Wall>);
static_assert(is_consumable_v<ResourceCell<double>>, "la especialización parcial cubre cualquier Reward");

// ---------------------------------------------------------------------------
// Concept: T es una de las alternativas de un std::variant.
// ---------------------------------------------------------------------------
template <typename T, typename Variant>
struct is_variant_alternative : std::false_type {};

template <typename T, typename... Alternatives>
struct is_variant_alternative<T, std::variant<Alternatives...>>
    : std::bool_constant<(std::is_same_v<T, Alternatives> || ...)> {};   // fold expression

template <typename T>
concept CellAlternative = is_variant_alternative<T, Cell>::value;

// holdsAnyOf<Ts...>(cell): true si la celda contiene alguno de los tipos.
// Paquete variádico + fold expression con ||.
template <CellAlternative... Ts>
[[nodiscard]] constexpr bool holdsAnyOf(const Cell& cell) noexcept {
    return (std::holds_alternative<Ts>(cell) || ...);
}

// ---------------------------------------------------------------------------
// Consultas en tiempo de ejecución que usan los rasgos anteriores.
// ---------------------------------------------------------------------------
[[nodiscard]] inline bool isTraversable(const Cell& cell) {
    return std::visit([](const auto& c) { return is_traversable_v<decltype(c)>; }, cell);
}

[[nodiscard]] inline bool isConsumable(const Cell& cell) {
    return std::visit([](const auto& c) { return is_consumable_v<decltype(c)>; }, cell);
}

// true si la celda es consumible y ya fue recogida/consumida.
// if constexpr + requires elige el campo correcto en compilación.
[[nodiscard]] inline bool isSpent(const Cell& cell) {
    return std::visit([](const auto& c) -> bool {
        if constexpr (requires { c.collected; }) {
            return c.collected;
        } else if constexpr (requires { c.consumed; }) {
            return c.consumed;
        } else {
            return false;
        }
    }, cell);
}

// Marca una celda consumible como usada. Retorna true si cambió su estado;
// false si no era consumible o ya estaba gastada. Así un recurso no puede
// recogerse dos veces.
inline bool markSpent(Cell& cell) {
    return std::visit([](auto& c) -> bool {
        if constexpr (requires { c.collected; }) {
            if (c.collected) return false;
            c.collected = true;
            return true;
        } else if constexpr (requires { c.consumed; }) {
            if (c.consumed) return false;
            c.consumed = true;
            return true;
        } else {
            return false;
        }
    }, cell);
}

// ---------------------------------------------------------------------------
// Representación textual (convención visual del enunciado §5.8).
// Son datos puros: la capa FTXUI los usará sin que el motor dependa de ella.
// Un recurso o batería gastados se muestran como espacio libre.
// ---------------------------------------------------------------------------
inline constexpr char kAgentAscii = '@';
inline constexpr std::string_view kAgentEmoji = "🤖";

[[nodiscard]] inline char toAscii(const Cell& cell) {
    return std::visit(Overloaded{
        [](const Empty&)                  { return '.'; },
        [](const Wall&)                   { return '#'; },
        [](const RoughTerrain&)           { return '~'; },
        [](const ResourceCell<int>& r)    { return r.collected ? '.' : 'R'; },
        [](const Battery& b)              { return b.consumed ? '.' : 'B'; },
        [](const Trap&)                   { return 'T'; },
        [](const Exit&)                   { return 'S'; },
    }, cell);
}

[[nodiscard]] inline std::string_view toEmoji(const Cell& cell) {
    return std::visit(Overloaded{
        [](const Empty&)               -> std::string_view { return "⬜"; },
        [](const Wall&)                -> std::string_view { return "⬛"; },
        [](const RoughTerrain&)        -> std::string_view { return "🟫"; },
        [](const ResourceCell<int>& r) -> std::string_view { return r.collected ? "⬜" : "💎"; },
        [](const Battery& b)           -> std::string_view { return b.consumed ? "⬜" : "⚡"; },
        [](const Trap&)                -> std::string_view { return "💥"; },
        [](const Exit&)                -> std::string_view { return "🏁"; },
    }, cell);
}

// Conversión inversa para cargar mapas. '@' no es una celda: el cargador de
// escenarios lo interpreta como posición inicial sobre un Empty.
[[nodiscard]] inline std::optional<Cell> cellFromAscii(char symbol) {
    switch (symbol) {
        case '.': return Cell{Empty{}};
        case '#': return Cell{Wall{}};
        case '~': return Cell{RoughTerrain{}};
        case 'R': return Cell{ResourceCell<int>{10}};
        case 'B': return Cell{Battery{}};
        case 'T': return Cell{Trap{}};
        case 'S': return Cell{Exit{}};
        default:  return std::nullopt;
    }
}

}  // namespace circuit_escape


#endif //PROYECTO1PG3_CELLS_H