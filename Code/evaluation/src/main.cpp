#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cassert>
#include <csignal>
#include <cstring>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <unordered_map>

constexpr uint16_t PORT        = 8080;
constexpr size_t   BUFFER_SIZE = 4096;

int server_sock = -1;

void send_response(int client_sock, int http_code, std::string short_msg,
                   std::string_view body = "", std::string_view type = "text/plain") {
  std::ostringstream response;
  response << "HTTP/1.1 " << http_code << " " << short_msg << "\r\n";
  if (!body.empty()) {
    response << "Content-Type: " << type << "\r\n"
             << "Content-Length: " << body.length() << "\r\n";
  }
  response << "\r\n";
  if (!body.empty()) { response << body; }
  std::string response_str = response.str();

  std::cout << "=== SENDING RESPONSE ===\n" << response_str << "\n------------------------\n";

  send(client_sock, response_str.data(), response_str.length(), 0);

  close(client_sock);
}

void handle_client(int client_sock) {
  try
  {
    char    buffer[BUFFER_SIZE] = {0};
    ssize_t read_len            = read(client_sock, buffer, BUFFER_SIZE - 1);
    if (read_len < 0) {
      perror("Read failed");
      errno = 0;

      send_response(client_sock, 500, "Internal Server Error");
      return;
    }
    std::string buffer_string(buffer);
    std::erase(buffer_string, '\r');
    std::cout << "=== RECEIVED REQUEST ===\n" << buffer_string << "\n------------------------\n";
    std::istringstream stream(buffer_string);

    // Read the start line
    std::string method, resource, HTTP_version;
    stream >> method >> resource >> HTTP_version;
    std::string line;
    std::getline(stream, line);    // Get the remains of the first line
    if (!std::ranges::all_of(line, isspace)) {
      send_response(client_sock, 400, "Extra characters found in start line");
      return;
    }

    if (HTTP_version != "HTTP/1.1") {
      send_response(client_sock, 505, "HTTP Version Not Supported", "HTTP/1.1 Must be Used");
      return;
    }

    // Read the headers
    std::unordered_map<std::string, std::string> headers;
    while (std::getline(stream, line)) {
      std::size_t delim_pos = line.find(':');
      if (delim_pos == std::string::npos) {
        break;    // Reached the end of the headers
      }

      std::string header = line.substr(0, delim_pos);
      for (char& c : header) { c = std::tolower(c); }
      std::string value = line.substr(delim_pos + 2);
      headers[header]   = value;
    }
    if (stream.peek() == '\n') { stream.get(); }

    // Check if body has been sent
    std::string body = stream.eof() ? "" : stream.str().substr(stream.tellg());
    if (!body.empty()) {
      if (!headers.contains("content-length")) {
        send_response(client_sock, 411, "Length Required",
                      "Received a body with no Content-Length header");
      } else if (body.length() != std::stoul(headers["content-length"])) {
        send_response(client_sock, 400, "Bad Request", "Content length does not match body length");
      }
      return;
    }

    // Actually fulfill request
    if (resource == "/GetAutofill") {
      if (method != "GET") {
        send_response(client_sock, 405, "Method Not Allowed");
        return;
      }
      if (!headers.contains("num-suggestions")) {
        send_response(client_sock, 400, "Bad Request", "Missing Num-Suggestions header");
        return;
      }
      if (!headers.contains("partial-query")) {
        send_response(client_sock, 400, "Bad Request", "Missing Partial-Query header");
        return;
      }
      std::vector<std::string> suggestions;
      for (unsigned int i = 0; i < std::stoul(headers["num-suggestions"]); i++) {
        suggestions.emplace_back("This is suggestion number ").append(std::to_string(i + 1));
      }

      nlohmann::json response_json = {
          {"suggestions", suggestions}
      };

      send_response(client_sock, 200, "OK", response_json.dump(2), "application/json");
      return;
    } else if (resource == "/GetQueryID") {
      if (method != "GET") {
        send_response(client_sock, 405, "Method Not Allowed");
        return;
      }

      static unsigned int i = 0;

      nlohmann::json response_json = {
          {"queryID", i++}
      };

      send_response(client_sock, 200, "OK", response_json.dump(2), "application/json");
      return;
    } else if (resource == "/ReportSearchResults") {
      if (method != "POST") {
        send_response(client_sock, 405, "Method Not Allowed");
        return;
      }

      if (body.empty()) { send_response(client_sock, 400, "Bad Request", "Requires a request body"); }

      try {
        nlohmann::json           request_json    = nlohmann::json::parse(body);
        unsigned int             query_id        = request_json["queryID"];
        std::string              raw_query       = request_json["rawQuery"];
        std::vector<std::string> results         = request_json["results"];
        unsigned int             clicked         = request_json["clicked"];
        std::string              query_timestamp = request_json["timestamp"];

        send_response(client_sock, 200, "OK");
        return;
      } catch (const std::exception& e) {
        send_response(client_sock, 400, "Bad Request", "Malformed JSON");
        return;
      }
    } else if (resource == "/SubmitFeedback") {
      if (method != "POST") {
        send_response(client_sock, 405, "Method Not Allowed");
        return;
      }

      if (body.empty()) { send_response(client_sock, 400, "Bad Request", "Requires a request body"); }

      try {
        nlohmann::json request_json = nlohmann::json::parse(body);
        std::string    label        = request_json["label"];
        std::string    title        = request_json["title"];
        std::string    text         = request_json["text"];

        send_response(client_sock, 200, "OK");
        return;
      } catch (const std::exception& e) {
        send_response(client_sock, 400, "Bad Request", "Malformed JSON");
        return;
      }
    } else if (resource == "/GetQueryData") {
      if (method != "GET") {
        send_response(client_sock, 405, "Method Not Allowed");
        return;
      }

      if (!headers.contains("query-id")) {
        send_response(client_sock, 400, "Bad Request", "Missing Query-ID header");
        return;
      }

      unsigned int query_id = std::stoul(headers["query-id"]);
      if (query_id % 2 == 0) {
        nlohmann::json response_json = {
            {"queryID",                                query_id},
            {"results", {"link1.com", "link2.com", "link3.com"}},
            {"clicked",                                       1}
        };

        send_response(client_sock, 200, "OK", response_json.dump(2), "application/json");
        return;
      } else {

        send_response(client_sock, 204, "No Content");
        return;
      }

    } else if (resource == "/ReportMetrics") {
      if (method != "POST") {
        send_response(client_sock, 405, "Method Not Allowed");
        return;
      }

      if (body.empty()) { send_response(client_sock, 400, "Bad Request", "Requires a request body"); }

      try {
        nlohmann::json request_json = nlohmann::json::parse(body);

        std::vector<nlohmann::json> metrics_list = request_json["metrics"];
        for (nlohmann::json& metric : metrics_list) {
          std::string label = metric["label"];
          nlohmann::json value = metric["value"];
        }

        send_response(client_sock, 200, "OK");
        return;
      } catch (const std::exception& e) {
        send_response(client_sock, 400, "Bad Request", "Malformed JSON");
        return;
      }
    } else {
      send_response(client_sock, 404, "Not Found", "Specified request target is not valid");
    }
  }
  catch(const std::exception& e)
  {
    std::string message = "This might be your fault, it might be ours, we don't know\n\nException msg: ";
    message.append(e.what());
    send_response(client_sock, 500, "Internal Server Error", message);
  }
  

  
}

void close_socket() { close(server_sock); }

void signal_handler(int sig_num) {
  switch (sig_num) {
    case SIGINT:
    case SIGTSTP: close(server_sock); return;
    default:      ; /* Nothing */
  }
  exit(EXIT_SUCCESS);
}

int main() {
  server_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (server_sock < 0) {
    perror("Socket failed");
    exit(EXIT_FAILURE);
  }

  signal(SIGINT, signal_handler);
  signal(SIGTSTP, signal_handler);

  // Set SO_REUSEADDR
  int opt = 1;
  if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
    perror("setsockopt failed");
    close(server_sock);
    return EXIT_FAILURE;
  }

  struct sockaddr_in address{.sin_family = AF_INET,
                             .sin_port   = htons(PORT),
                             .sin_addr{.s_addr = INADDR_ANY},
                             .sin_zero{0}};
  socklen_t          addrlen = sizeof(address);

  if (bind(server_sock, (struct sockaddr*)&address, sizeof(address)) < 0) {
    perror("Bind failed");
    exit(EXIT_FAILURE);
  }

  if (listen(server_sock, 3) < 0) {
    perror("Listen failed");
    exit(EXIT_FAILURE);
  }

  std::cout << "evaluation listening on port " << PORT << "...\n";

  while (true) {
    int client_sock = accept(server_sock, (struct sockaddr*)&address, &addrlen);
    if (client_sock < 0) {
      perror("Accept failed");
      exit(EXIT_FAILURE);
    }
    handle_client(client_sock);
  }
}
