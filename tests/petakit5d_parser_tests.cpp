#include "petakit5d_parser.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

void test_byte_language() {
    const auto result = pk5d::parse_payload(
        "pk5d:label=Embryo A;source=https://example.org/a.txt;x=256;y=128;z=32;c=2;t=60;fps=30\n");
    require(result.clips.size() == 1, "byte language should produce one clip");
    const auto& clip = result.clips.front();
    require(clip.label == "Embryo A", "label parsed from byte language");
    require(clip.x == 256 && clip.y == 128 && clip.z == 32 && clip.c == 2 && clip.t == 60,
            "5D dimensions parsed from byte language");
    require(clip.fps == 30.0, "fps parsed from byte language");
}

void test_tcl_tag() {
    const auto result = pk5d::parse_payload(
        "Before [petakit5d label {Cell Track} src {https://example.org/b.md} x 96 y 64 z 12 c 3 t 48 ms 12.5] after");
    require(result.clips.size() == 1, "tcl tag should produce one clip");
    const auto& clip = result.clips.front();
    require(clip.label == "Cell Track", "label parsed from tcl tag");
    require(clip.source == "https://example.org/b.md", "source parsed from tcl tag");
    require(clip.ms_per_frame == 12.5, "ms-per-frame parsed from tcl tag");
}

void test_query_string() {
    const auto result = pk5d::parse_query_payload(
        "?q=pk5d%3Alabel%3DRemote%20Inline%3Bt%3D10%3Bz%3D5&s=%5Bpetakit5d%20label%20%7BInline%20Tag%7D%20t%2020%20c%202%5D");
    require(result.clips.size() == 2, "query string should parse q and s entries");
    require(result.clips[0].label == "Remote Inline", "decoded q clip label");
    require(result.clips[1].label == "Inline Tag", "decoded s clip label");
    require(result.clips[1].c == 2, "decoded s clip c dimension");
}

}  // namespace

int main() {
    test_byte_language();
    test_tcl_tag();
    test_query_string();
    std::cout << "All parser tests passed\n";
    return 0;
}
