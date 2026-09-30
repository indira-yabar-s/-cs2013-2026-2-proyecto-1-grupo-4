# Contribuciones del Grupo 4

**Proyecto 1 - Circuito de Escape**  
**Curso:** CS2013 - Programación III (2026-2)  

---

## 1. División de Responsabilidades

| Integrante | Área | Responsabilidades Principales |
| :--- | :--- | :--- |
| **Jean Franco** | **Integración y Pruebas** | CMake, CTest, 4 suites de prueba automáticas, simulación reproducible y escenarios $20 \times 30$. |
| **Oriana Manrique** | **Tablero y Dominio** | `Position`, `Grid` (NTTP), tipos de `Cell`, `CellTraits` y algoritmos genéricos. |
| **Indira Yabar** | **Reglas y Entorno** | `NavigationEnvironment`, `GameRules`, perfiles de dificultad y eventos de juego. |
| **Rodolfo Huaroc** | **Controladores** | `IController`, concept `NavigationPolicy`, `PolicyController`, `RandomPolicy` y `HeuristicPolicy`. |
| **Pedro Chavez** | **Interfaz de Consola** | Integración de FTXUI, `ConsoleUI` (modo Emoji / ASCII) y bucle de juego interactivo. |

---

## 2. Resumen de Aportes

### Jean Franco (Integración y Pruebas)
* Modularizó `CMakeLists.txt` en bibliotecas estáticas (`circuit_escape_core`, `circuit_escape_ui`) y habilitó CTest.
* Implementó las 4 suites de pruebas con `assert`: `grid_test.cpp`, `environment_test.cpp`, `interactions_test.cpp` y `controllers_test.cpp`.
* Diseñó los mapas `scenario_01.txt` y `scenario_02.txt` y el modo headless `--simulate`.

### Oriana Manrique (Tablero y Dominio)
* Desarrolló `grid.h` con dimensiones conocidas en compilación sobre `std::array`.
* Definió los structs de celdas y la especialización total y parcial en `CellTraits`.
* Creó los templates de función genéricos en `grid_algorithms.h`.

### Indira Yabar (Reglas y Entorno)
* Creó `environment.hpp` y `game_rules.h` con los costos de movimiento, trampas y baterías.
* Implementó las reglas de precedencia de término (`goalReached` > `noEnergy` > `turnLimit`).

### Rodolfo Huaroc (Controladores)
* Diseñó el concept `NavigationPolicy` y el adaptador genérico `PolicyController`.
* Implementó la política aleatoria con semilla controlable y la política heurística por distancia Manhattan.

### Pedro Chavez (Interfaz de Consola)
* Desarrolló `console_ui.cpp` con renderizado de grilla y alineación a 2 columnas por celda.
* Implementó la traducción de eventos de teclado (WASD, flechas, E, H, Q) y el ejecutable interactivo.
