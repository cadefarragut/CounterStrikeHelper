#include "leetify_client.h"
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"
#include <iostream>

LeetifyClient::LeetifyClient(const std::string& api_key) : api_key_(api_key) {}

std::vector<MatchData> LeetifyClient::fetch_matches(const std::string& steam64_id, int limit) {
    std::vector<MatchData> matches;
    
    httplib::Client cli(base_url_);
    
    std::string path = "/v3/profile/matches?steam64_id=" + steam64_id;
    if (limit > 0) {
        path += "&limit=" + std::to_string(limit);
    }
    
    httplib::Headers headers = {
        {"x-api-key", api_key_}
    };
    
    auto res = cli.Get(path, headers);
    
    if (res && res->status == 200) {
        // Debug: print first 500 chars of response to see structure
        std::cout << "Leetify API response (first 500 chars): " << res->body.substr(0, 500) << std::endl;
        matches = parse_matches_list_from_json(res->body);
        std::cout << "Parsed " << matches.size() << " matches" << std::endl;
    } else {
        std::cerr << "Error fetching matches from Leetify: ";
        if (res) {
            std::cerr << "Status " << res->status << std::endl;
            std::cerr << "Response body: " << res->body.substr(0, 200) << std::endl;
        } else {
            std::cerr << "Connection failed" << std::endl;
        }
    }
    
    return matches;
}

MatchData LeetifyClient::fetch_match_details(const std::string& match_id) {
    MatchData match;
    
    httplib::Client cli(base_url_);
    
    std::string path = "/v3/matches/" + match_id;
    
    httplib::Headers headers = {
        {"x-api-key", api_key_}
    };
    
    auto res = cli.Get(path, headers);
    
    if (res && res->status == 200) {
        match = parse_match_from_json(res->body, match_id);
    } else {
        std::cerr << "Error fetching match details from Leetify: ";
        if (res) {
            std::cerr << "Status " << res->status << std::endl;
        } else {
            std::cerr << "Connection failed" << std::endl;
        }
    }
    
    return match;
}

bool LeetifyClient::is_valid() const {
    return !api_key_.empty();
}
