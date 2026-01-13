#pragma once

#include <string>
#include <vector>
#include "match_data.h"

class LeetifyClient {
public:
    LeetifyClient(const std::string& api_key);
    
    // Fetch the most recent match for a Steam ID
    MatchData fetch_recent_match(const std::string& steam64_id);
    
    // Fetch detailed match data by match ID
    MatchData fetch_match_details(const std::string& match_id);
    
    // Check if API key is valid
    bool is_valid() const;
    
private:
    std::string api_key_;
    std::string base_url_ = "https://api-public.cs-prod.leetify.com";
};
