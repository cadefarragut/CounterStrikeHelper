#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "ai_client.h"
#include "httplib.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

AIClient::AIClient(const std::string& api_key) : api_key_(api_key) {}

std::string AIClient::generate_match_comment(const MatchData& match,
                                             const std::vector<std::string>& tracked_steam_ids) {
    std::string prompt = build_comment_prompt(match, tracked_steam_ids);
    return make_api_request(prompt);
}

std::string AIClient::generate_player_comment(const PlayerStats& player, const MatchData& match) {
    std::ostringstream prompt;
    prompt << "Write a funny, short comment (2-3 sentences max) about this CS2 match performance. ";
    prompt << "Be playful and humorous - make fun if they did bad, praise if they did good. ";
    prompt << "Player: " << player.name << "\n";
    prompt << "K/D: " << player.kills << "/" << player.deaths << "\n";
    prompt << "ADR: " << player.adr << "\n";
    prompt << "Headshot %: " << player.headshot_percentage << "%\n";
    prompt << "Match result: " << (player.won_match ? "Won" : "Lost") << "\n";
    prompt << "Map: " << match.map_name << "\n";
    prompt << "Keep it light-hearted and entertaining!";
    
    return make_api_request(prompt.str());
}

std::string AIClient::build_comment_prompt(const MatchData& match,
                                           const std::vector<std::string>& tracked_steam_ids) {
    std::ostringstream prompt;
    prompt << "You are a witty CS2 match commentator for a Discord server. ";
    prompt << "Write a funny, entertaining comment about this CS2 match. ";
    prompt << "Focus mainly on the TRACKED players but feel free to roast or mention enemies who dominated them, ";
    prompt << "or teammates who carried/fed. ";
    prompt << "Be playful and humorous - roast players who did bad, celebrate those who did well. ";
    prompt << "Keep it 2-3 sentences max. Be creative and specific about the stats!\n\n";
    
    prompt << "Match details:\n";
    prompt << "Map: " << match.map_name << "\n";
    prompt << "Score: " << match.get_score_string() << "\n\n";
    
    // Get tracked players
    auto tracked_players = match.get_tracked_players(tracked_steam_ids);
    
    // Figure out which team the tracked players are on
    int tracked_team = -1;
    if (!tracked_players.empty()) {
        tracked_team = tracked_players[0].team_number;
    }
    
    // Print tracked players first
    prompt << "=== TRACKED PLAYERS (the ones we care about) ===\n";
    for (const auto& player : tracked_players) {
        prompt << "- " << player.name << ": ";
        prompt << "K/D/A " << player.kills << "/" << player.deaths << "/" << player.assists;
        prompt << ", ADR " << player.adr;
        prompt << ", HS% " << player.headshot_percentage << "%";
        prompt << ", KD " << std::fixed << std::setprecision(2) << player.kd_ratio;
        prompt << " (" << (player.won_match ? "WON" : "LOST") << ")\n";
    }
    
    // Separate teammates and enemies
    std::vector<const PlayerStats*> teammates;
    std::vector<const PlayerStats*> enemies;
    
    for (const auto& player : match.players) {
        // Skip tracked players (already printed)
        bool is_tracked = false;
        for (const auto& id : tracked_steam_ids) {
            if (player.steam_id == id) {
                is_tracked = true;
                break;
            }
        }
        if (is_tracked) continue;
        
        if (player.team_number == tracked_team) {
            teammates.push_back(&player);
        } else {
            enemies.push_back(&player);
        }
    }
    
    // Print teammates
    if (!teammates.empty()) {
        prompt << "\n=== TEAMMATES ===\n";
        for (const auto* player : teammates) {
            prompt << "- " << player->name << ": ";
            prompt << "K/D/A " << player->kills << "/" << player->deaths << "/" << player->assists;
            prompt << ", ADR " << player->adr;
            prompt << ", KD " << std::fixed << std::setprecision(2) << player->kd_ratio << "\n";
        }
    }
    
    // Print enemies
    if (!enemies.empty()) {
        prompt << "\n=== ENEMIES ===\n";
        for (const auto* player : enemies) {
            prompt << "- " << player->name << ": ";
            prompt << "K/D/A " << player->kills << "/" << player->deaths << "/" << player->assists;
            prompt << ", ADR " << player->adr;
            prompt << ", KD " << std::fixed << std::setprecision(2) << player->kd_ratio << "\n";
        }
    }
    
    prompt << "\nWrite a witty comment (focus on tracked players but mention standout enemies/teammates if relevant):";
    return prompt.str();
}

std::string AIClient::make_api_request(const std::string& prompt) {
    // Use Groq API (free tier with Llama models)
    httplib::SSLClient cli("api.groq.com", 443);
    cli.set_connection_timeout(30, 0);
    cli.set_read_timeout(30, 0);

    // Build JSON request body (OpenAI-compatible format)
    json request_body;
    request_body["model"] = "llama-3.1-8b-instant";  // Fast, free model
    request_body["messages"] = json::array();
    
    json system_msg;
    system_msg["role"] = "system";
    system_msg["content"] = "You are a witty CS2 match commentator. Keep responses short, funny, and savage. Roast bad performances, hype good ones. Mention specific stats.";
    request_body["messages"].push_back(system_msg);
    
    json user_msg;
    user_msg["role"] = "user";
    user_msg["content"] = prompt;
    request_body["messages"].push_back(user_msg);
    
    request_body["max_tokens"] = 200;
    request_body["temperature"] = 0.9;

    std::string body = request_body.dump();

    // Debug output
    std::cout << "  -> Calling Groq API (Llama 3.1)...\n";

    httplib::Headers headers = {
        {"Content-Type", "application/json"},
        {"Authorization", "Bearer " + api_key_}
    };

    auto res = cli.Post("/openai/v1/chat/completions", headers, body, "application/json");
    
    if (!res) {
        std::cerr << "  -> Groq connection failed\n";
        return "Error generating comment";
    }
    
    std::cout << "  -> Response status: " << res->status << "\n";
    
    if (res->status != 200) {
        std::cerr << "  -> Groq API error: Status " << res->status << "\n";
        std::cerr << "  -> Response: " << res->body.substr(0, 500) << "\n";
        return "Error generating comment";
    }

    // Parse response (OpenAI-compatible format)
    try {
        json response = json::parse(res->body);
        
        if (response.contains("choices") && 
            response["choices"].is_array() && 
            !response["choices"].empty() &&
            response["choices"][0].contains("message") &&
            response["choices"][0]["message"].contains("content")) {
            
            std::string content = response["choices"][0]["message"]["content"].get<std::string>();
            std::cout << "  -> Got response from Groq!\n";
            return content;
        }
        
        std::cerr << "  -> Unexpected response format\n";
        return "Error generating comment";
        
    } catch (const json::exception& e) {
        std::cerr << "  -> JSON parse error: " << e.what() << "\n";
        return "Error generating comment";
    }
}