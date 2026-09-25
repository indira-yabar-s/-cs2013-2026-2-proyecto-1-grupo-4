#ifndef PROYECTO1PG3_CONTROLLERS_H
#define PROYECTO1PG3_CONTROLLERS_H

#include <concepts>
#include <cstdint>
#include <optional>
#include <random>
#include <span>
#include <utility>

#include "observation.h"
#include "position.h"

namespace circuit_escape {

    // Interfaz común para todos los controladores.
    class IController {
    public:
        virtual ~IController() = default;

        virtual Action selectAction(
            const Observation& observation,
            std::span<const Action> legalActions
        ) = 0;
    };


    // Una política válida debe implementar selectAction(...)
    // y devolver un Action.
    template <typename Policy>
    concept NavigationPolicy = requires(
        Policy& policy,
        const Observation& observation,
        std::span<const Action> actions
    ) {
        {
            policy.selectAction(observation, actions)
        } -> std::same_as<Action>;
    };


    // Adaptador entre una Policy genérica y la interfaz IController.
    template <NavigationPolicy Policy>
    class PolicyController final : public IController {
    public:
        explicit PolicyController(Policy policy)
            : policy_(std::move(policy)) {
        }

        Action selectAction(
            const Observation& observation,
            std::span<const Action> legalActions
        ) override {
            return policy_.selectAction(
                observation,
                legalActions
            );
        }

    private:
        Policy policy_;
    };


    // Política que selecciona aleatoriamente una acción legal.
    class RandomPolicy {
    public:
        explicit RandomPolicy(std::uint32_t seed = 0u);

        void reseed(std::uint32_t seed);

        Action selectAction(
            const Observation& observation,
            std::span<const Action> legalActions
        );

    private:
        std::mt19937 generator_;
    };


    // Política que intenta acercarse a la meta
    // minimizando la distancia Manhattan.
    class HeuristicPolicy {
    public:
        Action selectAction(
            const Observation& observation,
            std::span<const Action> legalActions
        );

    private:
        static std::size_t manhattan(
            Position a,
            Position b
        );
    };


    // Controlador para una decisión entregada por la interfaz.
    // No lee directamente desde std::cin.
    class HumanController final : public IController {
    public:
        void provideAction(Action action);

        [[nodiscard]]
        bool hasPendingAction() const noexcept;

        Action selectAction(
            const Observation& observation,
            std::span<const Action> legalActions
        ) override;

    private:
        std::optional<Action> pendingAction_;
    };

} // namespace circuit_escape

#endif // PROYECTO1PG3_CONTROLLERS_H