#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <ctime>

struct PlayerStats {
    std::string steam_id;
    std::string name;
    double rating;
    double leetify_rating;
    int kills;
    int deaths;
    int assists;
    double kd_ratio;
    int headshot_percentage;
    int adr;  // Average Damage per Round
    bool won_match;
};

struct MatchData {
    std::string match_id;
    std::time_t timestamp;
    std::string map_name;
    std::string game_mode;
    std::vector<PlayerStats> players;
    
    // Get stats for a specific Steam ID (const version)
    const PlayerStats* get_player_stats(const std::string& steam_id) const;
    
    // Check if any tracked players are in this match
    bool has_tracked_players(const std::vector<std::string>& tracked_ids) const;
};

// Parse JSON response from Leetify API
MatchData parse_match_from_json(const std::string& json, const std::string& match_id);
std::vector<MatchData> parse_matches_list_from_json(const std::string& json);
