#include "circuit_escape/map_generation/level_preparation.hpp"

namespace circuit_escape {

namespace {

std::optional<GameLevel> level_for(const BatchReport& report, int map_id, const GameRules& rules,
                                   const std::string& difficulty) {
    for (std::size_t i = 0; i < report.maps.size(); ++i) {
        if (report.maps[i].id == map_id) {
            return make_game_level(report.maps[i], report.results[i], rules, difficulty);
        }
    }
    return std::nullopt;
}

}  // namespace

PreparedLevels prepare_levels(MapSize size, const BatchConfig& config) {
    validate_map_size(size);

    PreparedLevels prepared;
    prepared.rules = rules_for_size(size);
    const GameRules rules = prepared.rules;

    const BuildFunction build = [size](int id, std::uint32_t seed) { return build_map(id, seed, size); };
    const ValidateFunction validate = [rules](const MapCandidate& map) { return validate_map(map, rules); };

    prepared.report = run_map_batch(config, build, validate);
    prepared.selection = select_levels(prepared.report.results);

    if (prepared.selection.ok) {
        prepared.easy = level_for(prepared.report, prepared.selection.easy_id, rules, "facil");
        prepared.medium = level_for(prepared.report, prepared.selection.medium_id, rules, "medio");
        prepared.hard = level_for(prepared.report, prepared.selection.hard_id, rules, "dificil");
    }
    return prepared;
}

}  // namespace circuit_escape
