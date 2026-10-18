#include "engine.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <chrono>
#ifndef _WIN32
  #include <thread>
#endif
#include <cstring>
#include <cstdlib>
#include <fstream>

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")
  using socket_t = SOCKET;
  #define CLOSE_SOCKET(s) closesocket(s)
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <unistd.h>
  using socket_t = int;
  #define CLOSE_SOCKET(s) close(s)
  constexpr socket_t INVALID_SOCKET = -1;
  constexpr int SOCKET_ERROR = -1;
#endif

// Decodes percent-encoded URL parameters (e.g. %20 -> space, %22 -> ")
std::string url_decode(const std::string& in) {
    std::string out;
    out.reserve(in.size());
    for (size_t i = 0; i < in.size(); ++i) {
        if (in[i] == '%' && i + 2 < in.size()) {
            int val = 0;
            std::istringstream is(in.substr(i + 1, 2));
            if (is >> std::hex >> val) {
                out += static_cast<char>(val);
                i += 2;
            } else {
                out += in[i];
            }
        } else if (in[i] == '+') {
            out += ' ';
        } else {
            out += in[i];
        }
    }
    return out;
}

// Escapes JSON string characters
std::string json_escape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 10);
    for (char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 32) {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else {
                    out += c;
                }
        }
    }
    return out;
}

void handle_client(socket_t client_sock, SearchEngine* engine, const std::string& web_root) {
    char buffer[4096];
    int bytes_received = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        CLOSE_SOCKET(client_sock);
        return;
    }
    buffer[bytes_received] = '\0';

    std::string request(buffer);
    std::istringstream req_stream(request);
    std::string method, full_path, protocol;
    req_stream >> method >> full_path >> protocol;

    if (method != "GET") {
        std::string response = "HTTP/1.1 405 Method Not Allowed\r\nContent-Length: 0\r\n\r\n";
        send(client_sock, response.data(), static_cast<int>(response.size()), 0);
        CLOSE_SOCKET(client_sock);
        return;
    }

    std::string path = full_path;
    std::string query_params;
    size_t q_pos = full_path.find('?');
    if (q_pos != std::string::npos) {
        path = full_path.substr(0, q_pos);
        query_params = full_path.substr(q_pos + 1);
    }

    // Route 1: Serve UI (GET / or /index.html)
    if (path == "/" || path == "/index.html") {
        std::string html_path = web_root + "/index.html";
        std::ifstream file(html_path, std::ios::binary);
        if (!file.is_open()) {
            std::string not_found = "HTTP/1.1 404 Not Found\r\nContent-Length: 13\r\n\r\nFile Not Found";
            send(client_sock, not_found.data(), static_cast<int>(not_found.size()), 0);
            CLOSE_SOCKET(client_sock);
            return;
        }

        std::string content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
        std::string header = "HTTP/1.1 200 OK\r\n"
                             "Content-Type: text/html; charset=utf-8\r\n"
                             "Access-Control-Allow-Origin: *\r\n"
                             "Content-Length: " + std::to_string(content.size()) + "\r\n\r\n";
        send(client_sock, header.data(), static_cast<int>(header.size()), 0);
        send(client_sock, content.data(), static_cast<int>(content.size()), 0);
        CLOSE_SOCKET(client_sock);
        return;
    }

    // Route 2: Autocomplete API (GET /api/autocomplete?prefix=...)
    if (path == "/api/autocomplete") {
        std::string prefix;
        size_t p_pos = query_params.find("prefix=");
        if (p_pos != std::string::npos) {
            size_t end_p = query_params.find('&', p_pos);
            prefix = query_params.substr(p_pos + 7, (end_p == std::string::npos ? query_params.size() : end_p) - (p_pos + 7));
            prefix = url_decode(prefix);
        }

        auto suggestions = engine->autocomplete(prefix, 5);

        std::string json = "{\"prefix\":\"" + json_escape(prefix) + "\",\"suggestions\":[";
        for (size_t i = 0; i < suggestions.size(); ++i) {
            json += "\"" + json_escape(suggestions[i]) + "\"";
            if (i + 1 < suggestions.size()) json += ",";
        }
        json += "]}";

        std::string header = "HTTP/1.1 200 OK\r\n"
                             "Content-Type: application/json; charset=utf-8\r\n"
                             "Access-Control-Allow-Origin: *\r\n"
                             "Content-Length: " + std::to_string(json.size()) + "\r\n\r\n";
        send(client_sock, header.data(), static_cast<int>(header.size()), 0);
        send(client_sock, json.data(), static_cast<int>(json.size()), 0);
        CLOSE_SOCKET(client_sock);
        return;
    }

    // Route 3: Search API (GET /api/search?q=...&k=...)
    if (path == "/api/search") {
        std::string query_str;
        size_t q_param_pos = query_params.find("q=");
        if (q_param_pos != std::string::npos) {
            size_t end_q = query_params.find('&', q_param_pos);
            query_str = query_params.substr(q_param_pos + 2, (end_q == std::string::npos ? query_params.size() : end_q) - (q_param_pos + 2));
            query_str = url_decode(query_str);
        }

        auto t0 = std::chrono::high_resolution_clock::now();
        auto results = engine->search(query_str, 10);
        auto t1 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> duration = t1 - t0;

        std::string json = "{";
        json += "\"query\":\"" + json_escape(query_str) + "\",";
        json += "\"latency_ms\":" + std::to_string(duration.count()) + ",";
        json += "\"total_hits\":" + std::to_string(results.size()) + ",";
        json += "\"results\":[";
        for (size_t i = 0; i < results.size(); ++i) {
            const auto& r = results[i];
            json += "{";
            json += "\"doc_id\":" + std::to_string(r.doc_id) + ",";
            json += "\"path\":\"" + json_escape(r.path) + "\",";
            json += "\"score\":" + std::to_string(r.score) + ",";
            json += "\"snippet\":\"" + json_escape(r.snippet) + "\"";
            json += "}";
            if (i + 1 < results.size()) json += ",";
        }
        json += "]}";

        std::string header = "HTTP/1.1 200 OK\r\n"
                             "Content-Type: application/json; charset=utf-8\r\n"
                             "Access-Control-Allow-Origin: *\r\n"
                             "Content-Length: " + std::to_string(json.size()) + "\r\n\r\n";
        send(client_sock, header.data(), static_cast<int>(header.size()), 0);
        send(client_sock, json.data(), static_cast<int>(json.size()), 0);
        CLOSE_SOCKET(client_sock);
        return;
    }

    // Default 404
    std::string not_found = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
    send(client_sock, not_found.data(), static_cast<int>(not_found.size()), 0);
    CLOSE_SOCKET(client_sock);
}

int main(int argc, char* argv[]) {
    int port = 8080;
    const char* env_port = std::getenv("PORT");
    if (env_port) {
        port = std::atoi(env_port);
    } else if (argc >= 2) {
        port = std::atoi(argv[1]);
    }

    std::string index_dir = ".search_index";
    std::string web_root = "web";

#ifdef _WIN32
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }
#endif

    std::cout << "[*] Initializing C++ Search Engine...\n";
    SearchEngine engine(index_dir);
    if (!engine.load()) {
        std::cout << "[*] Building initial index with sample documents...\n";
        engine.add_document(0, "src/network/http_server.cpp", 1700000000,
            "class HttpServer { void parseHttpRequest(const std::string& buffer); }; // High performance socket server");
        engine.add_document(1, "include/network/http_parser.hpp", 1700000001,
            "bool parseHttpRequest(const char* data, size_t length); // Fast HTTP request parsing routine");
        engine.add_document(2, "src/index/inverted_index.cpp", 1700000002,
            "class PositionalInvertedIndex { void insert(uint32_t doc_id, uint32_t position); }; // Binary disk index");
        engine.add_document(3, "src/query/bm25_ranker.cpp", 1700000003,
            "float compute_bm25_score(uint32_t tf, uint32_t doc_length, float idf); // Min-Heap Top-K ranking algorithm");
        engine.add_document(4, "src/autocomplete/trie.cpp", 1700000004,
            "class AutocompleteTrie { std::vector<std::string> get_completions(const std::string& prefix); };");
        engine.commit();
    }
    std::cout << "[+] Search Engine ready (" << engine.total_documents() << " docs loaded).\n";

    socket_t server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == INVALID_SOCKET) {
        std::cerr << "Socket creation failed\n";
        return 1;
    }

    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_sock, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed on port " << port << "\n";
        CLOSE_SOCKET(server_sock);
        return 1;
    }

    if (listen(server_sock, 128) == SOCKET_ERROR) {
        std::cerr << "Listen failed\n";
        CLOSE_SOCKET(server_sock);
        return 1;
    }

    std::cout << "\n=========================================================\n";
    std::cout << "  C++ SEARCH ENGINE HTTP REST API & WEB SERVER ONLINE    \n";
    std::cout << "=========================================================\n";
    std::cout << "  Local URL   : http://localhost:" << port << "\n";
    std::cout << "  Search API  : http://localhost:" << port << "/api/search?q=parse\n";
    std::cout << "  Trie API    : http://localhost:" << port << "/api/autocomplete?prefix=par\n";
    std::cout << "=========================================================\n\n";

    while (true) {
        sockaddr_in client_addr{};
        int addr_len = sizeof(client_addr);
#ifdef _WIN32
        socket_t client_sock = accept(server_sock, reinterpret_cast<sockaddr*>(&client_addr), &addr_len);
#else
        socket_t client_sock = accept(server_sock, reinterpret_cast<sockaddr*>(&client_addr), reinterpret_cast<socklen_t*>(&addr_len));
#endif
        if (client_sock == INVALID_SOCKET) {
            continue;
        }

#ifdef _WIN32
        handle_client(client_sock, &engine, web_root);
#else
        std::thread(handle_client, client_sock, &engine, web_root).detach();
#endif
    }

    CLOSE_SOCKET(server_sock);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
