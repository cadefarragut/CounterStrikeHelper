#pragma once

#include <string>
#include <vector>
#include <map>
#include "match_data.h"

class AIClient {
public:
    AIClient(const std::string& api_key);

    // Generate a funny comment about a match based on player stats
    std::string generate_match_comment(const MatchData& match,
                                       const std::vector<std::string>& tracked_steam_ids);

    // Generate a comment about specific player performance
    std::string generate_player_comment(const PlayerStats& player, const MatchData& match);

    // Generate individual comments for multiple tracked players in one match
    // Returns map of steam_id -> comment
    std::map<std::string, std::string> generate_multi_player_comments(
        const MatchData& match,
        const std::vector<PlayerStats>& tracked_players);

private:
    std::string api_key_;

    std::string build_comment_prompt(const MatchData& match,
                                     const std::vector<std::string>& tracked_steam_ids);

    std::string build_multi_player_prompt(const MatchData& match,
                                          const std::vector<PlayerStats>& tracked_players);

    std::string make_api_request(const std::string& prompt);
};

// Keep old name for compatibility
using OpenAIClient = AIClient;