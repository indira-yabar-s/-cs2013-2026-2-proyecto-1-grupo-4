# Documento de Diseño: Circuito de Escape (C++20)

**Curso:** CS2013 - Programación III (2026-2)  
**Grupo:** 4  

Este documento resume las decisiones de arquitectura, separación de responsabilidades y la ubicación exacta de los temas de C++20 solicitados en el proyecto.

---

## 1. Arquitectura y Separación de Capas

El motor se organiza en 4 componentes desacoplados:
* **Dominio (`Grid`, `Position`, `Cell`):** Almacena y valida el estado de la cuadrícula. No conoce turnos, puntajes ni entradas de consola.
* **Entorno y Reglas (`NavigationEnvironment`, `GameRules`):** Aplica la lógica del juego, descuenta energía, resuelve colisiones y eventos, y determina el fin de partida. No lee ni imprime en consola.
* **Controladores (`IController`, `PolicyController`):** Reciben una observación inmutable (`Observation`) y eligen una acción legal (`Action`).
* **Presentación (`ConsoleUI`, `GameApplication`):** Lee eventos de teclado mediante FTXUI y dibuja la cuadrícula en modo Emoji o ASCII.

---

## 2. Aplicación de Temas Obligatorios de C++20

### 2.1 Templates de Funciones (`include/circuit_escape/grid_algorithms.h`)
* `countMatching`: Cuenta elementos de un rango `[first, last)` según un predicado. Restringido con `std::input_iterator` y `std::predicate`.
* `copyTraversablePositions`: Copia a un `std::output_iterator` las posiciones transitables del tablero. Funciona con `std::vector`, `std::list` o cualquier contenedor STL sin duplicar código.
* `findFirst<CellAlternative>`: Busca la primera posición de una celda específica retornando `std::optional<Position>`.
* `countCellsOf<Ts...>`: Cuenta celdas que contienen cualquiera de los tipos dados mediante template variádico y fold expression.

### 2.2 Templates de Clases (`include/circuit_escape/grid.h`, `graph.h`)
* `Grid<CellType>` (tablero de **tamaño variable**):
  * Parámetro de tipo: `CellType`.
  * Filas y columnas se reciben en el constructor (`Grid<Cell> g(21, 31)`), así el mismo tipo sirve para mapas de cualquier tamaño (cargados de `.txt` o generados).
  * Almacenamiento contiguo en `std::vector<CellType>` en orden por filas (`índice = fila * columnas + columna`).
  * Validación en ejecución: dimensiones 0 lanzan `std::invalid_argument`.
  * Métodos `at()` y `operator()` con validación de límites (`std::out_of_range`).
* `Graph<Weight>`: grafo no dirigido ponderado (lista de aristas + lista de adyacencia). `Weight` está restringido por el concept `EdgeWeight` (`std::totally_ordered` y `std::copyable`).

### 2.2.1 Generación de laberintos: Grafo + Disjoint Set + Kruskal
* `DisjointSet` (`disjoint_set.h/.cpp`): Union-Find con unión por rango y compresión de caminos (`find`/`unite` en O(α(n))).
* `kruskal(graph)` (`kruskal.h`): ordena las aristas por peso y acepta una arista solo si `DisjointSet::unite` devuelve `true` (no forma ciclo). O(E log E).
* `generateMaze(rows, columns, seed, options)` (`maze_generator.h/.cpp`):
  1. Las celdas impares `(2i+1, 2j+1)` son salas (vértices); todo lo demás es muro.
  2. Cada par de salas vecinas es una arista con peso aleatorio (`std::mt19937` con semilla).
  3. Kruskal elige el árbol de expansión mínima; se abre el muro entre las salas de cada arista del árbol → laberinto perfecto (conexo y sin ciclos).
  4. `MazeOptions::extraPassageChance` abre algunos muros extra para crear rutas alternativas, y se colocan recursos, baterías, trampas y terreno elevado.

### 2.2.2 Mapas en archivos `.txt` (`scenario.h/.cpp`)
* `loadScenarioFile(path)` / `scenarioFromStream(istream)`: el tamaño se deduce del archivo (filas = líneas, columnas = largo de la primera línea; todas deben medir igual). Se ignoran líneas vacías, comentarios `;` y `\r` de Windows.
* `saveScenarioFile(path, scenario)` / `writeScenario(ostream, scenario)`: escriben el mismo formato ASCII (con `@` en el inicio), así un mapa guardado se vuelve a cargar idéntico.

### 2.3 Especialización Total y Parcial (`include/circuit_escape/cells.h`)
Definición del rasgo `CellTraits<CellType>`:
* **Plantilla general:** `traversable = true`, `consumable = false`.
* **Especialización total (`Wall`):** `traversable = false`.
* **Especialización total (`Battery`):** `consumable = true`.
* **Especialización parcial (`ResourceCell<Reward>`):** `consumable = true`, válida para cualquier tipo de recompensa (`Reward`).

### 2.4 Templates Variádicos y Fold Expressions
* **Functor `Overloaded` (`include/circuit_escape/overloaded.h`):** Hereda de un pack de lambdas `operator()...` para despachar variantes con `std::visit`.
* **Fold expression (`include/circuit_escape/cells.h`):**
  ```cpp
  template <CellAlternative... Ts>
  constexpr bool holdsAnyOf(const Cell& cell) noexcept {
      return (std::holds_alternative<Ts>(cell) || ...);
  }
  ```

### 2.5 Concepts y Polimorfismo Dinámico (`include/circuit_escape/controllers.h`)
* **Concept estático `NavigationPolicy`:** Verifica en compilación que la política implemente `selectAction(Observation, span<Action>) -> Action`.
* **Interfaz virtual `IController`:** Permite cambiar el controlador dinámicamente en tiempo de ejecución.
* **Adaptador genérico `PolicyController<Policy>`:** Adapta cualquier política que cumpla el concept a la interfaz `IController` sin usar `dynamic_cast` ni `typeid`.

---

## 3. Uso Justificado de la Biblioteca Estándar

* `std::vector`: Tablero contiguo de tamaño variable (`Grid<CellType>`), también usado en el grafo y el Disjoint Set.
* `<filesystem>` y `<fstream>`: guardado y carga de mapas `.txt`.
* `std::vector`: Registro dinámico de eventos en cada turno y lista de acciones legales.
* `std::variant` y `std::visit`: Modela celdas (`Cell`) y eventos (`NavigationEvent`) de forma segura sin punteros crudos.
* `std::optional`: Indica explícitamente búsquedas o conversiones que pueden no tener valor (ej. `neighbor`, `findFirst`, `translate`).
* `std::unique_ptr`: Gestión automática de memoria RAII para controladores polimórficos (`IController`).
* `std::span`: Vistas de solo lectura para evitar copias innecesarias de vectores.
* `<random>`: `std::mt19937` con semilla fija para reproducibilidad exacta de simulaciones.

---

## 4. Prueba Negativa de Compilación Documentada

En `tests/negative_compilation_test.cpp.example` se documenta el fallo cuando una clase no cumple `NavigationPolicy`:

```cpp
struct InvalidPolicy {
    int selectAction(const Observation&, std::span<const Action>) { return 0; } // Error: debe retornar Action
};

// Al compilar:
PolicyController<InvalidPolicy> ctrl{InvalidPolicy{}};
```

**Diagnóstico generado por el compilador:**
```text
error: template constraint failure for 'template<NavigationPolicy Policy> class PolicyController'
note: the required expression '{policy.selectAction(observation, actions)} -> same_as<Action>' is not satisfied
```
