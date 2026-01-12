#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <ctime>
#include <nlohmann/json.hpp>
#include <optional>
using json = nlohmann::json;

struct PlayerStats {
    std::string steam_id;
    std::string name;
    int kills;
    int deaths;
    int assists;
    double kd_ratio;
    int headshot_percentage;
    int adr;  
    bool won_match;
};

struct MatchData {
    std::string match_id;
    std::string map_name;
    std::vector<PlayerStats> players;
    
    // Get stats for a specific Steam ID (const version)
    const PlayerStats* get_player_stats(const std::string& steam_id) const;
    
    // Check if any tracked players are in this match
    bool has_tracked_players(const std::vector<std::string>& tracked_ids) const;
};

// Parse JSON response from Leetify API
MatchData parse_match_details_from_json(const std::string& body, const std::string& match_id);
std::string parse_most_recent_match_id(const std::string& body);

