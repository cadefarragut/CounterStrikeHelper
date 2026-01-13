#pragma once

#include <string>
#include <unordered_set>
#include <fstream>
#include <iostream>
#include <filesystem>

/**
 * Manages persistence of seen match IDs to avoid re-reporting matches after restart.
 * Stores data in a simple text file, one match ID per line.
 */
class PersistenceManager {
public:
    explicit PersistenceManager(const std::string& filepath = "seen_matches.txt")
        : filepath_(filepath) {}
    
    // Load seen matches from file
    bool load() {
        seen_match_ids_.clear();
        
        std::ifstream file(filepath_);
        if (!file.is_open()) {
            // File doesn't exist yet - that's OK for first run
            std::cout << "[persistence] No existing file found, starting fresh\n";
            return true;
        }
        
        std::string line;
        while (std::getline(file, line)) {
            if (!line.empty()) {
                seen_match_ids_.insert(line);
            }
        }
        
        std::cout << "[persistence] Loaded " << seen_match_ids_.size() << " seen match IDs\n";
        return true;
    }
    
    // Save seen matches to file
    bool save() const {
        std::ofstream file(filepath_);
        if (!file.is_open()) {
            std::cerr << "[persistence] Failed to open file for writing: " << filepath_ << "\n";
            return false;
        }
        
        for (const auto& match_id : seen_match_ids_) {
            file << match_id << "\n";
        }
        
        return file.good();
    }
    
    // Check if a match has been seen
    bool has_seen(const std::string& match_id) const {
        return seen_match_ids_.count(match_id) > 0;
    }
    
    // Mark a match as seen (does NOT auto-save)
    void mark_seen(const std::string& match_id) {
        seen_match_ids_.insert(match_id);
    }
    
    // Mark a match as seen and immediately persist
    bool mark_seen_and_save(const std::string& match_id) {
        mark_seen(match_id);
        return save();
    }
    
    // Get count of seen matches
    size_t size() const { return seen_match_ids_.size(); }
    
    // Clear all seen matches
    void clear() { seen_match_ids_.clear(); }
    
    // Get the most recently added match ID (for display purposes)
    std::string get_last_match_id() const {
        return last_added_;
    }

private:
    std::string filepath_;
    std::unordered_set<std::string> seen_match_ids_;
    std::string last_added_;
};
