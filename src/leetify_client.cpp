#include "leetify_client.h"
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

LeetifyClient::LeetifyClient(const std::string& api_key) : api_key_(api_key) {}

MatchData LeetifyClient::fetch_recent_match(const std::string& steam64_id) {
    httplib::Client cli(base_url_);
    cli.set_connection_timeout(30, 0);
    cli.set_read_timeout(30, 0);

    std::string path = "/v3/profile/matches?steam64_id=" + steam64_id;

    httplib::Headers headers = {{"x-api-key", api_key_}};

    auto res = cli.Get(path, headers);
    if (!res) {
        std::cerr << "[leetify] Connection failed\n";
        return {};
    }
    if (res->status != 200) {
        std::cerr << "[leetify] Error fetching matches list. Status " << res->status << "\n";
        std::cerr << "[leetify] Response: " << res->body.substr(0, 200) << "\n";
        return {};
    }

    std::string recent_match_id = parse_most_recent_match_id(res->body);

    if (recent_match_id.empty()) {
        std::cerr << "[leetify] No matches found\n";
        return {};
    }

    std::cout << "[leetify] Most recent match ID: " << recent_match_id << "\n";
    return fetch_match_details(recent_match_id);
}

MatchData LeetifyClient::fetch_match_details(const std::string& match_id) {
    MatchData match;
    
    httplib::Client cli(base_url_);
    cli.set_connection_timeout(30, 0);
    cli.set_read_timeout(30, 0);
    
    std::string path = "/v2/matches/" + match_id;
    
    httplib::Headers headers = {{"x-api-key", api_key_}};
    
    auto res = cli.Get(path, headers);
    
    if (res && res->status == 200) {
        match = parse_match_details_from_json(res->body, match_id);
        std::cout << "[leetify] Parsed " << match.players.size() << " players\n";
    } else {
        std::cerr << "[leetify] Error fetching match details: ";
        if (res) {
            std::cerr << "Status " << res->status << "\n";
        } else {
            std::cerr << "Connection failed\n";
        }
    }
    return match;
}

bool LeetifyClient::is_valid() const {
    return !api_key_.empty();
}
