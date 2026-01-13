#include "config.h"
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <fstream>
#include <map>
#include <algorithm>

void Config::load_from_env_file(Config& config) {
    // Try multiple locations for .env file
    std::vector<std::string> locations = {".env", "../.env", "../../.env"};
    
    std::ifstream file;
    for (const auto& loc : locations) {
        file.open(loc);
        if (file.is_open()) {
            std::cout << "[config] Found .env at: " << loc << "\n";
            break;
        }
    }
    
    if (!file.is_open()) {
        std::cerr << "[config] Could not find .env file in any location\n";
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') continue;
        
        // Find the equals sign
        size_t equals_pos = line.find('=');
        if (equals_pos == std::string::npos) continue;
        
        // Extract key and value
        std::string key = line.substr(0, equals_pos);
        std::string value = line.substr(equals_pos + 1);
        
        // Trim whitespace from key
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        
        // Trim whitespace and quotes from value
        value.erase(0, value.find_first_not_of(" \t\"'"));
        value.erase(value.find_last_not_of(" \t\"'\r\n") + 1);

        if (key == "DISCORD_WEBHOOK_URL") {
            config.discord_webhook_url = value;
        } else if (key == "LEETIFY_API_KEY") {
            config.leetify_api_key = value;
        } else if (key == "OPENAI_API_KEY" || key == "GEMINI_API_KEY" || key == "GROQ_API_KEY") {
            // Accept any of these AI API keys
            config.openai_api_key = value;
        } else if (key == "TRACKED_STEAM_IDS") {
            parse_steam_ids(config, value);
        } else if (key == "POLL_INTERVAL_SECONDS") {
            try {
                config.poll_interval_seconds = std::stoi(value);
            } catch (...) {
                std::cerr << "[config] Invalid POLL_INTERVAL_SECONDS, using default\n";
            }
        }
    }
    
    file.close();
}

void Config::parse_steam_ids(Config& config, const std::string& value) {
    std::istringstream iss(value);
    std::string id;
    
    while (std::getline(iss, id, ',')) {
        // Trim whitespace
        id.erase(0, id.find_first_not_of(" \t"));
        id.erase(id.find_last_not_of(" \t") + 1);
        
        if (!id.empty()) {
            config.tracked_steam_ids.push_back(id);
        }
    }
}

Config Config::load_from_env() {
    Config config;
    
    load_from_env_file(config);

    // Validate and report missing fields
    if (config.discord_webhook_url.empty() || 
        config.leetify_api_key.empty() || 
        config.openai_api_key.empty() ||
        config.tracked_steam_ids.empty()) {
        
        std::cerr << "[config] Missing required configuration:\n";
        if (config.discord_webhook_url.empty()) std::cerr << "  - DISCORD_WEBHOOK_URL\n";
        if (config.leetify_api_key.empty()) std::cerr << "  - LEETIFY_API_KEY\n";
        if (config.openai_api_key.empty()) std::cerr << "  - GROQ_API_KEY (free at console.groq.com)\n";
        if (config.tracked_steam_ids.empty()) std::cerr << "  - TRACKED_STEAM_IDS\n";
    }
    
    return config;
}

bool Config::is_valid() const {
    return !discord_webhook_url.empty() && 
           !leetify_api_key.empty() && 
           !openai_api_key.empty() &&
           !tracked_steam_ids.empty();
}