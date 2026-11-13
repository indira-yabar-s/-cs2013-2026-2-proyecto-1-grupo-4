//
// Created by Oriana Manrique Romero on 23/09/26.
//

#ifndef PROYECTO1PG3_GRID_H
#define PROYECTO1PG3_GRID_H


#pragma once
// Tablero genérico de tamaño variable (definido en tiempo de ejecución).
//
//   CellType : parámetro de tipo (la celda)
//   rows / columns : se reciben en el constructor, así el mismo tipo
//                    Grid<Cell> sirve para mapas de 20x30, 7x9, 41x61...
//
// Almacenamiento contiguo en std::vector en orden por filas:
//   índice = fila * columns + columna
// Por eso los iteradores recorren de izquierda a derecha y de arriba hacia abajo.
// Grid no conoce turnos, energía, controladores ni reglas de victoria.

#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "circuit_escape/position.h"

namespace circuit_escape {

template <typename CellType>
class Grid {
    using storage_type = std::vector<CellType>;

public:
    using value_type      = CellType;
    using reference       = typename storage_type::reference;
    using const_reference = typename storage_type::const_reference;
    using size_type       = std::size_t;
    using iterator        = typename storage_type::iterator;
    using const_iterator  = typename storage_type::const_iterator;

    // ---- construcción -----------------------------------------------------
    // Crea un tablero rows x columns relleno con `fill` (por defecto CellType{}).
    // Lanza std::invalid_argument si alguna dimensión es 0.
    Grid(size_type rows, size_type columns, const CellType& fill = CellType{})
        : rows_(validated(rows, "filas")),
          columns_(validated(columns, "columnas")),
          cells_(rows_ * columns_, fill) {}

    // Copia, movimiento y destrucción por defecto: std::vector ya administra
    // sus elementos (regla del cero / RAII).

    // ---- dimensiones (conocidas en ejecución) -----------------------------
    [[nodiscard]] size_type rows() const noexcept { return rows_; }
    [[nodiscard]] size_type columns() const noexcept { return columns_; }
    [[nodiscard]] size_type size() const noexcept { return cells_.size(); }

    // ---- consulta de límites ---------------------------------------------
    [[nodiscard]] bool contains(Position position) const noexcept {
        return position.row < rows_ && position.column < columns_;
    }

    // ---- acceso validado (lanza std::out_of_range) ------------------------
    reference at(Position position) { return cells_[checkedIndex(position)]; }
    const_reference at(Position position) const { return cells_[checkedIndex(position)]; }

    // operator(): acceso equivalente por (fila, columna), también validado.
    reference operator()(size_type row, size_type column) { return at({row, column}); }
    const_reference operator()(size_type row, size_type column) const { return at({row, column}); }

    // ---- conversión índice <-> posición (útil con algoritmos) -------------
    [[nodiscard]] Position positionAt(size_type index) const noexcept {
        return Position{index / columns_, index % columns_};
    }

    [[nodiscard]] size_type indexOf(Position position) const noexcept {
        return position.row * columns_ + position.column;
    }

    // Posición de la celda apuntada por un iterador de este tablero.
    [[nodiscard]] Position positionOf(const_iterator it) const noexcept {
        return positionAt(static_cast<size_type>(it - cells_.cbegin()));
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
    static size_type validated(size_type value, const char* what) {
        if (value == 0) {
            throw std::invalid_argument(std::string("Grid: el número de ") + what +
                                        " debe ser mayor que 0");
        }
        return value;
    }

    size_type checkedIndex(Position position) const {
        if (!contains(position)) {
            throw std::out_of_range("Grid::at: posición " + toString(position) +
                                    " fuera de un tablero de " + std::to_string(rows_) +
                                    " x " + std::to_string(columns_));
        }
        return indexOf(position);
    }

    size_type rows_;
    size_type columns_;
    storage_type cells_;
};

}  // namespace circuit_escape

#endif //PROYECTO1PG3_GRID_H
