# Circuito de Escape (Proyecto 2)

CS2013 - Programación III (2026-2) · Grupo 4

Es un juego en consola hecho en C++20. Manejas un robot por un mapa con energía limitada, tienes que activar dos interruptores
(A y B) y llegar a la salida sin que te agarre un vigilante que te persigue. Tiene tres niveles (fácil, medio y difícil) y los mapas
los genera el mismo programa, que además comprueba que se puedan ganar.

| Integrante | Código UTEC | Usuario GitHub | Qué hizo en el Proyecto 2 |
| :--- | :--- | :--- | :--- |
| Oriana Manrique | 202320149 | orianamanriqueromero | Grid variable, grafo, DisjointSet, Kruskal, generador de laberintos y mapas |
| Rodolfo Huaroc | 202220307 | rodolfoextremo69 | Dijkstra, BFS, vigilante y ayuda H |
| Pedro Chavez | 202510334 | pedrochavezoscanoa | Interruptores A y B, captura, interfaz FTXUI, carga de niveles e integración del juego |
| Indira Yabar | 202520029 | indira-yabar-s | Concurrencia, validación de mapas, selección de niveles y generador de mapas |

Contenido: 1. Qué hace falta · 2. Compilar · 3. Probar · 4. Jugar · 5. Generador de mapas · 6. Si algo falla ·
7. Instalador de Windows · 8. Carpetas · 9. Decisiones de diseño · 10. Dónde está cada tema · 11. Pruebas ·
12. Aportes · 13. Limitaciones · 14. Uso de IA · 15. Referencias · 16. Detalles técnicos

---

## 1. Qué hace falta

Un compilador que soporte C++20, CMake 3.20 o más nuevo, e internet la primera vez, porque CMake baja la librería de la interfaz
(FTXUI v7.0.3) desde GitHub. Todo lo demás ya está en el repositorio.

**Windows con CLion (lo más fácil):** CLion ya trae CMake y el compilador MinGW, no hay que instalar nada más.

**Windows sin CLion:** instalar [CMake](https://cmake.org/download/) y [MinGW-w64](https://www.mingw-w64.org/) (versión 11 o más nueva,
con `g++` y `mingw32-make` en el PATH).

**Linux (Ubuntu o Debian):**

```bash
sudo apt install build-essential cmake git
```

**macOS:**

```bash
xcode-select --install
brew install cmake
```

Para ver si quedó bien instalado:

```text
cmake --version     (tiene que decir 3.20 o más)
g++ --version       (tiene que decir 11 o más)
```

---

## 2. Compilar

Todos los comandos se corren dentro de la carpeta del proyecto, la que tiene el archivo `CMakeLists.txt`.

**Windows (PowerShell) con MinGW:**

```powershell
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

**Linux o macOS:**

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

La primera vez demora unos minutos porque baja y compila FTXUI. Cuando termina, en la carpeta `build` quedan:

* `escape_circuit`: el juego.
* `map_generator`: genera y guarda los mapas de los tres niveles.
* Los 14 programas de prueba.

**Con CLion:** abrir la carpeta del proyecto, elegir el perfil *Release* en *Settings > Build > CMake* y darle a *Build*.
Para correr el juego desde CLion hay que editar la configuración `escape_circuit`: en *Program arguments* poner `--level easy` y en
*Working directory* poner la carpeta del proyecto (no `cmake-build-release`). Si no, el juego no encuentra los mapas.

---

## 3. Probar que todo funciona

```text
ctest --test-dir build --output-on-failure
```

Si todo está bien, al final sale esto:

```text
100% tests passed, 0 tests failed out of 14
```

También se puede probar el generador de mapas. Con estos comandos imprime una tabla con los 10 mapas y elige tres niveles
(a nosotros nos salió fácil = mapa 4 con costo 39, medio = mapa 9 con 47 y difícil = mapa 10 con 57):

```text
Windows:       .\build\map_generator.exe --maps 10 --workers 4 --seed 2026 --rows 15 --cols 21 --save assets/maps
Linux/macOS:   ./build/map_generator --maps 10 --workers 4 --seed 2026 --rows 15 --cols 21 --save assets/maps
```

Con la misma semilla siempre salen los mismos mapas.

---

## 4. Jugar

Desde la carpeta del proyecto. Los tres niveles ya vienen guardados en `assets/maps`, así que se puede jugar sin generar nada:

```text
Windows:       .\build\escape_circuit.exe --level easy
Linux/macOS:   ./build/escape_circuit --level easy
```

El nivel puede ser `easy`, `medium` o `hard`. Si en la consola los emojis se ven mal, se agrega `--ascii`.

**Controles:** WASD o flechas para moverse, E para esperar, H para la ayuda y Q para salir.
Dentro de la ayuda: 1 muestra la ruta a A, 2 la ruta a B, 3 el recorrido completo y H o Esc vuelve al juego. Mientras estás en la
ayuda no pasa ningún turno.

**Qué se ve en el tablero:** el robot, el vigilante, los interruptores A y B, la salida, recursos que dan puntos, baterías y trampas.
Con `--ascii` son `@` (robot), `V` (vigilante), `A`, `B`, `R` (recurso), `T` (trampa) y `S` (salida). Arriba salen la energía, el
turno y si A y B siguen pendientes o ya están activados.

**Reglas:**

* Cada paso gasta energía según la celda a la que entras. Esperar o moverte contra una pared cuesta un turno y una unidad de energía.
* Hay que activar A y B (en el orden que quieras) y llegar a la salida con energía mayor que cero y sin pasarte del límite de turnos.
* El vigilante se mueve con BFS, pero solo en los turnos pares. Si te alcanza pierdes, incluso si estabas en la salida.

Otras formas de jugar:

```text
escape_circuit --map assets/maps/mapa_medio.txt           (un mapa que ya está guardado)
escape_circuit --generate 20x30 --seed 7 --level hard     (genera un mapa de otro tamaño y lo juega)
escape_circuit --classic                                  (el juego del Proyecto 1, sin interruptores ni vigilante)
```

---

## 5. Generador de mapas

`map_generator` hace varios mapas a la vez usando varios hilos, revisa que cada uno se pueda ganar y guarda tres.

| Opción | Qué hace |
| :--- | :--- |
| `--maps N` | Cuántos mapas genera (de 0 a 10) |
| `--workers W` | Cuántos hilos trabajan en paralelo (1 o más) |
| `--seed S` | La semilla. El mapa número `i` usa la semilla `S + i` |
| `--rows F --cols C` | Tamaño (mínimo 7x7, máximo 200 por lado y 20 000 casillas) |
| `--save CARPETA` | Guarda `mapa_facil.txt`, `mapa_medio.txt` y `mapa_dificil.txt` |
| `--compare` | Corre con 1 hilo y con W hilos, compara que den lo mismo y mide cuánto demora cada fase |
| `--help` | Muestra la ayuda |

La energía inicial y el límite de turnos crecen con el tamaño del mapa: energía = `max(60, 3·(filas+columnas))` y turnos = `3·energía`.
Un mapa solo sirve como nivel si existe una partida ganadora comprobada. Si el lote no tiene tres costos distintos con victoria, el
programa avisa y termina con código 2. En ese caso hay que probar con otra semilla.

---

## 6. Si algo falla

| Problema | Qué hacer |
| :--- | :--- |
| `cmake` no se reconoce como comando | No está instalado o no está en el PATH. Instalarlo y abrir una terminal nueva. |
| CMake no puede descargar FTXUI | Revisar el internet. Si hay firewall o proxy, dejar pasar `github.com`. |
| El generador `MinGW Makefiles` no funciona | Falta `mingw32-make` en el PATH. Usar la terminal de CLion o instalar MinGW-w64. |
| Sale "No se encontro mapa_facil.txt" | El juego se abrió fuera de la carpeta del proyecto. Entrar a esa carpeta y repetir, o generar los mapas con `map_generator`. |
| Los emojis se ven como cuadros | Agregar `--ascii`. En Windows conviene usar Windows Terminal. |
| `ctest` dice que no encontró pruebas | Falta compilar primero con `cmake --build build`. |
| Quedó algo raro al compilar | Borrar la carpeta `build` y repetir la sección 2. |

No probamos con Visual Studio (MSVC). En Windows usamos MinGW y en Linux g++.

---

## 7. Instalador de Windows

El instalador se hace con [Inno Setup](https://jrsoftware.org/isinfo.php) usando `installer/setup.iss`. Primero hay que compilar
(sección 2) y generar los mapas (sección 3), y después:

```powershell
ISCC installer\setup.iss
```

Eso crea `dist\CircuitoDeEscape_Instalador.exe`. El instalador copia `escape_circuit.exe`, `map_generator.exe`, los tres mapas y este
README a `%LOCALAPPDATA%\Programs\CircuitoDeEscape`, y deja accesos en el menú Inicio para jugar fácil, medio o difícil, abrir el
generador y desinstalar. Los ejecutables van enlazados de forma estática (`-static`), así que en el equipo donde se instala no hace
falta compilador ni DLL.

* **Ejecutar:** desde el menú Inicio, o con `escape_circuit.exe --level medium` dentro de la carpeta instalada.
* **Desinstalar:** *Configuración > Aplicaciones > Circuito de Escape > Desinstalar*, o el acceso Desinstalar del menú Inicio.

Para instalar sin Inno Setup (por ejemplo en Linux): `cmake --install build --prefix instalado` copia los ejecutables y los mapas.

---

## 8. Carpetas del proyecto

| Carpeta o archivo | Qué tiene |
| :--- | :--- |
| `app/` | Los dos programas: `main.cpp` (el juego) y `map_generator_main.cpp` (el generador) |
| `include/circuit_escape/` | Los .h/.hpp: tablero, celdas, entorno, controladores, grafos, Kruskal, Dijkstra y BFS (`pathfinding/`) y mapas (`map_generation/`) |
| `src/` | Los .cpp |
| `tests/` | Las 14 pruebas |
| `assets/maps/` | Los tres niveles ya guardados |
| `installer/` | El script del instalador de Windows |
| `docs/` | Diseño, arquitectura y aportes |
| `CMakeLists.txt` | Cómo se compila: las bibliotecas `escape_core` y `escape_ui`, los dos ejecutables y las pruebas |

Los ejecutables usan la interfaz (`escape_ui`, con FTXUI) y el núcleo (`escape_core`: tablero, entorno, controladores, reglas y
generación de mapas). La interfaz solo dibuja y lee teclas, las reglas están en el entorno. Hay un diagrama en
[docs/architecture/arquitectura.md](docs/architecture/arquitectura.md).

---

## 9. Decisiones de diseño

| Decisión | Por qué |
| :--- | :--- |
| `Grid<Cell>` con filas y columnas que se eligen al ejecutar (`std::vector`) | Así hay mapas desde 7x7 hasta 200 por lado, ya sean generados o leídos de un archivo |
| `Cell` como `std::variant` | Cada tipo de celda es distinto y se maneja con `std::visit`, sin herencia |
| Controladores intercambiables (`IController`, `PolicyController`, concept `NavigationPolicy`) | El jugador, el vigilante y las pruebas usan la misma interfaz |
| El costo de un paso es la energía de la celda a la que entras | Dijkstra calcula lo mismo que gasta el motor, por eso sirve para validar mapas y para la ayuda H |
| El vigilante usa BFS, solo en turnos pares y con los vecinos siempre en el mismo orden (arriba, abajo, izquierda, derecha) | Se comporta igual cada vez y al jugador le da tiempo de escapar |
| La captura gana sobre el escape | Es una regla clara y fácil de probar |
| Kruskal con `DisjointSet` y pasillos extra | El árbol no tiene ciclos, y los pasillos extra dejan más de una ruta |
| Un resultado por mapa, cada hilo escribe en su propia posición | No hace falta mutex y da lo mismo con 1 o con N hilos |
| Un mapa solo es nivel si tiene una partida ganadora simulada | Así todos los niveles se pueden ganar |

Hay más detalle en [docs/design.md](docs/design.md).

---

## 10. Dónde está cada tema

| Tema | Dónde lo usamos | Archivos |
| :--- | :--- | :--- |
| Plantillas | `Grid<T>` y funciones genéricas | `grid.h`, `grid_algorithms.h` |
| Concepts | Políticas de navegación, tipos de celda y grafos | `controllers.h`, `cells.h`, `graph.h`, `scenario.h` |
| Variadic templates | Contar o buscar varios tipos de celda a la vez | `cells.h`, `grid_algorithms.h`, `overloaded.h` |
| `std::variant` y `std::visit` | Celdas del mapa, dibujo en pantalla y eventos del juego | `cells.h`, `overloaded.h`, `environment.hpp`, `console_ui.cpp`, `game_application.cpp` |
| Contenedores de la STL | `vector`, `priority_queue` (Dijkstra), conjuntos y colas (BFS) | `busquedas.hpp`, `graph.h`, `kruskal.h` |
| Complejidad algorítmica | Kruskal, Dijkstra y BFS | `kruskal.h`, `disjoint_set.h`, `busquedas.hpp` |
| Concurrencia | `std::jthread` y `std::atomic` para preparar mapas en paralelo | `map_batch_runner.cpp`, `map_generator_main.cpp` |
| `std::span` y rangos | Vistas de solo lectura sobre acciones y celdas | `controllers.h`, `guard_controller.hpp`, `scenario.h` |

(Los archivos están en `include/circuit_escape/` y `src/`.)

---

## 11. Pruebas

Son 14 pruebas con CTest (sección 3). Las que usan `assert` se compilan sin `NDEBUG`, así que también sirven en Release.

| Prueba | Qué revisa |
| :--- | :--- |
| `grid_test`, `environment_test`, `interactions_test`, `controllers_test`, `maze_test` | Las pruebas del Proyecto 1 que se mantuvieron: tablero, motor, controladores, DisjointSet y Kruskal |
| `kruskal_openings_test` | Costo mínimo en casos conocidos, aperturas del árbol y extras, que no haya ciclos y que la misma semilla dé el mismo mapa |
| `busquedas_test`, `help_test` | Dijkstra, BFS (muros, turnos pares, sin camino, desempate fijo) y la ayuda H |
| `route_evaluation_test` | Los dos recorridos completos, un tramo inaccesible, energía suficiente, cero o insuficiente |
| `map_selector_test` | La elección de fácil, medio y difícil, los empates y los lotes que no tienen tres costos distintos |
| `batch_runner_test`, `real_batch_test` | 1 hilo contra varios: mismos mapas, costos y niveles. Lote vacío y fallo de una tarea |
| `mission_test` | Interruptores en los dos órdenes, captura, vigilante en turnos pares, energía agotada y último turno permitido |
| `level_io_test` | Guardar y cargar niveles, victoria guardada que se puede repetir, tamaños 10x15, 20x30 y 40x60 y archivos inválidos |

---

## 12. Aportes de cada integrante

* **Oriana Manrique:** `Grid` de tamaño variable, grafo, `DisjointSet`, Kruskal, generador de laberintos y mapas `.txt`.
* **Rodolfo Huaroc:** Dijkstra, BFS, `GuardController` (el vigilante) y la pantalla de ayuda H.
* **Pedro Chavez:** interruptores A y B, reglas de captura, interfaz FTXUI, carga de niveles e integración del juego.
* **Indira Yabar:** concurrencia (`run_map_batch` con `std::jthread`), validación de mapas, evaluación de recorridos y energía,
  selección de niveles, `map_generator` y formato de los niveles guardados.

El detalle por commit está en [docs/contributions.md](docs/contributions.md). La base de CMake y de las pruebas viene del
Proyecto 1 y la ampliamos para el Proyecto 2.

---

## 13. Limitaciones

* En mapas chicos (por ejemplo 10x15), con algunas semillas no salen tres niveles ganables. Ahí hay que probar otra semilla.
* Las 14 pruebas las corrimos en Linux. En Windows usamos MinGW y Visual Studio no lo probamos.
* El vigilante solo persigue con BFS, no se adelanta a lo que haces.
* El juego se ve solo en consola, no tiene interfaz gráfica.

---

## 14. Uso de IA

Usamos Claude (un asistente de IA de Anthropic) como ayuda para juntar el código del grupo, corregir errores de compilación,
armar y ordenar las pruebas y escribir la documentación. Todo lo que cambió se compiló, pasó las 14 pruebas y lo revisamos entre
nosotros. Todos podemos explicar y defender el código que entregamos.

---

## 15. Referencias

* [FTXUI](https://github.com/ArthurSonzogni/FTXUI) v7.0.3, librería de interfaz en consola (licencia MIT).
* Teoría del curso CS2013: POO, programación genérica, contenedores, complejidad algorítmica y programación concurrente.
* [cppreference.com](https://en.cppreference.com/) para `std::variant`, `std::jthread` y `std::atomic`.
* Los algoritmos de Kruskal, Dijkstra y BFS, del texto del curso.
* [Inno Setup](https://jrsoftware.org/isinfo.php) para el instalador de Windows.

---

## 16. Detalles técnicos

### Formato de los mapas guardados (`assets/maps/mapa_*.txt`)

Primero va el tablero (una línea por fila y un carácter por celda) y después unas líneas con datos que empiezan con `;`.

```text
#####################          <- tablero: # muro, . libre, ~ terreno elevado, R recurso,
#@....#.....B.#.~.#.#             B batería, T trampa, S salida, @ inicio del jugador
...
; A=(1,12) B=(9,19) vigilante=(11,5) semilla=2034
; id=9 nivel=medio costo_minimo=47 filas=15 columnas=21 energia=108 energia_max=108 turnos=324 entrada=1 rugoso=2 espera=1 invalido=1 recurso=10 recarga=3 trampa_energia=2 trampa_puntos=1
; solucion=RRDDRRRR...
```

| Dato | Qué significa |
| :--- | :--- |
| `A`, `B`, `vigilante` | Posición de los interruptores y del vigilante (el jugador y la salida van dentro del tablero) |
| `semilla`, `id` | La semilla con la que se hizo el mapa y su número dentro del lote |
| `nivel`, `costo_minimo` | `facil`, `medio` o `dificil`, y el costo mínimo calculado con Dijkstra |
| `energia`, `energia_max`, `turnos` | Energía inicial y máxima, y límite de turnos |
| `entrada`, `rugoso`, `espera`, `invalido` | Costo de entrar a una casilla, de un terreno elevado, de esperar y de un movimiento inválido |
| `recurso`, `recarga`, `trampa_energia`, `trampa_puntos` | Puntos de un recurso, energía de una batería y lo que quita una trampa |
| `solucion` | La partida ganadora comprobada: `U` arriba, `D` abajo, `L` izquierda, `R` derecha, `W` esperar |

Al cargar un nivel se revisa que A, B, vigilante, inicio y salida sean distintos y estén en casillas libres.

### Cómo se preparan los mapas en paralelo

```text
Fase 1: build_map (Kruskal)     en paralelo con std::jthread  ->  esperar a que terminen todos
Fase 2: validate_map (Dijkstra) en paralelo                   ->  esperar a que terminen todos
Después, en orden: select_levels (fácil, medio, difícil)
```

* Las tareas se reparten con un contador atómico: cada hilo agarra el siguiente índice que esté libre.
* Los ids y las semillas se preparan antes de lanzar los hilos, y cada tarea escribe solo en su posición del resultado.
  Los errores se guardan por mapa y los junta el hilo principal.
* `validate_map` corre Dijkstra desde el inicio, A y B, compara inicio-A-B-salida con inicio-B-A-salida, mira si alcanza la energía
  y pide una partida ganadora simulada con todas las reglas. Esa partida queda guardada en `solucion`.
* `select_levels`: el fácil es el de menor costo, el difícil el de mayor costo y el medio el más cercano al promedio (sin contar el
  mínimo ni el máximo). Si hay empate se toma el de menor id, y hacen falta al menos tres costos distintos.

### Qué reutilizamos del Proyecto 1

| Componente original | Qué le cambiamos en el Proyecto 2 | Prueba |
| :--- | :--- | :--- |
| `Grid<Cell>`, `Position` | Filas y columnas se eligen al ejecutar | `grid_test`, `level_io_test` |
| `NavigationEnvironment` | Se amplió con interruptores, vigilante y captura. Sin misión funciona igual que antes | `environment_test`, `mission_test` |
| `IController`, `PolicyController`, `NavigationPolicy` | El vigilante (`GuardBfsPolicy`) cumple el mismo concept | `controllers_test`, `help_test` |
| `Observation` | Se le agregó el mapa de solo lectura, los interruptores y la posición del vigilante | `mission_test`, `help_test` |
| `DisjointSet`, `Graph`, Kruskal | `generateMazeReport` separa las aperturas del árbol de las adicionales | `maze_test`, `kruskal_openings_test` |
| FTXUI, CMake, CTest | Pantalla de ayuda H, elegir nivel y dos ejecutables | Todas las pruebas |
