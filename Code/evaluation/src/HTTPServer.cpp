#include "HTTPServer.h"

#include <asm-generic/socket.h>
#include <poll.h>
#include <sys/poll.h>

#include <array>
#include <cctype>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <nlohmann/json.hpp>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

#include "Logger.h"
#include "TCPSocket.h"
#include "Util.h"

std::string to_string(const HTTPRequest& request) {
  std::ostringstream ss;
  ss << request.method << " " << request.resource << " " << request.version << "\r\n";
  for (const auto& [header, value] : request.headers) { ss << header << ": " << value << "\r\n"; }
  ss << "\r\n" << request.body;
  return ss.str();
}

HTTPResponse::HTTPResponse(unsigned int code_, std::string_view status_)
    : version("HTTP/1.1")
    , code(code_)
    , status(status_) {}

HTTPResponse::HTTPResponse(unsigned int code_, std::string_view status_,
                           const nlohmann::json& body_)
    : version("HTTP/1.1")
    , code(code_)
    , status(status_) {
  body    = body_.dump(2);
  headers = {
      {  "Content-Type",            "application/json"},
      {"Content-Length", std::to_string(body.length())}
  };
}

HTTPResponse HTTPResponse::makeErrorResponse(unsigned int code_, std::string_view status_,
                                             std::string_view msg) {
  const nlohmann::json resp_body = {
      {  "error", status_},
      {"message",     msg}
  };
  return {code_, status_, resp_body};
}

std::string to_string(const HTTPResponse& response) {
  std::ostringstream ss;
  ss << response.version << " " << response.code << " " << response.status << "\r\n";
  for (const auto& [header, value] : response.headers) { ss << header << ": " << value << "\r\n"; }
  ss << "\r\n" << response.body;
  return ss.str();
}

bool HTTPServer::init() {
  if (!m_listener_socket.create()) {
    LOG(CRITICAL) << "Unable to open HTTP Listener socket";
    return false;
  }

  if (!m_listener_socket.bind(m_listener_port)) {
    LOG(CRITICAL) << "Unable to bind HTTP Listener socket to port " << m_listener_port;
    return false;
  }

  if (!m_listener_socket.listen(m_backlog_size)) {
    LOG(CRITICAL) << "Unable to listen on socket";
    return false;
  }

  return true;
}

bool HTTPServer::run(int shutdown_fd) {
  // Initialize HTTP server
  if (!init()) {
    LOG(CRITICAL) << "Failed to initialize HTTP server";
    return EXIT_FAILURE;
  }

  // Accept clients while checking for shutdown
  std::array<pollfd, 2> poll_fds{};
  poll_fds[0] = {.fd = shutdown_fd, .events = POLLIN, .revents = 0};
  poll_fds[1] = {.fd = m_listener_socket.fd(), .events = POLLIN, .revents = 0};
  while (true) {
    LOG(INFO) << "Blocking on poll...";
    const int poll_res = poll(poll_fds.data(), poll_fds.size(), -1);
    // Poll had an error
    if (poll_res == -1) {
      LOG(CRITICAL) << "Poll failed: " << my_strerror(errno);
      return false;
    }
    if (poll_res == 0) {
      LOG(DEBUG) << "Poll timed out. Polling agian...";
      continue;
    }
    if ((static_cast<uint8_t>(poll_fds[0].revents) & static_cast<uint8_t>(POLLIN)) != 0) {
      LOG(INFO) << "Poll received shutdown input";
      break;
    }
    if ((static_cast<uint8_t>(poll_fds[1].revents) & static_cast<uint8_t>(POLLIN)) == 0) {
      LOG(WARN) << "Poll received non-POLLIN event. This is unexpected but shouldn't be an issue";
      poll_fds[0].revents = 0;
      poll_fds[1].revents = 0;
      continue;
    }
    LOG(INFO) << "Poll received listener input, accepting client connection";
    std::optional<TCPSocket> client_socket = m_listener_socket.accept();
    if (!client_socket.has_value()) {
      LOG(WARN) << "Accept did not receive a client connection";
      continue;
    }

    // TODO: Make this multithreaded
    handle_client(client_socket.value());
  }
  return true;
}

void HTTPServer::handle_client(TCPSocket& sock) {
  sock.setTimeout<SO_RCVTIMEO>(500);
  std::optional<HTTPRequest> request_opt = parseRequest(sock);

  if (!request_opt.has_value()) {
    sock.send(HTTPResponse::makeErrorResponse(400, "Bad Request", "Error parsing request."));
    return;
  }

  HTTPRequest& request = request_opt.value();
  if (request.version != "HTTP/1.1") {
    sock.send(HTTPResponse::makeErrorResponse(505, "HTTP Version Not Supported",
                                              "HTTP/1.1 Must be Used."));
    return;
  }

  if (!request.body.empty()) {
    if (!request.headers.contains("content-length")) {
      sock.send(HTTPResponse::makeErrorResponse(
          411, "Length Required",
          "Content-Length header must be specified when sending a request body"));
      return;
    }

    unsigned int content_length = 0;
    try {
      content_length = std::stoul(request.headers["content-length"]);
    } catch (const std::exception& e) {
      sock.send(HTTPResponse::makeErrorResponse(
          400, "Bad Request",
          "Specified content length (" + request.headers["content-length"] + ") is invalid"));
      return;
    }

    if (content_length != request.body.length()) {
      sock.send(HTTPResponse::makeErrorResponse(
          400, "Bad Request",
          "Provided content length " + request.headers["content-length"]
              + " does not match actual content length " + std::to_string(request.body.length())));
      return;
    }
  }

  if (!m_handlers.contains(request.resource)) {
    sock.send(
        HTTPResponse::makeErrorResponse(404, "Not Found", "Resource (API function) not found"));
    return;
  }

  // Maybe multithreaded starts here by creating a thread?
  m_handlers.at(request.resource)(sock, request);
}

std::optional<HTTPRequest> HTTPServer::parseRequest(TCPSocket& sock) {
  LOG(DEBUG) << "Parsing HTTP Response on sock " << sock.fd();
  SocketStream ss(sock);

  HTTPRequest request;
  request.method   = ss.nextWord();
  request.resource = ss.nextWord();
  request.version  = ss.nextWord();
  if (ss.passedBuffer().find('\n') != std::string::npos || request.method.empty()
      || request.resource.empty() || request.version.empty()) {
    LOG(TRACE) << "First line is not complete (" << ss.passedBuffer() << ")";
    return std::nullopt;
  }
  LOG(TRACE) << "METHOD: " << request.method << " RESOURCE: " << request.resource
             << " VERSION: " << request.version;
  std::string line = ss.nextLine();
  for (const char c : line) {
    if (isspace(c) == 0) {
      LOG(TRACE) << "Extra chars at end of first line: " << line;
      return std::nullopt;
    }
  }
  while (ss.hasNext() && !(line = ss.nextLine()).empty()) {
    const std::size_t delim_pos = line.find(':');
    if (delim_pos == std::string::npos) { break; }

    std::string header = line.substr(0, delim_pos);
    for (char& c : header) { c = static_cast<char>(std::tolower(c)); }
    std::size_t val_start = delim_pos + 1;
    while (isspace(line[val_start]) != 0) { val_start++; }
    request.headers[header] = line.substr(val_start);
    LOG(TRACE) << "HEADER: " << header << " - VALUE: " << request.headers[header];
  }
  if (ss.hasNext()) {
    request.body = ss.remaining();
    LOG(TRACE) << "BODY:\n" << request.body;
  }
  return request;
}

std::optional<HTTPResponse> HTTPServer::parseResponse(TCPSocket& sock) {
  LOG(DEBUG) << "Parsing HTTP Response on sock " << sock.fd();
  SocketStream ss(sock);

  HTTPResponse response;
  response.version = ss.nextWord();
  try {
    response.code = std::stoul(ss.nextWord());
  } catch (const std::exception& e) {
    LOG(TRACE) << "Invalid response code provided";
    return std::nullopt;
  }
  response.status = ss.nextLine(true);
  if (ss.passedBuffer().find('\n') != ss.passedBuffer().length() - 1 || response.version.empty()
      || response.code == 0 || response.status.empty()) {
    LOG(TRACE) << "First line is not complete (" << ss.passedBuffer() << ")";
    return std::nullopt;
  }
  LOG(TRACE) << "VERSION: " << response.version << " CODE: " << response.code
             << " STATUS: " << response.status;
  std::string line;
  while (ss.hasNext() && !(line = ss.nextLine()).empty()) {
    const std::size_t delim_pos = line.find(':');
    if (delim_pos == std::string::npos) { break; }

    std::string header = line.substr(0, delim_pos);
    for (char& c : header) { c = static_cast<char>(std::tolower(c)); }
    std::size_t val_start = delim_pos + 1;
    while (isspace(line[val_start]) != 0) { val_start++; }
    response.headers[header] = line.substr(val_start);
    LOG(TRACE) << "HEADER: " << header << " - VALUE: " << response.headers[header];
  }
  if (ss.hasNext()) {
    response.body = ss.remaining();
    LOG(TRACE) << "BODY:\n" << response.body;
  }
  return response;
}

