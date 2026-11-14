#include "circuit_escape/disjoint_set.h"

#include <numeric>
#include <stdexcept>
#include <string>
#include <utility>

namespace circuit_escape {

DisjointSet::DisjointSet(std::size_t n) : parent_(n), rank_(n, 0), sets_(n) {
    std::iota(parent_.begin(), parent_.end(), std::size_t{0});
}

std::size_t DisjointSet::find(std::size_t x) {
    if (x >= parent_.size()) {
        throw std::out_of_range("DisjointSet::find: elemento " + std::to_string(x) +
                                " fuera de rango");
    }
    // 1) subir hasta la raíz
    std::size_t root = x;
    while (parent_[root] != root) root = parent_[root];
    // 2) compresión de caminos: todos apuntan directo a la raíz
    while (parent_[x] != root) {
        const std::size_t next = parent_[x];
        parent_[x] = root;
        x = next;
    }
    return root;
}

bool DisjointSet::unite(std::size_t a, std::size_t b) {
    std::size_t rootA = find(a);
    std::size_t rootB = find(b);
    if (rootA == rootB) return false;

    // Unión por rango: el árbol más bajo cuelga del más alto.
    if (rank_[rootA] < rank_[rootB]) std::swap(rootA, rootB);
    parent_[rootB] = rootA;
    if (rank_[rootA] == rank_[rootB]) ++rank_[rootA];
    --sets_;
    return true;
}

bool DisjointSet::connected(std::size_t a, std::size_t b) {
    return find(a) == find(b);
}

}  // namespace circuit_escape
