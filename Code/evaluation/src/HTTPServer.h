#pragma once

#include <nlohmann/json.hpp>
#include <optional>
#include <unordered_map>

#include "TCPSocket.h"

// TODO: HEADERS SHOULD NOT CHANGE ORDER (unordered_map is a problem here)

struct HTTPRequest {
  std::string                                  method;
  std::string                                  resource;
  std::string                                  version;
  std::unordered_map<std::string, std::string> headers;
  std::string                                  body;
};

std::string to_string(const HTTPRequest& request);

struct HTTPResponse {
  HTTPResponse() = default;

  HTTPResponse(unsigned int code, std::string_view status);

  // This will add Content-Type and Content-Length headers
  HTTPResponse(unsigned int code, std::string_view status, const nlohmann::json& body);

  // Specifically not a constructor to avoid mistakes.
  // This generates a JSON error response with the given status and message
  static HTTPResponse makeErrorResponse(unsigned int code, std::string_view status,
                                        std::string_view msg);

  std::string                                  version;
  unsigned int                                 code;
  std::string                                  status;
  std::unordered_map<std::string, std::string> headers;
  std::string                                  body;
};

std::string to_string(const HTTPResponse& response);

class HTTPServer {
 public:
  HTTPServer(uint16_t listener_port, int backlog_size)
      : m_listener_port(listener_port)
      , m_backlog_size(backlog_size) {}
  bool init();
  bool run(int shutdown_fd);

  void handle_client(TCPSocket& client_sock);

  static std::optional<HTTPRequest>  parseRequest(TCPSocket& sock);
  static std::optional<HTTPResponse> parseResponse(TCPSocket& sock);

 private:
  uint16_t m_listener_port;
  int      m_backlog_size;

  TCPSocket m_listener_socket;

  const std::unordered_map<std::string, std::function<void(const TCPSocket&, const HTTPRequest&)>>
      m_handlers;
};
