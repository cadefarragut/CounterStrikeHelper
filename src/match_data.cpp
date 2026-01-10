#include "match_data.h"
#include <algorithm>
#include <regex>
#include <iomanip>
#include <sstream>
#include <unordered_map>
#include <ctime>

// Simple JSON helper functions (basic parsing for Leetify API format)
// Note: For production, consider using nlohmann/json library

namespace {
    // Extract string value from JSON
    std::string extract_string(const std::string& json, const std::string& key) {
        std::regex pattern("\"" + key + "\"\\s*:\\s*\"([^\"]+)\"");
        std::smatch match;
        if (std::regex_search(json, match, pattern)) {
            return match[1].str();
        }
        return "";
    }
    
    // Extract number value from JSON
    double extract_number(const std::string& json, const std::string& key) {
        std::regex pattern("\"" + key + "\"\\s*:\\s*([0-9]+\\.?[0-9]*)");
        std::smatch match;
        if (std::regex_search(json, match, pattern)) {
            return std::stod(match[1].str());
        }
        return 0.0;
    }
    
    // Extract integer value from JSON
    int extract_int(const std::string& json, const std::string& key) {
        std::regex pattern("\"" + key + "\"\\s*:\\s*([0-9]+)");
        std::smatch match;
        if (std::regex_search(json, match, pattern)) {
            return std::stoi(match[1].str());
        }
        return 0;
    }
    
    // Extract boolean value from JSON
    bool extract_bool(const std::string& json, const std::string& key) {
        std::regex pattern("\"" + key + "\"\\s*:\\s*(true|false)");
        std::smatch match;
        if (std::regex_search(json, match, pattern)) {
            return match[1].str() == "true";
        }
        return false;
    }
}

const PlayerStats* MatchData::get_player_stats(const std::string& steam_id) const {
    for (const auto& player : players) {
        if (player.steam_id == steam_id) {
            return &player;
        }
    }
    return nullptr;
}

bool MatchData::has_tracked_players(const std::vector<std::string>& tracked_ids) const {
    for (const auto& tracked_id : tracked_ids) {
        for (const auto& player : players) {
            if (player.steam_id == tracked_id) {
                return true;
            }
        }
    }
    return false;
}

MatchData parse_match_from_json(const std::string& json, const std::string& match_id) {
    MatchData match;
    match.match_id = match_id;
    
    // Parse basic match info
    match.map_name = extract_string(json, "mapName");
    match.game_mode = extract_string(json, "gameMode");
    
    // Parse timestamp (assuming Unix timestamp)
    // Note: Leetify API format may vary, adjust as needed
    int64_t timestamp = extract_int(json, "finishedAt");
    match.timestamp = static_cast<std::time_t>(timestamp / 1000);  // Assuming milliseconds
    
    // Parse players array
    // This is a simplified parser - you may need to adjust based on actual API response
    // Look for "players" array in JSON
    std::regex players_pattern("\"players\"\\s*:\\s*\\[([^\\]]+)\\]");
    std::smatch players_match;
    
    if (std::regex_search(json, players_match, players_pattern)) {
        // Extract individual player objects
        // This is a basic implementation - you may need more sophisticated parsing
        // For production, use a proper JSON library like nlohmann/json
    }
    
    return match;
}

std::vector<MatchData> parse_matches_list_from_json(const std::string& json) {
    std::vector<MatchData> matches;

    // Leetify profile/matches endpoint returns an array of match objects
    // Fields seen: id, finished_at (ISO string), map_name, stats (array)

    // Find each match object by "id"
    std::regex match_pattern("\"id\"\\s*:\\s*\"([^\"]+)\"");
    std::sregex_iterator iter(json.begin(), json.end(), match_pattern);
    std::sregex_iterator end;

    for (std::sregex_iterator i = iter; i != end; ++i) {
        std::smatch m = *i;
        if (m.size() < 2) continue;

        MatchData match;
        match.match_id = m[1].str();

        // Narrow context around this match to pick other fields
        size_t pos = m.position();
        size_t ctx_start = (pos > 500) ? pos - 500 : 0;
        size_t ctx_end = std::min(pos + 2000, json.size());
        std::string ctx = json.substr(ctx_start, ctx_end - ctx_start);

        // map_name
        std::regex map_pattern("\"map_name\"\\s*:\\s*\"([^\"]+)\"");
        std::smatch map_match;
        if (std::regex_search(ctx, map_match, map_pattern) && map_match.size() > 1) {
            match.map_name = map_match[1].str();
        }

        // finished_at (ISO 8601)
        std::regex finished_pattern("\"finished_at\"\\s*:\\s*\"([^\"]+)\"");
        std::smatch fin_match;
        if (std::regex_search(ctx, fin_match, finished_pattern) && fin_match.size() > 1) {
            // We won't fully parse ISO to time_t here; leave timestamp as 0 for simplicity
            // Could be improved by parsing with std::get_time + from_time_t
        }

        // Parse stats array entries for this match
        // Look for entries like: {"steam64_id":"...","name":"...","kills":X,"deaths":Y,"assists":Z,"adr":N,"rating":R}
        std::regex stats_pattern("\\{[^\\}]*\"steam64_id\"\\s*:\\s*\"([^\"]+)\"[^\\}]*\\}");
        auto stats_begin = std::sregex_iterator(ctx.begin(), ctx.end(), stats_pattern);
        auto stats_end = std::sregex_iterator();
        for (auto s = stats_begin; s != stats_end; ++s) {
            std::string entry = s->str();
            PlayerStats p;
            // steam id
            p.steam_id = (*s)[1].str();
            // name
            std::regex name_re("\"name\"\\s*:\\s*\"([^\"]+)\"");
            std::smatch name_match;
            if (std::regex_search(entry, name_match, name_re) && name_match.size() > 1) {
                p.name = name_match[1].str();
            }
            // kills/deaths/assists
            auto extract_int_field = [&](const std::string& key) -> int {
                std::regex r("\"" + key + "\"\\s*:\\s*([0-9]+)");
                std::smatch m2;
                if (std::regex_search(entry, m2, r) && m2.size() > 1) {
                    return std::stoi(m2[1].str());
                }
                return 0;
            };
            auto extract_double_field = [&](const std::string& key) -> double {
                std::regex r("\"" + key + "\"\\s*:\\s*([0-9]+\\.?[0-9]*)");
                std::smatch m2;
                if (std::regex_search(entry, m2, r) && m2.size() > 1) {
                    return std::stod(m2[1].str());
                }
                return 0.0;
            };

            p.kills = extract_int_field("kills");
            p.deaths = extract_int_field("deaths");
            p.assists = extract_int_field("assists");
            p.adr = extract_int_field("adr");
            p.headshot_percentage = extract_int_field("hs_percentage");
            p.rating = extract_double_field("rating");
            p.leetify_rating = extract_double_field("leetify_rating");
            p.kd_ratio = (p.deaths == 0) ? static_cast<double>(p.kills) : static_cast<double>(p.kills) / std::max(1, p.deaths);
            p.won_match = false; // Unknown from this endpoint

            match.players.push_back(p);
        }

        matches.push_back(match);
    }

    return matches;
}
