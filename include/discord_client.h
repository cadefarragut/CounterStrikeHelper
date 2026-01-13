#pragma once

#include <string>
#include <vector>
#include "match_data.h"

class DiscordClient {
public:
    DiscordClient(const std::string& webhook_url);
    
    // Send a simple text message
    bool send_message(const std::string& message);
    
    // Send a formatted match report
    bool send_match_report(const MatchData& match, const std::string& comment);
    
    // Send a formatted embed with match stats
    bool send_embed(const std::string& title, const std::string& description, 
                    const std::vector<PlayerStats>& players, const std::string& footer);
    
private:
    std::string webhook_url_;
    std::string webhook_path_;
    std::string base_url_ = "https://discord.com";
    
    std::string escape_json(const std::string& str);
    std::string extract_webhook_path(const std::string& webhook_url);
};
