#include "config.h"
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <fstream>
#include <map>
#include <algorithm>

void Config::load_from_env_file(Config& config) {
    // Load .env file from project root
    std::string filename = "../.env";
    std::ifstream file(filename);

    std::map<std::string, std::string> env_values;
    std::string line;
    
    while (std::getline(file, line)) {
        // Find the equals sign
        size_t equals_pos = line.find('=');
        if (equals_pos == std::string::npos) {
            continue;  // Invalid line format
        }
        
        // Extract key and value
        std::string key = line.substr(0, equals_pos);
        std::string value = line.substr(equals_pos + 1);
        //std::cout << "Key: " << key << ", Value: " << value << std::endl;

        if(key == "DISCORD_WEBHOOK_URL") {
            config.discord_webhook_url = value;
        } else if(key == "LEETIFY_API_KEY") {
            config.leetify_api_key = value;
        } else if(key == "OPENAI_API_KEY") {
            config.openai_api_key = value;
        } else if(key == "TRACKED_STEAM_IDS") {
            parse_steam_ids(config, value);
        }
    }
    
    file.close();
}

void Config::parse_steam_ids(Config& config, const std::string& value) {
    // Parse comma-separated Steam IDs
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
    
    // Load configuration from .env file (searches multiple locations)
    load_from_env_file(config);
    std::cout << "Discord Webhook URL: " << config.discord_webhook_url << std::endl;
    std::cout << "Leetify API Key: " << config.leetify_api_key << std::endl;
    std::cout << "OpenAI API Key: " << config.openai_api_key << std::endl;
    std::cout << "Tracked Steam IDs: " << config.tracked_steam_ids.size() << std::endl;
    // Validate required fields
    if (config.discord_webhook_url.empty() || 
        config.leetify_api_key.empty() || 
        config.openai_api_key.empty() ||
        config.tracked_steam_ids.empty()) {
        std::cerr << "Error: Missing required configuration in .env file:" << std::endl;
        if (config.discord_webhook_url.empty()) std::cerr << "  - DISCORD_WEBHOOK_URL" << std::endl;
        if (config.leetify_api_key.empty()) std::cerr << "  - LEETIFY_API_KEY" << std::endl;
        if (config.openai_api_key.empty()) std::cerr << "  - OPENAI_API_KEY" << std::endl;
        if (config.tracked_steam_ids.empty()) std::cerr << "  - TRACKED_STEAM_IDS" << std::endl;
        std::cerr << "Please create a .env file in the project root with these values." << std::endl;
    }
    
    return config;
}

bool Config::is_valid() const {
    return !discord_webhook_url.empty() && 
           !leetify_api_key.empty() && 
           !openai_api_key.empty() &&
           !tracked_steam_ids.empty();
}
