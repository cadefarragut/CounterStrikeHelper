#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "ai_client.h"
#include "httplib.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <regex>

using json = nlohmann::json;

AIClient::AIClient(const std::string& api_key) : api_key_(api_key) {}

std::string AIClient::generate_match_comment(const MatchData& match,
                                             const std::vector<std::string>& tracked_steam_ids) {
    std::string prompt = build_comment_prompt(match, tracked_steam_ids);
    return make_api_request(prompt);
}

std::map<std::string, std::string> AIClient::generate_multi_player_comments(
    const MatchData& match,
    const std::vector<PlayerStats>& tracked_players) {

    std::map<std::string, std::string> result;

    if (tracked_players.empty()) {
        return result;
    }

    // If only one tracked player, use the simpler single-player method
    if (tracked_players.size() == 1) {
        std::string comment = generate_player_comment(tracked_players[0], match);
        result[tracked_players[0].steam_id] = comment;
        return result;
    }

    // Build prompt for multiple players
    std::string prompt = build_multi_player_prompt(match, tracked_players);
    std::string response = make_api_request(prompt);

    // Parse the response - expect format like:
    // [PlayerName1]: comment here
    // [PlayerName2]: comment here
    for (const auto& player : tracked_players) {
        // Try to find this player's section in the response
        std::string pattern = "\\[" + player.name + "\\]:?\\s*";
        std::regex player_regex(pattern, std::regex::icase);
        std::smatch match_result;

        if (std::regex_search(response, match_result, player_regex)) {
            // Find where this player's comment starts
            size_t start = match_result.position() + match_result.length();
            // Find where the next player's comment starts (or end of string)
            size_t end = response.length();

            for (const auto& other : tracked_players) {
                if (other.steam_id == player.steam_id) continue;
                std::string other_pattern = "\\[" + other.name + "\\]:?";
                std::regex other_regex(other_pattern, std::regex::icase);
                std::smatch other_match;
                std::string remaining = response.substr(start);
                if (std::regex_search(remaining, other_match, other_regex)) {
                    size_t other_pos = start + other_match.position();
                    if (other_pos < end) {
                        end = other_pos;
                    }
                }
            }

            std::string comment = response.substr(start, end - start);
            // Trim whitespace and newlines
            size_t first = comment.find_first_not_of(" \n\r\t");
            size_t last = comment.find_last_not_of(" \n\r\t");
            if (first != std::string::npos && last != std::string::npos) {
                comment = comment.substr(first, last - first + 1);
            }
            result[player.steam_id] = comment;
        }
    }

    // Fallback: if parsing failed for any player, generate individual comments
    for (const auto& player : tracked_players) {
        if (result.find(player.steam_id) == result.end() || result[player.steam_id].empty()) {
            std::cout << "  -> Fallback: generating individual comment for " << player.name << "\n";
            result[player.steam_id] = generate_player_comment(player, match);
        }
    }

    return result;
}

std::string AIClient::build_multi_player_prompt(const MatchData& match,
                                                const std::vector<PlayerStats>& tracked_players) {
    std::ostringstream prompt;
    prompt << "You are a SAVAGE CS2 match commentator for a Discord server full of friends who roast each other. ";
    prompt << "Write SHORT but BRUTAL comments for EACH of the tracked players below. ";
    prompt << "If someone did BAD: be absolutely ruthless - question their skill, mock their stats, suggest they uninstall, ";
    prompt << "compare them to bots, say they were boosted, imply they were AFK, be creative and MEAN. ";
    prompt << "If someone did GOOD: go full simp mode - call them a god, say they hard carried, worship their aim, ";
    prompt << "say the enemies should uninstall, glaze them like they're the next s1mple. ";
    prompt << "Keep each comment to 1-2 sentences. Reference specific stats to make the roasts/praise hit harder!\n\n";

    prompt << "IMPORTANT: Format your response EXACTLY like this, with each player on their own line:\n";
    for (const auto& player : tracked_players) {
        prompt << "[" << player.name << "]: (your comment here)\n";
    }
    prompt << "\n";

    prompt << "Match details:\n";
    prompt << "Map: " << match.map_name << "\n";
    prompt << "Score: " << match.get_score_string() << "\n\n";

    prompt << "=== TRACKED PLAYERS ===\n";
    for (const auto& player : tracked_players) {
        prompt << "- " << player.name << ": ";
        prompt << "K/D/A " << player.kills << "/" << player.deaths << "/" << player.assists;
        prompt << ", ADR " << player.adr;
        prompt << ", HS% " << player.headshot_percentage << "%";
        prompt << ", KD " << std::fixed << std::setprecision(2) << player.kd_ratio;
        prompt << " (" << (player.won_match ? "WON" : "LOST") << ")\n";
    }

    // Add context about other players
    std::vector<const PlayerStats*> others;
    for (const auto& player : match.players) {
        bool is_tracked = false;
        for (const auto& tp : tracked_players) {
            if (player.steam_id == tp.steam_id) {
                is_tracked = true;
                break;
            }
        }
        if (!is_tracked) {
            others.push_back(&player);
        }
    }

    if (!others.empty()) {
        prompt << "\n=== OTHER PLAYERS (for context) ===\n";
        for (const auto* player : others) {
            prompt << "- " << player->name << ": ";
            prompt << "K/D/A " << player->kills << "/" << player->deaths << "/" << player->assists;
            prompt << ", ADR " << player->adr << "\n";
        }
    }

    prompt << "\nNow write a short, witty comment for each tracked player:";
    return prompt.str();
}

std::string AIClient::generate_player_comment(const PlayerStats& player, const MatchData& match) {
    std::ostringstream prompt;
    prompt << "Write a SAVAGE comment (1-2 sentences) about this CS2 match performance. ";
    prompt << "If they did BAD: be absolutely BRUTAL - mock their stats, question if they were AFK, ";
    prompt << "suggest they uninstall, say they got carried, compare them to Silver players. Be MEAN. ";
    prompt << "If they did GOOD: worship them like a god - full simp mode, call them insane, ";
    prompt << "say they hard carried, compare them to pro players, glaze them hard. ";
    prompt << "Player: " << player.name << "\n";
    prompt << "K/D: " << player.kills << "/" << player.deaths << "\n";
    prompt << "ADR: " << player.adr << "\n";
    prompt << "Headshot %: " << player.headshot_percentage << "%\n";
    prompt << "Match result: " << (player.won_match ? "Won" : "Lost") << "\n";
    prompt << "Map: " << match.map_name << "\n";
    prompt << "Reference their specific stats to make it hit harder!";

    return make_api_request(prompt.str());
}

std::string AIClient::build_comment_prompt(const MatchData& match,
                                           const std::vector<std::string>& tracked_steam_ids) {
    std::ostringstream prompt;
    prompt << "You are a SAVAGE CS2 match commentator for a Discord server full of friends who love roasting each other. ";
    prompt << "Write a BRUTAL or WORSHIPING comment about this CS2 match. ";
    prompt << "Focus mainly on the TRACKED players but feel free to roast enemies who dominated them, ";
    prompt << "or mock teammates who fed. ";
    prompt << "If they did BAD: be absolutely ruthless - question their existence, mock their pathetic stats, ";
    prompt << "suggest they go back to Valorant, imply they were boosted or AFK. Be MEAN and creative. ";
    prompt << "If they did GOOD: full simp mode - call them a literal god, say they carried their trash teammates, ";
    prompt << "compare them to pro players, worship their aim. ";
    prompt << "Keep it 2-3 sentences max. Reference specific stats to make it hit harder!\n\n";
    
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
    system_msg["content"] = "You are a BRUTAL CS2 match commentator for a Discord of friends who roast each other mercilessly. Be absolutely SAVAGE to bad performers - mock them, question their skill, suggest they uninstall, be creative and MEAN. For good performers, go full simp - worship them like gods, glaze them hard, say they carried. Always reference specific stats. Keep it short but devastating or worshiping.";
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