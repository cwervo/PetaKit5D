#include "petakit5d_parser.h"

#include <string>

extern "C" {
const char* pk5d_parse(const char* input) {
    static std::string response;
    response = pk5d::parse_result_to_json(pk5d::parse_payload(input == nullptr ? "" : input));
    return response.c_str();
}

const char* pk5d_parse_query(const char* query) {
    static std::string response;
    response = pk5d::parse_result_to_json(pk5d::parse_query_payload(query == nullptr ? "" : query));
    return response.c_str();
}
}
