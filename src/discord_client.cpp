#include "discord_client.h"
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

DiscordClient::DiscordClient(const std::string& webhook_url) : webhook_url_(webhook_url) {
    webhook_path_ = extract_webhook_path(webhook_url);
}

std::string DiscordClient::extract_webhook_path(const std::string& webhook_url) {
    size_t pos = webhook_url.find("://");
    if (pos != std::string::npos) {
        pos = webhook_url.find("/", pos + 3);
        if (pos != std::string::npos) {
            return webhook_url.substr(pos);
        }
    }
    if (webhook_url[0] == '/') {
        return webhook_url;
    }
    return "/" + webhook_url;
}

bool DiscordClient::send_message(const std::string& message) {
    httplib::Client cli(base_url_);
    cli.set_connection_timeout(30, 0);
    cli.set_read_timeout(30, 0);
    
    json payload;
    payload["content"] = message;
    
    auto res = cli.Post(webhook_path_, payload.dump(), "application/json");
    
    if (res && res->status >= 200 && res->status < 300) {
        return true;
    } else {
        std::cerr << "[discord] Error sending message: ";
        if (res) {
            std::cerr << "Status " << res->status << "\n";
        } else {
            std::cerr << "Connection failed\n";
        }
        return false;
    }
}

bool DiscordClient::send_match_report(const MatchData& match, const std::string& comment) {
    httplib::Client cli(base_url_);
    cli.set_connection_timeout(30, 0);
    cli.set_read_timeout(30, 0);
    
    // Just send the AI comment as a simple message
    // Include map and score as a header, then the comment
    std::ostringstream message;
    message << "🎮 **" << match.map_name << "** (" << match.get_score_string() << ")\n\n";
    message << comment;
    
    json payload;
    payload["content"] = message.str();
    
    auto res = cli.Post(webhook_path_, payload.dump(), "application/json");
    
    if (res && res->status >= 200 && res->status < 300) {
        return true;
    } else {
        std::cerr << "[discord] Error sending match report: ";
        if (res) {
            std::cerr << "Status " << res->status << ", Body: " << res->body.substr(0, 200) << "\n";
        } else {
            std::cerr << "Connection failed\n";
        }
        return false;
    }
}

bool DiscordClient::send_embed(const std::string& title, const std::string& description,
                               const std::vector<PlayerStats>& players, const std::string& footer_text) {
    // Keep this for backwards compatibility, but simplify it
    httplib::Client cli(base_url_);
    cli.set_connection_timeout(30, 0);
    cli.set_read_timeout(30, 0);
    
    std::ostringstream message;
    message << "**" << title << "**\n";
    message << description;
    
    if (!footer_text.empty()) {
        message << "\n\n_" << footer_text << "_";
    }
    
    json payload;
    payload["content"] = message.str();
    
    auto res = cli.Post(webhook_path_, payload.dump(), "application/json");
    
    return res && res->status >= 200 && res->status < 300;
}

std::string DiscordClient::escape_json(const std::string& str) {
    std::ostringstream escaped;
    for (char c : str) {
        switch (c) {
            case '"': escaped << "\\\""; break;
            case '\\': escaped << "\\\\"; break;
            case '\b': escaped << "\\b"; break;
            case '\f': escaped << "\\f"; break;
            case '\n': escaped << "\\n"; break;
            case '\r': escaped << "\\r"; break;
            case '\t': escaped << "\\t"; break;
            default:
                if (c < 0x20) {
                    escaped << "\\u" << std::hex << std::setw(4) << std::setfill('0') << (int)c;
                } else {
                    escaped << c;
                }
                break;
        }
    }
    return escaped.str();
}