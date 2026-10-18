cmake_minimum_required(VERSION 3.10)
project(SearchEngine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

# Core Search Engine Library
add_library(search_engine_core
    src/tokenizer.cpp
    src/index_writer.cpp
    src/index_reader.cpp
    src/boolean_retriever.cpp
    src/phrase_matcher.cpp
    src/query_parser.cpp
    src/bm25_ranker.cpp
    src/trie.cpp
    src/manifest.cpp
    src/engine.cpp
)

# Interactive Search Shell CLI
add_executable(search_engine src/main.cpp)
target_link_libraries(search_engine PRIVATE search_engine_core)

# 25,000+ Document Benchmark Runner
add_executable(benchmark_runner benchmarks/benchmark_runner.cpp)
target_link_libraries(benchmark_runner PRIVATE search_engine_core)

# Unit Test Executables
enable_testing()

add_executable(test_tokenizer tests/test_tokenizer.cpp src/tokenizer.cpp)
add_test(NAME test_tokenizer COMMAND test_tokenizer)

add_executable(test_index tests/test_index.cpp 
    src/tokenizer.cpp src/index_writer.cpp src/index_reader.cpp)
add_test(NAME test_index COMMAND test_index)

add_executable(test_boolean tests/test_boolean.cpp 
    src/tokenizer.cpp src/index_writer.cpp src/index_reader.cpp
    src/boolean_retriever.cpp src/phrase_matcher.cpp src/query_parser.cpp)
add_test(NAME test_boolean COMMAND test_boolean)

add_executable(test_bm25 tests/test_bm25.cpp 
    src/tokenizer.cpp src/index_writer.cpp src/index_reader.cpp src/bm25_ranker.cpp)
add_test(NAME test_bm25 COMMAND test_bm25)

add_executable(test_trie tests/test_trie.cpp src/trie.cpp)
add_test(NAME test_trie COMMAND test_trie)

add_executable(test_incremental tests/test_incremental.cpp 
    src/tokenizer.cpp src/index_writer.cpp src/index_reader.cpp 
    src/manifest.cpp src/boolean_retriever.cpp)
add_test(NAME test_incremental COMMAND test_incremental)
