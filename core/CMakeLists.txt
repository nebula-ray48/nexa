set(PARSER_SRC ${CMAKE_SOURCE_DIR}/tree-sitter-nexa/src/parser.c
        include/nexa/compiler.h
        include/nexa/registry.h
        src/analyzer.cpp
        include/nexa/analyzer.h
        include/nexa/type_system.h)

add_library(nexa-core STATIC
    ${PARSER_SRC}
    src/compiler.cpp
    ${tree-sitter_SOURCE_DIR}/lib/src/lib.c
)

target_include_directories(nexa-core PUBLIC
    include
    ${tree-sitter_SOURCE_DIR}/lib/include
)

set_nexa_warnings(nexa-core)