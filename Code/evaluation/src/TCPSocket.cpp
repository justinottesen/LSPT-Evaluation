#include "TCPSocket.h"

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <optional>

#include "Logger.h"
#include "Util.h"

TCPSocket::~TCPSocket() {
  if (m_socket != -1) { close(); }
}

bool TCPSocket::create() {
  m_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (m_socket == -1) {
    LOG(WARN) << "Unable to open socket: " << my_strerror(errno);
  } else {
    LOG(TRACE) << "Opened socket";
  }
  return m_socket != -1;
}

bool TCPSocket::close() {
  if (m_socket == -1) {
    LOG(WARN) << "Tried to close closed socket";
    return false;
  }

  const int ret = ::close(m_socket);
  if (ret == -1) {
    LOG(WARN) << "Unable to close socket: " << my_strerror(errno);
  } else {
    LOG(TRACE) << "Closed socket";
    m_socket = -1;
  }
  return ret == 0;
}

bool TCPSocket::bind(uint16_t port) const {
  if (m_socket == -1) {
    LOG(WARN) << "Tried to bind closed socket";
    return false;
  }

  struct sockaddr_in address{.sin_family = AF_INET,
                             .sin_port   = htons(port),
                             .sin_addr{.s_addr = INADDR_ANY},
                             .sin_zero{0}};

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  const int ret = ::bind(m_socket, reinterpret_cast<struct sockaddr*>(&address), sizeof(address));

  if (ret == -1) {
    LOG(WARN) << "Unable to bind socket on port " << port << ": " << my_strerror(errno);
  } else {
    LOG(DEBUG) << "Bound socket to port " << port;
  }
  return ret == 0;
}

bool TCPSocket::listen(int backlog) const {
  if (m_socket == -1) {
    LOG(WARN) << "Tried to listen on closed socket";
    return false;
  }

  const int ret = ::listen(m_socket, backlog);
  if (ret == -1) {
    LOG(WARN) << "Socket unable to listen: " << my_strerror(errno);
  } else {
    LOG(DEBUG) << "Socket listening";
  }
  return ret == 0;
}

std::optional<TCPSocket> TCPSocket::accept() const {
  if (m_socket == -1) {
    LOG(WARN) << "Tried to accept on closed socket";
    return std::nullopt;
  }

  struct sockaddr_in address{};
  socklen_t          addrlen = sizeof(address);

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  const int ret = ::accept(m_socket, reinterpret_cast<struct sockaddr*>(&address), &addrlen);
  if (ret < 0) {
    LOG(WARN) << "Accept failed: " << my_strerror(errno);
    return std::nullopt;
  }
  LOG(DEBUG) << "Socket accepted connection";
  return TCPSocket(ret);
}
