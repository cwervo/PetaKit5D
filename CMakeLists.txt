cmake_minimum_required(VERSION 3.16)
project(PetaKit5DViewer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(pk5d_core
    src/petakit5d_parser.cpp
)

target_include_directories(pk5d_core PUBLIC src)

add_executable(pk5d_cli src/main.cpp)
target_link_libraries(pk5d_cli PRIVATE pk5d_core)

include(CTest)
if(BUILD_TESTING)
    add_executable(pk5d_parser_tests tests/petakit5d_parser_tests.cpp)
    target_link_libraries(pk5d_parser_tests PRIVATE pk5d_core)
    add_test(NAME pk5d_parser_tests COMMAND pk5d_parser_tests)
endif()

if(EMSCRIPTEN)
    add_executable(petakit5d_viewer
        src/petakit5d_parser.cpp
        src/wasm_entry.cpp
    )
    target_include_directories(petakit5d_viewer PRIVATE src)
    set_target_properties(petakit5d_viewer PROPERTIES
        OUTPUT_NAME petakit5d_viewer
        SUFFIX ".js"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/dist"
    )
    target_link_options(petakit5d_viewer PRIVATE
        "-sEXPORTED_FUNCTIONS=['_pk5d_parse','_pk5d_parse_query','_malloc','_free']"
        "-sEXPORTED_RUNTIME_METHODS=['ccall','UTF8ToString']"
        "-sALLOW_MEMORY_GROWTH=1"
        "-sENVIRONMENT=web"
        "-sMODULARIZE=1"
        "-sEXPORT_ES6=1"
        "-sEXPORT_NAME=PetaKit5DModule"
    )

    add_custom_target(copy_web_assets ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/dist"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/web/index.html"
            "${CMAKE_SOURCE_DIR}/web/app.js"
            "${CMAKE_SOURCE_DIR}/web/styles.css"
            "${CMAKE_BINARY_DIR}/dist"
        DEPENDS petakit5d_viewer
    )
endif()
