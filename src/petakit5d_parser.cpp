#include "petakit5d_parser.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iomanip>
#include <map>
#include <sstream>
#include <stdexcept>

namespace pk5d {
namespace {

bool is_http_url(const std::string& value) {
    return value.rfind("http://", 0) == 0 || value.rfind("https://", 0) == 0;
}

std::string lower_copy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

bool parse_int(const std::string& text, int& target) {
    try {
        size_t index = 0;
        int value = std::stoi(text, &index);
        if (index != text.size()) {
            return false;
        }
        target = value;
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool parse_double(const std::string& text, double& target) {
    try {
        size_t index = 0;
        double value = std::stod(text, &index);
        if (index != text.size()) {
            return false;
        }
        target = value;
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

std::string json_escape(const std::string& input) {
    std::ostringstream stream;
    for (unsigned char ch : input) {
        switch (ch) {
            case '\\': stream << "\\\\"; break;
            case '"': stream << "\\\""; break;
            case '\n': stream << "\\n"; break;
            case '\r': stream << "\\r"; break;
            case '\t': stream << "\\t"; break;
            default:
                if (ch < 0x20) {
                    stream << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                           << static_cast<int>(ch) << std::dec;
                } else {
                    stream << ch;
                }
        }
    }
    return stream.str();
}

void assign_value(TimelineClip& clip, const std::string& raw_key, const std::string& value,
                  std::vector<std::string>& warnings) {
    const std::string key = lower_copy(trim(raw_key));
    if (key == "label" || key == "name" || key == "title") {
        clip.label = trim(url_decode(value));
    } else if (key == "source" || key == "src" || key == "url" || key == "link") {
        clip.source = trim(url_decode(value));
    } else if (key == "note" || key == "description") {
        clip.note = trim(url_decode(value));
    } else if (key == "x") {
        if (!parse_int(trim(value), clip.x)) {
            warnings.push_back("Invalid x dimension: " + value);
        }
    } else if (key == "y") {
        if (!parse_int(trim(value), clip.y)) {
            warnings.push_back("Invalid y dimension: " + value);
        }
    } else if (key == "z") {
        if (!parse_int(trim(value), clip.z)) {
            warnings.push_back("Invalid z dimension: " + value);
        }
    } else if (key == "c") {
        if (!parse_int(trim(value), clip.c)) {
            warnings.push_back("Invalid c dimension: " + value);
        }
    } else if (key == "t" || key == "frames") {
        if (!parse_int(trim(value), clip.t)) {
            warnings.push_back("Invalid t dimension: " + value);
        }
    } else if (key == "fps") {
        if (!parse_double(trim(value), clip.fps)) {
            warnings.push_back("Invalid fps value: " + value);
        }
    } else if (key == "ms" || key == "ms_per_frame") {
        if (!parse_double(trim(value), clip.ms_per_frame)) {
            warnings.push_back("Invalid ms-per-frame value: " + value);
        }
    }
}

bool finalize_clip(TimelineClip& clip) {
    if (clip.label.empty()) {
        clip.label = clip.source.empty() ? "PetaKit5D clip" : clip.source;
    }
    return !(clip.label.empty() && clip.source.empty() && clip.t == 0 && clip.z == 0 && clip.c == 0 &&
             clip.x == 0 && clip.y == 0 && clip.fps == 0.0 && clip.ms_per_frame == 0.0 && clip.note.empty());
}

std::vector<std::string> split(const std::string& input, char separator) {
    std::vector<std::string> parts;
    std::string current;
    std::istringstream stream(input);
    while (std::getline(stream, current, separator)) {
        parts.push_back(current);
    }
    return parts;
}

std::vector<std::string> tokenize_tclish(const std::string& input) {
    std::vector<std::string> tokens;
    std::string current;
    bool in_quotes = false;
    int brace_depth = 0;
    int bracket_depth = 0;
    for (size_t i = 0; i < input.size(); ++i) {
        const char ch = input[i];
        if (in_quotes) {
            if (ch == '"') {
                tokens.push_back(current);
                current.clear();
                in_quotes = false;
            } else {
                current.push_back(ch);
            }
            continue;
        }
        if (bracket_depth > 0) {
            current.push_back(ch);
            if (ch == '[') {
                ++bracket_depth;
            } else if (ch == ']') {
                --bracket_depth;
                if (bracket_depth == 0) {
                    tokens.push_back(current);
                    current.clear();
                }
            }
            continue;
        }
        if (brace_depth > 0) {
            if (ch == '{') {
                ++brace_depth;
                current.push_back(ch);
            } else if (ch == '}') {
                --brace_depth;
                if (brace_depth == 0) {
                    tokens.push_back(current);
                    current.clear();
                } else {
                    current.push_back(ch);
                }
            } else {
                current.push_back(ch);
            }
            continue;
        }
        if (std::isspace(static_cast<unsigned char>(ch))) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
            continue;
        }
        if (ch == '"') {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
            in_quotes = true;
            continue;
        }
        if (ch == '{') {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
            brace_depth = 1;
            continue;
        }
        if (ch == '[') {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
            bracket_depth = 1;
            current.push_back(ch);
            continue;
        }
        current.push_back(ch);
    }
    if (!current.empty()) {
        tokens.push_back(current);
    }
    return tokens;
}

void parse_byte_line(const std::string& line, ParseResult& result) {
    const auto separator = line.find(':');
    if (separator == std::string::npos) {
        return;
    }
    TimelineClip clip;
    for (const auto& pair : split(line.substr(separator + 1), ';')) {
        const auto equal = pair.find('=');
        if (equal == std::string::npos) {
            continue;
        }
        assign_value(clip, pair.substr(0, equal), pair.substr(equal + 1), result.warnings);
    }
    if (finalize_clip(clip)) {
        result.clips.push_back(clip);
    }
}

void parse_tcl_tag(const std::string& content, ParseResult& result) {
    const auto tokens = tokenize_tclish(content);
    if (tokens.empty() || lower_copy(tokens.front()) != "petakit5d") {
        return;
    }

    TimelineClip clip;
    for (size_t i = 1; i + 1 < tokens.size(); i += 2) {
        assign_value(clip, tokens[i], tokens[i + 1], result.warnings);
    }
    if ((tokens.size() % 2) == 0) {
        result.warnings.push_back("Ignoring dangling petakit5d key without value.");
    }
    if (finalize_clip(clip)) {
        result.clips.push_back(clip);
    }
}

void parse_inline_payload(const std::string& input, ParseResult& result) {
    std::istringstream stream(input);
    std::string line;
    while (std::getline(stream, line)) {
        const std::string trimmed = trim(line);
        const std::string lowered = lower_copy(trimmed);
        if (lowered.rfind("pk5d:", 0) == 0 || lowered.rfind("petakit5d:", 0) == 0) {
            parse_byte_line(trimmed, result);
        }
    }

    size_t start = 0;
    while ((start = input.find('[', start)) != std::string::npos) {
        bool in_quotes = false;
        int brace_depth = 0;
        int bracket_depth = 1;
        size_t end = std::string::npos;
        for (size_t cursor = start + 1; cursor < input.size(); ++cursor) {
            const char ch = input[cursor];
            if (in_quotes) {
                if (ch == '"') {
                    in_quotes = false;
                }
                continue;
            }
            if (brace_depth > 0) {
                if (ch == '{') {
                    ++brace_depth;
                } else if (ch == '}') {
                    --brace_depth;
                }
                continue;
            }
            if (ch == '"') {
                in_quotes = true;
                continue;
            }
            if (ch == '{') {
                brace_depth = 1;
                continue;
            }
            if (ch == '[') {
                ++bracket_depth;
                continue;
            }
            if (ch == ']') {
                --bracket_depth;
                if (bracket_depth == 0) {
                    end = cursor;
                    break;
                }
            }
        }
        if (end == std::string::npos) {
            break;
        }
        parse_tcl_tag(input.substr(start + 1, end - start - 1), result);
        start = end + 1;
    }
}

}  // namespace

std::string trim(const std::string& value) {
    size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start]))) {
        ++start;
    }
    size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
        --end;
    }
    return value.substr(start, end - start);
}

std::string url_decode(const std::string& value) {
    std::string output;
    output.reserve(value.size());
    for (size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '+' ) {
            output.push_back(' ');
            continue;
        }
        if (value[i] == '%' && i + 2 < value.size()) {
            const std::string hex = value.substr(i + 1, 2);
            char* end = nullptr;
            const long decoded = std::strtol(hex.c_str(), &end, 16);
            if (end != nullptr && *end == '\0') {
                output.push_back(static_cast<char>(decoded));
                i += 2;
                continue;
            }
        }
        output.push_back(value[i]);
    }
    return output;
}

std::map<std::string, std::string> parse_query_string(const std::string& query) {
    std::string sanitized = query;
    if (!sanitized.empty() && sanitized.front() == '?') {
        sanitized.erase(sanitized.begin());
    }

    std::map<std::string, std::string> values;
    for (const auto& pair : split(sanitized, '&')) {
        if (pair.empty()) {
            continue;
        }
        const auto equal = pair.find('=');
        const std::string key = url_decode(equal == std::string::npos ? pair : pair.substr(0, equal));
        const std::string value = url_decode(equal == std::string::npos ? std::string() : pair.substr(equal + 1));
        values[key] = value;
    }
    return values;
}

ParseResult parse_payload(const std::string& input) {
    ParseResult result;
    parse_inline_payload(input, result);
    return result;
}

ParseResult parse_query_payload(const std::string& query) {
    ParseResult result;
    const auto values = parse_query_string(query);

    const auto q_it = values.find("q");
    if (q_it != values.end() && !q_it->second.empty()) {
        if (is_http_url(q_it->second)) {
            TimelineClip clip;
            clip.label = "Remote PetaKit5D link";
            clip.source = q_it->second;
            clip.note = "Remote URLs are displayed as linked sources; upload the referenced .txt or .md file to parse it locally in the static viewer.";
            result.clips.push_back(clip);
        } else {
            ParseResult parsed = parse_payload(q_it->second);
            result.clips.insert(result.clips.end(), parsed.clips.begin(), parsed.clips.end());
            result.warnings.insert(result.warnings.end(), parsed.warnings.begin(), parsed.warnings.end());
        }
    }

    const auto s_it = values.find("s");
    if (s_it != values.end() && !s_it->second.empty()) {
        ParseResult parsed = parse_payload(s_it->second);
        result.clips.insert(result.clips.end(), parsed.clips.begin(), parsed.clips.end());
        result.warnings.insert(result.warnings.end(), parsed.warnings.begin(), parsed.warnings.end());
    }
    return result;
}

std::string parse_result_to_json(const ParseResult& result) {
    std::ostringstream json;
    json << "{\"clips\":[";
    for (size_t i = 0; i < result.clips.size(); ++i) {
        const auto& clip = result.clips[i];
        if (i > 0) {
            json << ',';
        }
        json << "{"
             << "\"label\":\"" << json_escape(clip.label) << "\"," 
             << "\"source\":\"" << json_escape(clip.source) << "\"," 
             << "\"note\":\"" << json_escape(clip.note) << "\"," 
             << "\"x\":" << clip.x << ','
             << "\"y\":" << clip.y << ','
             << "\"z\":" << clip.z << ','
             << "\"c\":" << clip.c << ','
             << "\"t\":" << clip.t << ','
             << "\"fps\":" << clip.fps << ','
             << "\"msPerFrame\":" << clip.ms_per_frame
             << "}";
    }
    json << "],\"warnings\":[";
    for (size_t i = 0; i < result.warnings.size(); ++i) {
        if (i > 0) {
            json << ',';
        }
        json << "\"" << json_escape(result.warnings[i]) << "\"";
    }
    json << "]}";
    return json.str();
}

}  // namespace pk5d
