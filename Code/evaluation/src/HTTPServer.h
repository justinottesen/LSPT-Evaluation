#pragma once

#include <nlohmann/json.hpp>
#include <optional>
#include <unordered_map>

#include "TCPSocket.h"

// TODO: HEADERS SHOULD NOT CHANGE ORDER (unordered_map is a problem here)

struct HTTPRequest {
  HTTPRequest() = default;

  HTTPRequest(std::string_view method, std::string_view resource, const nlohmann::json& body);
  
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

class HTTPWorker {
 public:
  HTTPWorker(TCPSocket&& sock)
      : m_socket(std::move(sock)) {}

  void run();

  static std::optional<HTTPRequest>  parseRequest(TCPSocket& sock);
  static std::optional<HTTPResponse> parseResponse(TCPSocket& sock);

  using Handler = void (HTTPWorker::*)(const HTTPRequest& request) const;
  static Handler handlerMapper(std::string_view resource) {
    const std::unordered_map<std::string_view, Handler> map = {
        {        "/v0/GetAutofill",         &HTTPWorker::v0getAutofill},
        {         "/v0/GetQueryID",          &HTTPWorker::v0getQueryID},
        {"/v0/ReportSearchResults", &HTTPWorker::v0reportSearchResults},
        {     "/v0/SubmitFeedback",      &HTTPWorker::v0submitFeedback},
        {       "/v0/GetQueryData",        &HTTPWorker::v0getQueryData},
        {      "/v0/ReportMetrics",       &HTTPWorker::v0reportMetrics},
    };
    return (map.contains(resource) ? map.at(resource) : &HTTPWorker::notFound);
  }

  void v0getAutofill(const HTTPRequest& /* request */) const {
    const auto suggestions = {"Why is RPI so cool?", "I love RPI", "Best Food Near RPI"};
    m_socket.send(HTTPResponse{200, "OK", {{"suggestions", suggestions}}});
  }
  void v0getQueryID(const HTTPRequest& /* request */) const {
    static unsigned int ID = 0;
    m_socket.send(HTTPResponse{200, "OK", {{"query_ID", ID++}}});
  }
  void v0reportSearchResults(const HTTPRequest& request) const;
  void v0submitFeedback(const HTTPRequest& /* request */) const { m_socket.send(HTTPResponse{200, "OK"}); }
  void v0getQueryData(const HTTPRequest& /* request */) const {
    m_socket.send(HTTPResponse{200, "OK", {{"queries", nlohmann::json::array()}}});
  }
  void v0reportMetrics(const HTTPRequest& /* request */) const { m_socket.send(HTTPResponse{200, "OK"}); }
  void notFound(const HTTPRequest& /* request */) const {
    m_socket.send(HTTPResponse::makeErrorResponse(404, "Not Found", "Resource (API function) not found"));
  }

 private:
  TCPSocket m_socket;
};

class HTTPServer {
 public:
  HTTPServer(uint16_t listener_port, int backlog_size)
      : m_listener_port(listener_port)
      , m_backlog_size(backlog_size) {}
  bool init();
  bool run(int shutdown_fd);

 private:
  uint16_t m_listener_port;
  int      m_backlog_size;

  TCPSocket m_listener_socket;

  const std::unordered_map<std::string, std::function<void(const TCPSocket&, const HTTPRequest&)>>
      m_handlers;
};
