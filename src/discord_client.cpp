#include "discord_client.h"
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <optional>
using json = nlohmann::json;

DiscordClient::DiscordClient(const std::string& webhook_url) : webhook_url_(webhook_url) {
    webhook_path_ = extract_webhook_path(webhook_url);
}

std::string DiscordClient::extract_webhook_path(const std::string& webhook_url) {
    // Extract path from full webhook URL
    // Example: https://discord.com/api/webhooks/123456/abc123 -> /api/webhooks/123456/abc123
    size_t pos = webhook_url.find("://");
    if (pos != std::string::npos) {
        pos = webhook_url.find("/", pos + 3);
        if (pos != std::string::npos) {
            return webhook_url.substr(pos);
        }
    }
    // If no scheme found, assume it's already a path or return as-is
    if (webhook_url[0] == '/') {
        return webhook_url;
    }
    return "/" + webhook_url;
}

bool DiscordClient::send_message(const std::string& message) {
    httplib::Client cli(base_url_);
    
    // Set timeouts
    cli.set_connection_timeout(30, 0);
    cli.set_read_timeout(30, 0);
    
    std::ostringstream json;
    json << R"({"content":")" << escape_json(message) << R"("})";
    
    auto res = cli.Post(webhook_path_, json.str(), "application/json");
    
    if (res && res->status >= 200 && res->status < 300) {
        return true;
    } else {
        std::cerr << "Error sending Discord message: ";
        if (res) {
            std::cerr << "Status " << res->status << ", Response: " << res->body.substr(0, 200) << std::endl;
        } else {
            std::cerr << "Connection failed" << std::endl;
        }
        return false;
    }
}

bool DiscordClient::send_match_report(const MatchData& match, const std::string& comment) {
    httplib::Client cli(base_url_);
    
    // Set timeouts
    cli.set_connection_timeout(30, 0);
    cli.set_read_timeout(30, 0);
    
    std::ostringstream json;
    json << R"({"embeds":[{"title":"Match Report: )" << escape_json(match.map_name) 
         << R"(","description":")" << escape_json(comment) << R"(","color":3447003)";
    
    // Add fields for tracked players
    std::vector<std::string> field_values;
    for (const auto& player : match.players) {
        std::ostringstream field;
        field << player.name << "\n"
              << "K/D: " << player.kills << "/" << player.deaths 
              << " (" << std::fixed << std::setprecision(2) << player.kd_ratio << ")\n"
              << "ADR: " << player.adr << "\n";
              //<< "Rating: " << std::fixed << std::setprecision(2) << player.rating;
        
        json << R"(,{"name":")" << escape_json(player.name) 
             << R"(","value":")" << escape_json(field.str()) << R"(","inline":true})";
    }
    
    json << R"(]})";
    
    auto res = cli.Post(webhook_path_, json.str(), "application/json");
    
    if (!res || res->status < 200 || res->status >= 300) {
        std::cerr << "Discord webhook error: ";
        if (res) {
            std::cerr << "Status " << res->status << ", Response: " << res->body.substr(0, 200) << std::endl;
        } else {
            std::cerr << "Connection failed" << std::endl;
        }
        return false;
    }
    
    return true;
}

bool DiscordClient::send_embed(const std::string& title, const std::string& description,
                               const std::vector<PlayerStats>& players, const std::string& footer) {
    httplib::Client cli(base_url_);
    
    // Set timeouts
    cli.set_connection_timeout(30, 0);
    cli.set_read_timeout(30, 0);
    
    std::ostringstream json;
    json << R"({"embeds":[{"title":")" << escape_json(title)
         << R"(","description":")" << escape_json(description) << R"(","color":3447003)";
    
    for (const auto& player : players) {
        std::ostringstream field;
        field << "K/D: " << player.kills << "/" << player.deaths 
              << " | ADR: " << player.adr;
         //     << " | Rating: " << std::fixed << std::setprecision(2) << player.rating;
        
        json << R"(,{"name":")" << escape_json(player.name)
             << R"(","value":")" << escape_json(field.str()) << R"(","inline":false})";
    }
    
    if (!footer.empty()) {
        json << R"(,"footer":{"text":")" << escape_json(footer) << R"("})";
    }
    
    json << R"(]})";
    
    auto res = cli.Post(webhook_path_, json.str(), "application/json");
    
    if (!res || res->status < 200 || res->status >= 300) {
        std::cerr << "Discord webhook error: ";
        if (res) {
            std::cerr << "Status " << res->status << ", Response: " << res->body.substr(0, 200) << std::endl;
        } else {
            std::cerr << "Connection failed" << std::endl;
        }
        return false;
    }
    
    return true;
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
