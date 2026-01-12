#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "openai_client.h"
#include "httplib.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <optional>
using json = nlohmann::json;

OpenAIClient::OpenAIClient(const std::string& api_key) : api_key_(api_key) {}

std::string OpenAIClient::generate_match_comment(const MatchData& match,
                                                 const std::vector<std::string>& tracked_steam_ids) {
    std::string prompt = build_comment_prompt(match, tracked_steam_ids);
    return make_chat_completion(prompt);
}

std::string OpenAIClient::generate_player_comment(const PlayerStats& player, const MatchData& match) {
    std::ostringstream prompt;
    prompt << "Write a funny, short comment (2-3 sentences max) about this CS2 match performance. ";
    prompt << "Be playful and humorous - make fun if they did bad, praise if they did good. ";
    prompt << "Player: " << player.name << "\n";
    prompt << "K/D: " << player.kills << "/" << player.deaths << "\n";
    prompt << "ADR: " << player.adr << "\n";
    prompt << "Match result: " << (player.won_match ? "Won" : "Lost") << "\n";
    prompt << "Map: " << match.map_name << "\n";
    prompt << "Keep it light-hearted and entertaining!";
    
    return make_chat_completion(prompt.str());
}

std::string OpenAIClient::build_comment_prompt(const MatchData& match,
                                               const std::vector<std::string>& tracked_steam_ids) {
    std::ostringstream prompt;
    prompt << "Write a funny, entertaining comment about this CS2 match. ";
    prompt << "Focus on the tracked players' performances. ";
    prompt << "Be playful and humorous - roast them if they did bad, celebrate if they did well. ";
    prompt << "Keep it 3-4 sentences max. Match details:\n\n";
    prompt << "Map: " << match.map_name << "\n";
    prompt << "Tracked Players:\n";
    
   /* for (const auto& steam_id : tracked_steam_ids) {
        const PlayerStats* player = match.get_player_stats(steam_id);
        if (player) {
            prompt << "- " << player->name << ": K/D " << player->kills << "/" << player->deaths;
            prompt << ", ADR " << player->adr;
            prompt << " (" << (player->won_match ? "Won" : "Lost") << ")\n";
        }
    } */
    
    return prompt.str();
}

std::string OpenAIClient::make_chat_completion(const std::string& prompt) {
    // Use SSL client with host only (no scheme)
    httplib::SSLClient cli(base_url_);
 
    auto escape_json = [](const std::string& in) {
        std::ostringstream out;
        for (char c : in) {
            switch (c) {
                case '"':  out << "\\\""; break;
                case '\\': out << "\\\\"; break;
                case '\n': out << "\\n";  break;
                case '\r': out << "\\r";  break;
                case '\t': out << "\\t";  break;
                default:   out << c;      break;
            }
        }
        return out.str();
    };

    std::string escaped_prompt = escape_json(prompt);

    // Build JSON
    std::ostringstream json;
    json << "{"
         << "\"model\":\"" << model_ << "\","
         << "\"messages\":[{\"role\":\"user\",\"content\":\"" << escaped_prompt << "\"}],"
         << "\"max_tokens\":200,"
         << "\"temperature\":0.8"
         << "}";

    std::string json_str = json.str();

    // Debug: print JSON (first 200 chars) to verify structure
    std::cout << "  -> OpenAI request JSON (first 200 chars): " << json_str.substr(0, 200) << std::endl;

    httplib::Headers headers = {
        {"Authorization", "Bearer " + api_key_},
        {"Content-Type", "application/json"},
        {"Accept", "application/json"}
    };

    // Enable SSL verification and set timeout
    cli.set_connection_timeout(30, 0);  // 30 seconds
    cli.set_read_timeout(30, 0);

    auto res = cli.Post(api_path_ + "/chat/completions", headers, json_str, "application/json");
    
    if (res && res->status == 200) {
        // Parse response to extract the message content
        // This is simplified - you may want to use a proper JSON parser
        // Look for "content" field in the response
        std::string response = res->body;
        
        // Simple extraction (for production, use nlohmann/json)
        size_t content_start = response.find("\"content\":\"");
        if (content_start != std::string::npos) {
            content_start += 11;  // Length of "\"content\":\""
            size_t content_end = response.find("\"", content_start);
            if (content_end != std::string::npos) {
                std::string content = response.substr(content_start, content_end - content_start);
                // Unescape
                std::string result;
                for (size_t i = 0; i < content.length(); ++i) {
                    if (content[i] == '\\' && i + 1 < content.length()) {
                        if (content[i+1] == 'n') { result += '\n'; ++i; }
                        else if (content[i+1] == 't') { result += '\t'; ++i; }
                        else if (content[i+1] == 'r') { result += '\r'; ++i; }
                        else { result += content[i]; }
                    } else {
                        result += content[i];
                    }
                }
                return result;
            }
        }
        return "Failed to parse OpenAI response";
    } else {
        std::cerr << "Error calling OpenAI API: ";
        if (res) {
            std::cerr << "Status " << res->status << std::endl;
            std::cerr << "Response body (first 500 chars): " << res->body.substr(0, 500) << std::endl;
        } else {
            std::cerr << "Connection failed - check your API key and network connection" << std::endl;
        }
        return "Error generating comment";
    }
}
