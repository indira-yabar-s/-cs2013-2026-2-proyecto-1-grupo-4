//
// Created by Oriana Manrique Romero on 23/09/26.
//

#ifndef PROYECTO1PG3_GRID_ALGORITHMS_H
#define PROYECTO1PG3_GRID_ALGORITHMS_H


#pragma once
// Templates de función propios sobre el tablero (enunciado §6.1).
//
// Todos reciben rangos mediante iteradores o trabajan sobre cualquier Grid,
// por lo que el mismo algoritmo sirve para std::vector, std::list, std::deque
// o el std::array interno del tablero, sin duplicar código (§9).

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <optional>
#include <type_traits>
#include <variant>

#include "circuit_escape/cells.h"
#include "circuit_escape/grid.h"
#include "circuit_escape/position.h"

namespace circuit_escape {

// 1) Cuenta los elementos de [first, last) que cumplen un predicado.
//    Rango por iteradores; funciona con celdas, posiciones o cualquier tipo.
template <std::input_iterator It, std::sentinel_for<It> Sentinel, typename Predicate>
    requires std::predicate<Predicate&, std::iter_reference_t<It>>
[[nodiscard]] std::size_t countMatching(It first, Sentinel last, Predicate predicate) {
    std::size_t count = 0;
    for (; first != last; ++first) {
        if (predicate(*first)) ++count;
    }
    return count;
}

// 2) Copia hacia `out` las posiciones de [first, last) cuya celda en el
//    tablero es transitable. Rango por iteradores + iterador de salida:
//    sirve con std::vector<Position>, std::list<Position>, std::back_inserter...
template <typename CellType, std::size_t Rows, std::size_t Columns,
          std::input_iterator It, std::sentinel_for<It> Sentinel,
          std::output_iterator<Position> Out>
    requires std::same_as<std::iter_value_t<It>, Position>
Out copyTraversablePositions(const Grid<CellType, Rows, Columns>& grid,
                             It first, Sentinel last, Out out) {
    for (; first != last; ++first) {
        const Position p = *first;
        if (grid.contains(p) && isTraversable(grid.at(p))) {
            *out++ = p;
        }
    }
    return out;
}

// 3) Primera posición (en orden por filas) cuya celda contiene la alternativa
//    CellT. std::optional expresa que la búsqueda puede fallar.
template <CellAlternative CellT, std::size_t Rows, std::size_t Columns>
[[nodiscard]] std::optional<Position> findFirst(const Grid<Cell, Rows, Columns>& grid) {
    const auto it = std::find_if(grid.begin(), grid.end(), [](const Cell& cell) {
        return std::holds_alternative<CellT>(cell);
    });
    if (it == grid.end()) return std::nullopt;
    return grid.positionOf(it);
}

// 4) Cuenta las celdas del tablero que contienen alguno de los tipos Ts...
//    (paquete variádico reenviado a holdsAnyOf, que usa una fold expression).
template <CellAlternative... Ts, std::size_t Rows, std::size_t Columns>
[[nodiscard]] std::size_t countCellsOf(const Grid<Cell, Rows, Columns>& grid) {
    return countMatching(grid.begin(), grid.end(),
                         [](const Cell& cell) { return holdsAnyOf<Ts...>(cell); });
}

}  // namespace circuit_escape


#endif //PROYECTO1PG3_GRID_ALGORITHMS_H