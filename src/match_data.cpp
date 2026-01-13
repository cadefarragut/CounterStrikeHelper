#include "match_data.h"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// ============================================================================
// MatchData method implementations
// ============================================================================

std::optional<PlayerStats> MatchData::get_player_stats(const std::string& steam_id) const {
    for (const auto& player : players) {
        if (player.steam_id == steam_id) {
            return player;
        }
    }
    return std::nullopt;
}

bool MatchData::has_tracked_players(const std::vector<std::string>& tracked_ids) const {
    for (const auto& player : players) {
        for (const auto& tracked_id : tracked_ids) {
            if (player.steam_id == tracked_id) {
                return true;
            }
        }
    }
    return false;
}

std::vector<PlayerStats> MatchData::get_tracked_players(const std::vector<std::string>& tracked_ids) const {
    std::vector<PlayerStats> result;
    for (const auto& player : players) {
        for (const auto& tracked_id : tracked_ids) {
            if (player.steam_id == tracked_id) {
                result.push_back(player);
                break;
            }
        }
    }
    return result;
}

std::string MatchData::get_score_string() const {
    if (team_scores.size() < 2) {
        return "?-?";
    }
    // Sort by team number for consistent ordering
    auto scores = team_scores;
    std::sort(scores.begin(), scores.end(), 
        [](const TeamScore& a, const TeamScore& b) { return a.team_number < b.team_number; });
    
    return std::to_string(scores[0].score) + "-" + std::to_string(scores[1].score);
}

bool MatchData::is_valid() const {
    return !match_id.empty() && !players.empty();
}

// ============================================================================
// JSON Parsing functions
// ============================================================================

MatchData parse_match_details_from_json(const std::string& body, const std::string& match_id) {
    MatchData match;
    match.match_id = match_id;

    json j;
    try {
        j = json::parse(body);
    } catch (const std::exception& e) {
        std::cerr << "[parse] JSON parse failed: " << e.what() << "\n";
        return match;
    }

    match.map_name = j.value("map_name", "");
    match.game_finished_at = j.value("game_finished_at", "");

    // Parse team scores
    if (j.contains("team_scores") && j["team_scores"].is_array()) {
        for (const auto& ts : j["team_scores"]) {
            TeamScore score;
            score.team_number = ts.value("team_number", -1);
            score.score = ts.value("score", 0);
            match.team_scores.push_back(score);
        }
    }

    if (!j.contains("stats") || !j["stats"].is_array()) {
        std::cerr << "[parse] Missing stats array\n";
        return match;
    }

    const auto& stats = j["stats"];
    std::cout << "[parse] stats_size=" << stats.size() << "\n";

    match.players.clear();
    match.players.reserve(stats.size());

    for (size_t i = 0; i < stats.size(); ++i) {
        const auto& s = stats.at(i);
        if (!s.is_object()) {
            std::cout << "[parse] stats[" << i << "] not object\n";
            continue;
        }

        PlayerStats pd{};

        pd.steam_id = s.value("steam64_id", "");
        pd.name = s.value("name", "");
        pd.kills = s.value("total_kills", 0);
        pd.deaths = s.value("total_deaths", 0);
        pd.assists = s.value("total_assists", 0);
        pd.kd_ratio = s.value("kd_ratio", 0.0);
        pd.team_number = s.value("initial_team_number", -1);

        // Use "dpr" as ADR (damage per round)
        pd.adr = static_cast<int>(std::lround(s.value("dpr", 0.0)));

        // Calculate headshot percentage
        int hs_kills = s.value("total_hs_kills", 0);
        pd.headshot_percentage = (pd.kills > 0)
            ? static_cast<int>(std::lround(100.0 * hs_kills / pd.kills))
            : 0;

        // Determine win/loss from team scores
        pd.won_match = false;
        if (pd.team_number != -1 && match.team_scores.size() >= 2) {
            int my_score = 0;
            int opponent_score = 0;
            
            for (const auto& ts : match.team_scores) {
                if (ts.team_number == pd.team_number) {
                    my_score = ts.score;
                } else {
                    opponent_score = ts.score;
                }
            }
            pd.won_match = (my_score > opponent_score);
        }

        match.players.push_back(std::move(pd));
    }

    std::cout << "[parse] match.players.size() after loop = " << match.players.size() << "\n";
    return match;
}

std::string parse_most_recent_match_id(const std::string& body) {
    json j;
    try {
        j = json::parse(body);
    } catch (...) {
        return "";
    }

    if (!j.is_array() || j.empty()) return "";

    const auto& first = j.at(0);
    if (!first.is_object()) return "";

    return first.value("id", "");
}
