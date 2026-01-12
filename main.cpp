#include <iostream>
#include <thread>
#include <chrono>
#include <unordered_set>
#include <vector>
#include <memory>
#include "config.h"
#include "leetify_client.h"
#include "discord_client.h"
#include "openai_client.h"
#include "match_data.h"
#include <nlohmann/json.hpp>
#include <optional>
using json = nlohmann::json;

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"

int main() {
    std::cout << "CounterStrike Helper - Starting..." << std::endl;
    
    // Load configuration
    Config config = Config::load_from_env();
    if (!config.is_valid()) {
        std::cerr << "Error: Invalid configuration. Please check your environment variables." << std::endl;
        return 1;
    }
    
    std::cout << "Configuration loaded successfully!" << std::endl;
    std::cout << "Tracking " << config.tracked_steam_ids.size() << " Steam ID(s)" << std::endl;
    std::cout << "Poll interval: " << config.poll_interval_seconds << " seconds" << std::endl;
    
    // Initialize clients
    LeetifyClient leetify_client(config.leetify_api_key);
    DiscordClient discord_client(config.discord_webhook_url);
    OpenAIClient openai_client(config.openai_api_key);
    
    // Store seen match IDs to avoid duplicates
    std::unordered_set<std::string> seen_match_ids;
    
    // Main polling loop
    std::cout << "Starting polling loop..." << std::endl;
    //while (true) {
        try {
            // Fetch matches for each tracked Steam ID
            for (const auto& steam_id : config.tracked_steam_ids) {
                std::cout << "Fetching recent match for Steam ID: " << steam_id << std::endl;
                
                // Fetch recent matches (last 5 to check for new ones)
                auto match = leetify_client.fetch_recent_match(steam_id);   

                if (match.match_id.empty()) {
                    std::cout << "  -> No match returned (fetch failed or no matches)\n";
                    continue;
                }
    
                    //std::cout << "Processing match: " << match.match_id << std::endl;
                    {
                    /*   

                    // Use the already-parsed match data (no extra API call)
                    MatchData detailed_match = match;
                    
                    // Generate comment using OpenAI
                    std::cout << "  -> Generating match comment..." << std::endl;
                    std::string comment = openai_client.generate_match_comment(detailed_match, config.tracked_steam_ids);
                    
                    // Check if comment generation failed
                    if (comment == "Error generating comment" || comment.empty()) {
                        std::cerr << "  -> ERROR: Failed to generate comment, skipping Discord send" << std::endl;
                        seen_match_ids.insert(detailed_match.match_id);  // Mark as seen to avoid retrying
                        continue;
                    }
                    
                    std::cout << "  -> Generated comment: " << comment.substr(0, 100) << "..." << std::endl;
                    
                    // Send to Discord
                    std::cout << "  -> Sending match report to Discord..." << std::endl;
                    bool success = discord_client.send_match_report(detailed_match, comment);
                    
                    if (success) {
                        std::cout << "  -> Match report sent successfully!" << std::endl;
                        // Mark as seen
                        seen_match_ids.insert(detailed_match.match_id);
                    } else {
                        std::cerr << "  -> ERROR: Failed to send match report to Discord" << std::endl;
                    }
                    
                    // Small delay between processing matches
                    std::this_thread::sleep_for(std::chrono::seconds(2));
                    */
                } 
            }
            
            std::cout << "Polling cycle complete. Waiting " << config.poll_interval_seconds 
                      << " seconds until next cycle..." << std::endl;
            
        } catch (const std::exception& e) {
            std::cerr << "Error in polling loop: " << e.what() << std::endl;
            // Send error notification to Discord
            discord_client.send_message("⚠️ Error in polling loop: " + std::string(e.what()));
        }
        
        // Wait before next poll
        std::this_thread::sleep_for(std::chrono::seconds(config.poll_interval_seconds));
    //}
    
    return 0;
}
