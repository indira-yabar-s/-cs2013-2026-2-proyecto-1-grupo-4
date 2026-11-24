// Reglas del juego con interruptores y vigilante (secciones 3.1, 3.2 y 7 del enunciado):
// activacion persistente, salida anticipada, capturas, energia, limite de turnos, espera,
// movimiento invalido, reinicio y partida completa.

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

#include "../test_check.hpp"
#include "circuit_escape/controllers/guard_controller.hpp"
#include "circuit_escape/environment.hpp"
#include "circuit_escape/scenario.h"

using namespace circuit_escape;

namespace {

// Pasillo de prueba. 'V' vive en una casilla sellada del fondo: el vigilante no puede salir.
//   #########
//   #B.@.A.S#     B=(1,1)  inicio=(1,3)  A=(1,5)  salida=(1,7)
//   #########
//   #V#######
//   #########
Scenario corridor() {
    return scenarioFromLines(std::vector<std::string>{
        "#########",
        "#..@...S#",
        "#########",
        "#.#######",
        "#########",
    });
}

constexpr Position kB{1, 1};
constexpr Position kA{1, 5};
constexpr Position kSealedGuard{3, 1};

GameRules rules_with(int energy, std::size_t turns) {
    GameRules rules = rulesFor(Difficulty::standard);
    rules.initialEnergy = energy;
    rules.turnLimit = turns;
    return rules;
}

NavigationEnvironment make_env(const Scenario& scenario, const GameRules& rules, Position a, Position b,
                               Position guard, NavigationEnvironment::GuardPolicy policy) {
    NavigationEnvironment env(scenario.grid, scenario.start, rules);
    env.enableMission(a, b, guard, std::move(policy));
    return env;
}

NavigationEnvironment::GuardPolicy still_guard() {
    return [](Position, Position, std::size_t) { return Action::wait; };
}

StepResult play(NavigationEnvironment& env, const std::vector<Action>& actions) {
    StepResult last;
    for (Action action : actions) {
        last = env.step(action);
    }
    return last;
}

const std::vector<Action> kAThenB{Action::right, Action::right,                                   // A
                                  Action::left, Action::left, Action::left, Action::left,        // B
                                  Action::right, Action::right, Action::right, Action::right,     // salida
                                  Action::right, Action::right};
const std::vector<Action> kBThenA{Action::left, Action::left,                                     // B
                                  Action::right, Action::right, Action::right, Action::right,    // A
                                  Action::right, Action::right};                                 // salida

void test_switches_persist_and_both_orders_win() {
    const Scenario scenario = corridor();

    auto env_ab = make_env(scenario, rules_with(40, 100), kA, kB, kSealedGuard, still_guard());
    env_ab.step(Action::right);
    const StepResult at_a = env_ab.step(Action::right);
    check(at_a.observation.switch_a_activated && !at_a.observation.switch_b_activated, "A se activa al entrar");
    const StepResult moved_away = env_ab.step(Action::left);
    check(moved_away.observation.switch_a_activated, "A permanece activado al salir");
    const StepResult result_ab = play(env_ab, {Action::left, Action::left, Action::left, Action::right, Action::right,
                                               Action::right, Action::right, Action::right, Action::right});
    check(result_ab.finished && result_ab.reason == EndReason::goalReached, "orden A luego B: victoria");

    auto env_ba = make_env(scenario, rules_with(40, 100), kA, kB, kSealedGuard, still_guard());
    const StepResult result_ba = play(env_ba, kBThenA);
    check(result_ba.finished && result_ba.reason == EndReason::goalReached, "orden B luego A: victoria");
}

void test_early_exit_does_not_win() {
    auto env = make_env(corridor(), rules_with(60, 100), kA, kB, kSealedGuard, still_guard());
    const StepResult at_exit = play(env, {Action::right, Action::right, Action::right, Action::right});
    check(env.state().agent == Position({1, 7}), "el jugador pisa la salida");
    check(!at_exit.finished, "pisar la salida sin B no termina la partida");
    check(at_exit.observation.switch_a_activated && !at_exit.observation.switch_b_activated, "solo A activo");

    // Despues activa B y vuelve: ahora si gana.
    const StepResult won = play(env, {Action::left, Action::left, Action::left, Action::left, Action::left,
                                      Action::left, Action::right, Action::right, Action::right, Action::right,
                                      Action::right, Action::right});
    check(won.finished && won.reason == EndReason::goalReached, "vuelve a la salida con A y B: gana");
}

void test_capture_on_exit_beats_escape() {
    // Inicio (1,1), A (1,2), B (1,3), salida (1,4), vigilante (1,6) que avanza a la izquierda.
    const Scenario scenario = scenarioFromLines(std::vector<std::string>{
        "########",
        "#@..S..#",
        "########",
    });
    //   t1: espera                (impar: el vigilante no se mueve)
    //   t2: A=(1,2)               (par: vigilante (1,6)->(1,5))
    //   t3: B=(1,3)               (impar)
    //   t4: salida=(1,4)          (par: el vigilante (1,5)->(1,4) y alcanza al jugador en la salida)
    auto env = make_env(scenario, rules_with(40, 100), {1, 2}, {1, 3}, {1, 6},
                        [](Position, Position, std::size_t) { return Action::left; });
    const StepResult result = play(env, {Action::wait, Action::right, Action::right, Action::right});
    check(result.observation.switch_a_activated, "A estaba activo");
    check(result.finished, "termina");
    check(result.reason == EndReason::captured, "la captura tiene prioridad sobre el escape");
}

void test_player_steps_on_guard() {
    auto env = make_env(corridor(), rules_with(40, 100), kA, kB, {1, 4}, still_guard());
    const StepResult result = env.step(Action::right);  // el jugador entra en (1,4), donde esta el vigilante
    check(result.finished && result.reason == EndReason::captured, "el jugador pisa al vigilante: pierde");
}

// Pasillo con salida abajo a la derecha; A y B estan en casillas selladas.
Scenario chase_line() {
    return scenarioFromLines(std::vector<std::string>{
        "###########",
        "#.@.......#",
        "#########S#",
        "#.#.#######",
        "###########",
    });
}

void test_guard_moves_only_on_even_turns() {
    const Scenario line = chase_line();
    NavigationEnvironment env(line.grid, line.start, rules_with(60, 100));
    enable_guarded_mission(env, {3, 1}, {3, 3}, {1, 9});  // vigilante real (BFS)

    std::vector<Position> guard_positions;
    guard_positions.push_back(*env.state().guard);
    for (int turn = 1; turn <= 6; ++turn) {
        env.step(Action::wait);
        guard_positions.push_back(*env.state().guard);
    }
    check(guard_positions[1] == guard_positions[0], "turno 1 (impar): el vigilante espera");
    check(guard_positions[2] != guard_positions[1], "turno 2 (par): el vigilante avanza");
    check(guard_positions[3] == guard_positions[2], "turno 3 (impar): espera");
    check(guard_positions[4] != guard_positions[3], "turno 4 (par): avanza");
    check(guard_positions[5] == guard_positions[4], "turno 5 (impar): espera");
    check(guard_positions[6] != guard_positions[5], "turno 6 (par): avanza");
    check(guard_positions[2] == Position({1, 8}), "primer avance: (1,9) -> (1,8)");
    check(guard_positions[4] == Position({1, 7}), "segundo avance: (1,8) -> (1,7)");
}

void test_energy_and_waits() {
    // Esperar consume un turno y una unidad de energia.
    auto env = make_env(corridor(), rules_with(5, 100), kA, kB, kSealedGuard, still_guard());
    const StepResult waited = env.step(Action::wait);
    check(waited.observation.energy == 4 && waited.observation.turn == 1, "esperar: -1 energia y 1 turno");

    // Movimiento invalido (contra un muro) tambien.
    const StepResult invalid = env.step(Action::up);
    check(invalid.observation.energy == 3 && invalid.observation.turn == 2, "movimiento invalido: -1 energia y 1 turno");
    check(invalid.observation.agent == Position({1, 3}), "el jugador no se mueve");

    // Energia agotada: derrota.
    play(env, {Action::wait, Action::wait, Action::wait});
    check(env.isFinished() && env.endReason() == EndReason::noEnergy, "energia agotada: derrota");
}

void test_arriving_with_zero_energy_does_not_win() {
    // Camino A -> B -> salida cuesta exactamente 12; con energia 12 llega en cero: no gana.
    auto exact = make_env(corridor(), rules_with(12, 100), kA, kB, kSealedGuard, still_guard());
    const StepResult zero = play(exact, kAThenB);
    check(zero.finished && zero.reason == EndReason::noEnergy, "energia igual al costo: llega en cero y no gana");

    auto enough = make_env(corridor(), rules_with(13, 100), kA, kB, kSealedGuard, still_guard());
    const StepResult won = play(enough, kAThenB);
    check(won.finished && won.reason == EndReason::goalReached && won.observation.energy == 1,
          "energia mayor al costo: gana con energia restante");
}

void test_turn_limit_including_last_turn() {
    // El ultimo turno permitido cuenta: la victoria en el turno 12 con limite 12 es valida.
    auto last_turn = make_env(corridor(), rules_with(40, 12), kA, kB, kSealedGuard, still_guard());
    const StepResult won = play(last_turn, kAThenB);
    check(won.finished && won.reason == EndReason::goalReached, "victoria en el ultimo turno permitido");

    auto too_late = make_env(corridor(), rules_with(40, 11), kA, kB, kSealedGuard, still_guard());
    const StepResult lost = play(too_late, {Action::right, Action::right, Action::left, Action::left, Action::left,
                                            Action::left, Action::right, Action::right, Action::right, Action::right,
                                            Action::right});
    check(lost.finished && lost.reason == EndReason::turnLimit, "el limite termina sin victoria");
}

void test_reset_restores_initial_state() {
    auto env = make_env(corridor(), rules_with(40, 100), kA, kB, kSealedGuard, still_guard());
    play(env, {Action::right, Action::right, Action::left});
    env.reset(0);
    const Observation state = env.state();
    check(state.agent == Position({1, 3}), "reset: posicion inicial");
    check(state.energy == 40 && state.turn == 0 && state.score == 0, "reset: energia, turno y puntos");
    check(!state.switch_a_activated && !state.switch_b_activated, "reset: A y B desactivados");
    check(*state.guard == kSealedGuard, "reset: vigilante en su casilla inicial");
    check(!env.isFinished(), "reset: partida activa");

    // Despues del reinicio se puede ganar de nuevo con la misma secuencia.
    const StepResult won = play(env, kAThenB);
    check(won.finished && won.reason == EndReason::goalReached, "reset: la misma secuencia gana otra vez");
}

void test_standing_player_is_caught_by_real_guard() {
    const Scenario line = chase_line();
    NavigationEnvironment env(line.grid, line.start, rules_with(80, 200));
    enable_guarded_mission(env, {3, 1}, {3, 3}, {1, 9});
    for (int i = 0; i < 60 && !env.isFinished(); ++i) {
        env.step(Action::wait);
    }
    check(env.isFinished() && env.endReason() == EndReason::captured, "un jugador quieto es alcanzado por el vigilante");
}

}  // namespace

int main() {
    test_switches_persist_and_both_orders_win();
    test_early_exit_does_not_win();
    test_capture_on_exit_beats_escape();
    test_player_steps_on_guard();
    test_guard_moves_only_on_even_turns();
    test_energy_and_waits();
    test_arriving_with_zero_energy_does_not_win();
    test_turn_limit_including_last_turn();
    test_reset_restores_initial_state();
    test_standing_player_is_caught_by_real_guard();
    return test_exit_code();
}
