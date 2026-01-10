#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include <map>

struct Config {
    // API Keys
    std::string discord_webhook_url;
    std::string leetify_api_key;
    std::string openai_api_key;
    
    // Steam IDs to track
    std::vector<std::string> tracked_steam_ids;
    
    // Polling settings
    int poll_interval_seconds = 60;  // Default: poll every 60 seconds
    
    // OpenAI settings
    std::string openai_model = "gpt-3.5-turbo";
    
    // Load configuration from .env file
    static Config load_from_env();
    
    // Validate configuration
    bool is_valid() const;
    
private:
    // Helper to parse .env file (checks ../.env first, then .env)
    static void load_from_env_file(Config& config);
    
    // Helper to parse Steam IDs (comma-separated format)
    static void parse_steam_ids(Config& config, const std::string& value);
};
