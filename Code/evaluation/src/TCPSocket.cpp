#include "TCPSocket.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <optional>

#include "Logger.h"
#include "Util.h"

#ifdef REUSEADDR
std::mutex                   TCPSocket::ports_mutex  = {};
std::unordered_set<uint16_t> TCPSocket::ports_in_use = {};
#endif

TCPSocket::~TCPSocket() {
  if (m_socket != -1) { close(); }
}

bool TCPSocket::create() {
  m_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (m_socket == -1) {
    LOG(WARN) << "Unable to open socket: " << my_strerror(errno);
    return false;
  } else {
    LOG(TRACE) << "Opened socket fd: " << m_socket;
  }

#ifdef REUSEADDR
  int opt = 1;
  if (setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
    LOG(WARN) << "Unable to setsockopt for SO_REUSEADDR: " << my_strerror(errno);
  }
#endif

  return true;
}

bool TCPSocket::close() {
  if (m_socket == -1) {
    LOG(WARN) << "Tried to close closed socket";
    return false;
  }

#ifdef REUSEADDR
  LOG(DEBUG) << "REUSEADDR defined, removing port from use before closing socket";
  struct sockaddr_in address{
      .sin_family = AF_INET, .sin_port = htons(0), .sin_addr{.s_addr = INADDR_ANY}, .sin_zero{0}};
  socklen_t addrlen = sizeof(address);
  if (getsockname(m_socket, reinterpret_cast<struct sockaddr*>(&address), &addrlen) == -1) {
    LOG(WARN) << "Unable getsockname on socket " << m_socket << ": " << my_strerror(errno);
  } else {
    std::lock_guard<std::mutex> lock(ports_mutex);
    ports_in_use.erase(ntohs(address.sin_port));
  }
#endif

  const int ret = ::close(m_socket);
  if (ret == -1) {
    LOG(WARN) << "Unable to close socket (fd: " << m_socket << "): " << my_strerror(errno);
  } else {
    LOG(TRACE) << "Closed socket fd: " << m_socket;
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

#ifdef REUSEADDR
  LOG(DEBUG) << "REUSEADDR defined, checking port (" << port << ") availability before binding";
  std::lock_guard<std::mutex> lock(ports_mutex);
  if (ports_in_use.contains(port)) {
    LOG(WARN) << "Unable to bind socket (fd: " << m_socket << ") on port " << port << ": "
              << my_strerror(EADDRINUSE);
    return false;
  }
#endif

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  const int ret = ::bind(m_socket, reinterpret_cast<struct sockaddr*>(&address), sizeof(address));

  if (ret == -1) {
    LOG(WARN) << "Unable to bind socket (fd: " << m_socket << ") on port " << port << ": "
              << my_strerror(errno);
    return false;
  } else {
    LOG(DEBUG) << "Bound socket (fd: " << m_socket << ") to port " << port;
#ifdef REUSEADDR
    LOG(DEBUG) << "REUSEADDR defined, adding port (" << port << ") to in use";
    ports_in_use.emplace(port);
#endif
    return true;
  }
}

bool TCPSocket::listen(int backlog) const {
  if (m_socket == -1) {
    LOG(WARN) << "Tried to listen on closed socket";
    return false;
  }

  const int ret = ::listen(m_socket, backlog);
  if (ret == -1) {
    LOG(WARN) << "Socket (fd: " << m_socket << ") unable to listen: " << my_strerror(errno);
  } else {
    LOG(DEBUG) << "Socket (fd: " << m_socket << ") listening";
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
    LOG(WARN) << "Accept (fd: " << m_socket << ") failed: " << my_strerror(errno);
    return std::nullopt;
  } else {
    LOG(DEBUG) << "Socket (fd: " << m_socket << ") accepted connection (new fd: " << ret << ")";
    return TCPSocket(ret);
  }
}

bool TCPSocket::connect(const char* ip, uint16_t port) const {
  struct sockaddr_in server_address{
      .sin_family = AF_INET, .sin_port = htons(port), .sin_addr{}, .sin_zero{}};

  int ret = inet_pton(AF_INET, ip, &server_address.sin_addr);
  if (ret == 0) {
    LOG(WARN) << "Connect (inet_pton) failed: " << ip << " is not a valid address";
    return false;
  } else if (ret == -1) {
    LOG(WARN) << "Connect (inet_pton) failed: " << my_strerror(errno);
    return false;
  }

  ret = ::connect(m_socket, reinterpret_cast<struct sockaddr*>(&server_address),
                  sizeof(server_address));
  if (ret == -1) {
    LOG(WARN) << "Connect (connect) failed: " << my_strerror(errno);
  } else {
    LOG(DEBUG) << "Connect succeeded (Address: " << ip << ":" << port << ")";
  }
  return ret == 0;
}
