//
// Created by Oriana Manrique Romero on 23/09/26.
//

#ifndef PROYECTO1PG3_GRID_H
#define PROYECTO1PG3_GRID_H


#pragma once
// Tablero genérico de tamaño fijo.
//
//   CellType : parámetro de tipo  (la celda)
//   Rows     : parámetro no-tipo  (filas)
//   Columns  : parámetro no-tipo  (columnas)
//
// Almacenamiento contiguo en std::array en orden por filas:
//   índice = fila * Columns + columna
// Por eso los iteradores recorren de izquierda a derecha y de arriba hacia abajo.
// Grid no conoce turnos, energía, controladores ni reglas de victoria.

#include <array>
#include <cstddef>
#include <stdexcept>
#include <string>

#include "circuit_escape/position.h"

namespace circuit_escape {

template <typename CellType, std::size_t Rows, std::size_t Columns>
    requires (Rows > 0 && Columns > 0)
class Grid {
    static_assert(Rows > 0 && Columns > 0, "Grid requiere al menos una fila y una columna");

    using storage_type = std::array<CellType, Rows * Columns>;

public:
    using value_type      = CellType;
    using reference       = CellType&;
    using const_reference = const CellType&;
    using size_type       = std::size_t;
    using iterator        = typename storage_type::iterator;
    using const_iterator  = typename storage_type::const_iterator;

    // ---- construcción -----------------------------------------------------
    constexpr Grid() = default;

    // Rellena todo el tablero con una misma celda.
    constexpr explicit Grid(const CellType& fill) { cells_.fill(fill); }

    // Copia, movimiento y destrucción por defecto: std::array ya administra
    // sus elementos (regla del cero / RAII).

    // ---- dimensiones (conocidas en compilación) ---------------------------
    [[nodiscard]] static constexpr size_type rows() noexcept { return Rows; }
    [[nodiscard]] static constexpr size_type columns() noexcept { return Columns; }
    [[nodiscard]] static constexpr size_type size() noexcept { return Rows * Columns; }

    // ---- consulta de límites ---------------------------------------------
    [[nodiscard]] constexpr bool contains(Position position) const noexcept {
        return position.row < Rows && position.column < Columns;
    }

    // ---- acceso validado (lanza std::out_of_range) ------------------------
    CellType& at(Position position) { return cells_[checkedIndex(position)]; }
    const CellType& at(Position position) const { return cells_[checkedIndex(position)]; }

    // operator(): acceso equivalente por (fila, columna), también validado.
    CellType& operator()(size_type row, size_type column) { return at({row, column}); }
    const CellType& operator()(size_type row, size_type column) const { return at({row, column}); }

    // ---- conversión índice <-> posición (útil con algoritmos) -------------
    [[nodiscard]] static constexpr Position positionOf(size_type index) noexcept {
        return Position{index / Columns, index % Columns};
    }

    [[nodiscard]] static constexpr size_type indexOf(Position position) noexcept {
        return position.row * Columns + position.column;
    }

    // Posición de la celda apuntada por un iterador de este tablero.
    [[nodiscard]] Position positionOf(const_iterator it) const noexcept {
        return positionOf(static_cast<size_type>(it - cells_.cbegin()));
    }

    // ---- iteradores (orden por filas) -------------------------------------
    iterator begin() noexcept { return cells_.begin(); }
    iterator end() noexcept { return cells_.end(); }
    const_iterator begin() const noexcept { return cells_.begin(); }
    const_iterator end() const noexcept { return cells_.end(); }
    const_iterator cbegin() const noexcept { return cells_.cbegin(); }
    const_iterator cend() const noexcept { return cells_.cend(); }

    friend bool operator==(const Grid&, const Grid&) = default;

private:
    size_type checkedIndex(Position position) const {
        if (!contains(position)) {
            throw std::out_of_range("Grid::at: posición " + toString(position) +
                                    " fuera de un tablero de " + std::to_string(Rows) +
                                    " x " + std::to_string(Columns));
        }
        return indexOf(position);
    }

    storage_type cells_{};
};

}  // namespace circuit_escape

#endif //PROYECTO1PG3_GRID_H