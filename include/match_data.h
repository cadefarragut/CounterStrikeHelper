#pragma once

#include <string>
#include <vector>
#include <optional>

struct PlayerStats {
    std::string steam_id;
    std::string name;
    int kills = 0;
    int deaths = 0;
    int assists = 0;
    double kd_ratio = 0.0;
    int headshot_percentage = 0;
    int adr = 0;  
    bool won_match = false;
    int team_number = -1;
};

struct TeamScore {
    int team_number = -1;
    int score = 0;
};

struct MatchData {
    std::string match_id;
    std::string map_name;
    std::string game_finished_at;
    std::vector<PlayerStats> players;
    std::vector<TeamScore> team_scores;
    
    // Get stats for a specific Steam ID
    std::optional<PlayerStats> get_player_stats(const std::string& steam_id) const;
    
    // Check if any tracked players are in this match
    bool has_tracked_players(const std::vector<std::string>& tracked_ids) const;
    
    // Get all tracked players' stats
    std::vector<PlayerStats> get_tracked_players(const std::vector<std::string>& tracked_ids) const;
    
    // Get final score as string (e.g., "13-10")
    std::string get_score_string() const;
    
    // Check if match data is valid/complete
    bool is_valid() const;
};

// Parse JSON response from Leetify API
MatchData parse_match_details_from_json(const std::string& body, const std::string& match_id);
std::string parse_most_recent_match_id(const std::string& body);
