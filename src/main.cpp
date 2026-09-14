#include "petakit5d_parser.h"

#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace {
std::string read_all(std::istream& input) {
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}
}

int main(int argc, char** argv) {
    if (argc > 2 && std::string(argv[1]) == "--query") {
        std::cout << pk5d::parse_result_to_json(pk5d::parse_query_payload(argv[2])) << '\n';
        return 0;
    }

    if (argc > 1) {
        std::ifstream file(argv[1]);
        if (!file) {
            std::cerr << "Unable to open input file: " << argv[1] << '\n';
            return 1;
        }
        std::cout << pk5d::parse_result_to_json(pk5d::parse_payload(read_all(file))) << '\n';
        return 0;
    }

    std::cout << pk5d::parse_result_to_json(pk5d::parse_payload(read_all(std::cin))) << '\n';
    return 0;
}
