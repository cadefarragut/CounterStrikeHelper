#pragma once

#include <string>
#include <vector>
#include "match_data.h"

class OpenAIClient {
public:
    OpenAIClient(const std::string& api_key);
    
    // Generate a funny comment about a match based on player stats
    std::string generate_match_comment(const MatchData& match, 
                                      const std::vector<std::string>& tracked_steam_ids);
    
    // Generate a comment about specific player performance
    std::string generate_player_comment(const PlayerStats& player, const MatchData& match);
    
private:
    std::string api_key_;
    std::string base_url_ = "api.openai.com";
    std::string api_path_ = "/v1";
    std::string model_ = "gpt-3.5-turbo";
    
    // Build prompt for OpenAI
    std::string build_comment_prompt(const MatchData& match, 
                                     const std::vector<std::string>& tracked_steam_ids);
    
    // Make API call to OpenAI
    std::string make_chat_completion(const std::string& prompt);
};
