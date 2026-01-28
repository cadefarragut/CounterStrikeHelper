#pragma once

#include <string>
#include <vector>
#include <map>
#include "match_data.h"

class DiscordClient {
public:
    DiscordClient(const std::string& webhook_url);

    // Send a simple text message
    bool send_message(const std::string& message);

    // Send a formatted match report
    bool send_match_report(const MatchData& match, const std::string& comment);

    // Send a match report with individual comments for multiple tracked players
    bool send_multi_player_report(const MatchData& match,
                                  const std::vector<PlayerStats>& tracked_players,
                                  const std::map<std::string, std::string>& player_comments);

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
