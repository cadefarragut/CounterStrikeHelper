#pragma once

#include <string>
#include <vector>
#include "match_data.h"

class AIClient {
public:
    AIClient(const std::string& api_key);
    
    // Generate a funny comment about a match based on player stats
    std::string generate_match_comment(const MatchData& match, 
                                       const std::vector<std::string>& tracked_steam_ids);
    
    // Generate a comment about specific player performance
    std::string generate_player_comment(const PlayerStats& player, const MatchData& match);
    
private:
    std::string api_key_;
    
    std::string build_comment_prompt(const MatchData& match, 
                                     const std::vector<std::string>& tracked_steam_ids);
    
    std::string make_api_request(const std::string& prompt);
};

// Keep old name for compatibility
using OpenAIClient = AIClient;