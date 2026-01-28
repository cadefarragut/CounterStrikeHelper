#include <iostream>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>
#include <map>

#include "config.h"
#include "leetify_client.h"
#include "discord_client.h"
#include "ai_client.h"
#include "match_data.h"
#include "persistence.h"

// Global flag for graceful shutdown (Ctrl+C)
std::atomic<bool> g_running{true};

void signal_handler(int signal) {
    std::cout << "\n[main] Received signal " << signal << ", shutting down...\n";
    g_running = false;
}

void print_banner() {
    std::cout << R"(
   ____  ____  ____    _   _      _                 
  / ___|/ ___|/ ___|  | | | | ___| |_ __   ___ _ __ 
 | |    \___ \\___ \  | |_| |/ _ \ | '_ \ / _ \ '__|
 | |___  ___) |___) | |  _  |  __/ | |_) |  __/ |   
  \____||____/____/  |_| |_|\___|_| .__/ \___|_|   
                                  |_|               
)" << '\n';
    std::cout << "Counter-Strike 2 Match Tracker & Discord Reporter\n";
    std::cout << "=================================================\n\n";
}

int main() {
    print_banner();
    
    // Setup signal handler for graceful shutdown
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    
    // Load configuration
    Config config = Config::load_from_env();
    if (!config.is_valid()) {
        std::cerr << "Error: Invalid configuration. Please check your .env file.\n";
        return 1;
    }
    
    std::cout << "Configuration loaded successfully!\n";
    std::cout << "Tracking " << config.tracked_steam_ids.size() << " Steam ID(s):\n";
    for (const auto& id : config.tracked_steam_ids) {
        std::cout << "  - " << id << "\n";
    }
    std::cout << "Poll interval: " << config.poll_interval_seconds << " seconds\n\n";
    
    // Initialize clients
    LeetifyClient leetify_client(config.leetify_api_key);
    DiscordClient discord_client(config.discord_webhook_url);
    OpenAIClient openai_client(config.openai_api_key);
    
    // Load persistence (remembered match IDs)
    PersistenceManager persistence("seen_matches.txt");
    persistence.load();

    // Silent initialization: if this is a fresh start (no seen matches),
    // mark current matches as seen without posting to avoid stale match spam
    if (persistence.is_empty()) {
        std::cout << "[init] First run detected - performing silent initialization...\n";
        std::cout << "[init] Marking current matches as seen (won't post stale matches)\n";

        for (const auto& steam_id : config.tracked_steam_ids) {
            std::cout << "[init] Checking current match for Steam ID: " << steam_id << "\n";
            MatchData match = leetify_client.fetch_recent_match(steam_id);

            if (!match.match_id.empty()) {
                std::cout << "[init]   -> Marking match " << match.match_id << " as seen\n";
                persistence.mark_seen_and_save(match.match_id);
            } else {
                std::cout << "[init]   -> No match found for this player\n";
            }
        }

        std::cout << "[init] Silent initialization complete. Only new matches will be posted.\n\n";
    }

    // Send startup message to Discord
    discord_client.send_message("🎮 CS2 Match Tracker is now online! Monitoring " +
                                std::to_string(config.tracked_steam_ids.size()) + " player(s).");

    std::cout << "Starting polling loop (Ctrl+C to stop)...\n\n";
    
    // Main polling loop
    while (g_running) {
        try {
            // Phase 1: Collect all new matches from all tracked players
            // Use map to deduplicate by match_id
            std::map<std::string, MatchData> new_matches;

            for (const auto& steam_id : config.tracked_steam_ids) {
                if (!g_running) break;

                std::cout << "[poll] Checking for new matches for Steam ID: " << steam_id << "\n";

                // Fetch the most recent match
                MatchData match = leetify_client.fetch_recent_match(steam_id);

                if (match.match_id.empty()) {
                    std::cout << "  -> No match found or fetch failed\n";
                    continue;
                }

                // Check if we've already processed this match
                if (persistence.has_seen(match.match_id)) {
                    std::cout << "  -> Match " << match.match_id << " already processed, skipping\n";
                    continue;
                }

                // Check if we already collected this match from another player
                if (new_matches.find(match.match_id) != new_matches.end()) {
                    std::cout << "  -> Match " << match.match_id << " already collected from another player\n";
                    continue;
                }

                // New match found - add to collection
                std::cout << "  -> New match found: " << match.match_id << " on " << match.map_name << "\n";
                new_matches[match.match_id] = match;
            }

            // Phase 2: Process each unique new match
            for (auto& [match_id, match] : new_matches) {
                if (!g_running) break;

                std::cout << "\n*** PROCESSING NEW MATCH ***\n";
                std::cout << "  Match ID: " << match.match_id << "\n";
                std::cout << "  Map: " << match.map_name << "\n";
                std::cout << "  Score: " << match.get_score_string() << "\n";

                // Get ALL tracked players' stats from this match
                auto tracked_players = match.get_tracked_players(config.tracked_steam_ids);

                if (tracked_players.empty()) {
                    std::cout << "  -> No tracked players found in match (unexpected)\n";
                    persistence.mark_seen_and_save(match.match_id);
                    continue;
                }

                // Print tracked player stats
                std::cout << "  Tracked players in this match: " << tracked_players.size() << "\n";
                for (const auto& player : tracked_players) {
                    std::cout << "    " << player.name
                              << " - K/D/A: " << player.kills << "/" << player.deaths << "/" << player.assists
                              << " | ADR: " << player.adr
                              << " | HS%: " << player.headshot_percentage << "%"
                              << " | " << (player.won_match ? "WIN" : "LOSS") << "\n";
                }

                // Generate AI comments for all tracked players
                std::cout << "\n  -> Generating AI commentary for " << tracked_players.size() << " player(s)...\n";

                std::map<std::string, std::string> player_comments;
                bool use_multi_player_report = tracked_players.size() > 1;

                if (use_multi_player_report) {
                    // Multiple tracked players - generate individual comments
                    player_comments = openai_client.generate_multi_player_comments(match, tracked_players);

                    // Check if generation worked, fallback if needed
                    for (const auto& player : tracked_players) {
                        auto it = player_comments.find(player.steam_id);
                        if (it == player_comments.end() || it->second.empty() ||
                            it->second == "Error generating comment") {
                            std::cerr << "  -> Fallback comment for " << player.name << "\n";
                            player_comments[player.steam_id] = player.name + " went " +
                                std::to_string(player.kills) + "/" + std::to_string(player.deaths) +
                                ". " + (player.won_match ? "At least they won!" : "Rough game.");
                        }
                    }

                    // Print comments
                    for (const auto& player : tracked_players) {
                        std::cout << "  -> " << player.name << ": " << player_comments[player.steam_id] << "\n";
                    }

                    // Send multi-player report to Discord
                    std::cout << "  -> Sending multi-player report to Discord...\n";
                    bool discord_success = discord_client.send_multi_player_report(match, tracked_players, player_comments);

                    if (discord_success) {
                        std::cout << "  -> Successfully posted to Discord!\n";
                    } else {
                        std::cerr << "  -> Failed to post to Discord\n";
                    }
                } else {
                    // Single tracked player - use original flow
                    std::string comment = openai_client.generate_match_comment(match, config.tracked_steam_ids);

                    if (comment.empty() || comment == "Error generating comment") {
                        std::cerr << "  -> Failed to generate AI comment, using fallback\n";
                        const auto& p = tracked_players[0];
                        comment = p.name + " just finished a game on " + match.map_name +
                                  " with a " + std::to_string(p.kills) + "/" + std::to_string(p.deaths) +
                                  " K/D. " + (p.won_match ? "They won!" : "Tough loss.");
                    }

                    std::cout << "  -> AI Comment: " << comment << "\n";

                    // Send to Discord
                    std::cout << "  -> Sending to Discord...\n";
                    bool discord_success = discord_client.send_match_report(match, comment);

                    if (discord_success) {
                        std::cout << "  -> Successfully posted to Discord!\n";
                    } else {
                        std::cerr << "  -> Failed to post to Discord\n";
                    }
                }

                // Mark match as seen (so we don't process it again)
                persistence.mark_seen_and_save(match.match_id);
                std::cout << "  -> Match marked as processed\n\n";

                // Small delay between processing matches
                std::this_thread::sleep_for(std::chrono::seconds(2));
            }

        } catch (const std::exception& e) {
            std::cerr << "[error] Exception in polling loop: " << e.what() << "\n";
            discord_client.send_message("⚠️ Error in match tracker: " + std::string(e.what()));
        }

        // Wait before next poll (interruptible for quick shutdown)
        std::cout << "[poll] Waiting " << config.poll_interval_seconds << " seconds until next check...\n";
        for (int i = 0; i < config.poll_interval_seconds && g_running; ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    
    // Cleanup
    std::cout << "\n[main] Saving state and shutting down...\n";
    persistence.save();
    discord_client.send_message("👋 CS2 Match Tracker is going offline.");
    
    std::cout << "[main] Goodbye!\n";
    return 0;
}
