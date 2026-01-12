#include "match_data.h"
#include <algorithm>
#include <regex>
#include <iomanip>
#include <sstream>
#include <unordered_map>
#include <ctime>
#include <nlohmann/json.hpp>
#include <optional>
#include <iostream>
using json = nlohmann::json;


MatchData parse_match_details_from_json(const std::string& body, const std::string& match_id) {
    MatchData match;
    match.match_id = match_id;

    json j;
    try {
        j = json::parse(body);
    } catch (...) {
        std::cerr << "[parse] json parse failed\n";
        return match;
    }

    match.map_name = j.value("map_name", "");

    if (!j.contains("stats") || !j["stats"].is_array()) {
        std::cerr << "[parse] missing stats array\n";
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

        PlayerStats pd{}; // zero-init

        pd.steam_id = s.value("steam64_id", "");
        pd.name     = s.value("name", "");

        pd.kills   = s.value("total_kills", 0);
        pd.deaths  = s.value("total_deaths", 0);
        pd.assists = s.value("total_assists", 0);

        pd.kd_ratio = s.value("kd_ratio", 0.0);

        // Use "dpr" as ADR-ish (damage per round)
        pd.adr = (int)std::lround(s.value("dpr", 0.0));

        int hs_kills = s.value("total_hs_kills", 0);
        pd.headshot_percentage = (pd.kills > 0)
            ? (int)std::lround(100.0 * (double)hs_kills / (double)pd.kills)
            : 0;

        // win/loss from team_scores
        pd.won_match = false;
        int team = s.value("initial_team_number", -1);
        if (team != -1 && j.contains("team_scores") && j["team_scores"].is_array()) {
            int my_score = -1, opp_score = -1;
            for (const auto& ts : j["team_scores"]) {
                int t = ts.value("team_number", -1);
                int sc = ts.value("score", 0);
                if (t == team) my_score = sc;
                else opp_score = std::max(opp_score, sc);
            }
            if (my_score != -1 && opp_score != -1) pd.won_match = (my_score > opp_score);
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

    std::string id = first.value("id", "");
    if (id.empty()) return "";

    return id;
}

