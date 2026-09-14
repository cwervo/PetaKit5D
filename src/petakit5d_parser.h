#pragma once

#include <map>
#include <string>
#include <vector>

namespace pk5d {

struct TimelineClip {
    std::string label;
    std::string source;
    std::string note;
    int x = 0;
    int y = 0;
    int z = 0;
    int c = 0;
    int t = 0;
    double fps = 0.0;
    double ms_per_frame = 0.0;
};

struct ParseResult {
    std::vector<TimelineClip> clips;
    std::vector<std::string> warnings;
};

std::string trim(const std::string& value);
std::string url_decode(const std::string& value);
std::map<std::string, std::string> parse_query_string(const std::string& query);
ParseResult parse_payload(const std::string& input);
ParseResult parse_query_payload(const std::string& query);
std::string parse_result_to_json(const ParseResult& result);

}  // namespace pk5d
