# Multi-stage Dockerfile for High-Performance C++ Search Engine
FROM gcc:12-bookworm AS builder

WORKDIR /app

# Copy source files
COPY include/ ./include/
COPY src/ ./src/
COPY web/ ./web/
COPY CMakeLists.txt ./

# Compile with modern C++17 and -O3 optimizations
RUN g++ -std=c++17 -O3 -Iinclude \
    src/tokenizer.cpp \
    src/index_writer.cpp \
    src/index_reader.cpp \
    src/boolean_retriever.cpp \
    src/phrase_matcher.cpp \
    src/query_parser.cpp \
    src/bm25_ranker.cpp \
    src/trie.cpp \
    src/manifest.cpp \
    src/engine.cpp \
    src/server.cpp \
    -lpthread \
    -o /app/server

# Lightweight runtime stage
FROM debian:bookworm-slim

WORKDIR /app

COPY --from=builder /app/server /app/server
COPY web/ /app/web/

ENV PORT=8080
EXPOSE 8080

CMD ["/app/server"]
