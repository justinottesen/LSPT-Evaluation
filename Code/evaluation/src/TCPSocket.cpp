#include "TCPSocket.h"

#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <bits/types.h>
#include <bits/types/struct_timeval.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <array>
#include <cctype>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <optional>
#include <string>
#include <string_view>

#include "Logger.h"
#include "Util.h"

static constexpr unsigned int RECV_BUFFER_SIZE = 4096;

#ifdef REUSEADDR
std::mutex                   TCPSocket::ports_mutex  = {};
std::unordered_set<uint16_t> TCPSocket::ports_in_use = {};
#endif

TCPSocket::TCPSocket()
    : m_socket(-1)
    , m_send_timeout(0)
    , m_recv_timeout(0) {}

TCPSocket::TCPSocket(TCPSocket&& sock) noexcept
    : m_socket(sock.m_socket)
    , m_send_timeout(sock.m_send_timeout)
    , m_recv_timeout(sock.m_recv_timeout) {
  sock.m_socket       = -1;
  sock.m_send_timeout = 0;
  sock.m_recv_timeout = 0;
}

TCPSocket& TCPSocket::operator=(TCPSocket&& sock) noexcept {
  if (m_socket != -1) {
    if (!close()) { LOG(WARN) << "Leaking open socket" << m_socket; }
  }
  m_socket            = sock.m_socket;
  m_send_timeout      = sock.m_send_timeout;
  m_recv_timeout      = sock.m_recv_timeout;
  sock.m_socket       = -1;
  sock.m_send_timeout = 0;
  sock.m_recv_timeout = 0;
  return *this;
}

TCPSocket::~TCPSocket() {
  if (m_socket != -1) {
    if (!close()) { LOG(WARN) << "Leaking open socket" << m_socket; }
  }
}

bool TCPSocket::create() {
  m_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (m_socket == -1) {
    LOG(WARN) << "Unable to open socket: " << my_strerror(errno);
    return false;
  }
  LOG(TRACE) << "Opened socket fd: " << m_socket;

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
    m_socket       = -1;
    m_send_timeout = 0;
    m_recv_timeout = 0;
  }
  return ret == 0;
}

template <>
bool TCPSocket::send<std::string_view>(const std::string_view& val, bool full_msg) const {
  if (m_socket == -1) {
    LOG(WARN) << "Tried to send on closed socket";
    return false;
  }
  LOG(DEBUG) << "Sending message:\n" << val;
  unsigned int sent = 0;
  do {
    const ssize_t n = ::send(m_socket, val.data() + sent, val.length() - sent, 0);
    if (n == -1) {
      LOG(WARN) << "Send Failed: " << my_strerror(errno);
      break;
    }
    sent += n;
    LOG(TRACE) << "Sent " << n << " bytes (overall " << sent << "/" << val.length() << ")";
  } while (sent < val.length() && full_msg);
  if (sent < val.length()) {
    LOG(WARN) << "Failed to send full message (sent " << sent << " of " << val.length() << ")";
  }
  return sent == val.length();
}

std::string TCPSocket::recv() const {
  std::string buf;
  if (m_socket == -1) {
    LOG(WARN) << "Tried to receive on closed socket";
    return buf;
  }
  buf.resize(RECV_BUFFER_SIZE);
  LOG(INFO) << "BUF SIZE " << buf.capacity();
  const ssize_t n = ::recv(m_socket, buf.data(), buf.size() - 1, 0);
  if (n == -1) {
    LOG(WARN) << "Recv failed: " << my_strerror(errno);
    return "";
  }
  buf.resize(n);
  LOG(DEBUG) << "Received " << n << " bytes:\n" << buf;
  return buf;
}

template <> bool TCPSocket::setTimeout<SO_RCVTIMEO>(unsigned int timeout_ms) {
  return setTimeout(timeout_ms, SO_RCVTIMEO);
}

template <> bool TCPSocket::setTimeout<SO_SNDTIMEO>(unsigned int timeout_ms) {
  return setTimeout(timeout_ms, SO_SNDTIMEO);
}

bool TCPSocket::setTimeout(unsigned int timeout_ms, int option) {
  if (m_socket == -1) {
    LOG(WARN) << "Tried to set timeout on closed socket";
    return false;
  }
  if ((option == SO_RCVTIMEO && m_recv_timeout == timeout_ms)
      || (option == SO_SNDTIMEO && m_send_timeout == timeout_ms)) {
    LOG(DEBUG) << "Socket (fd: " << m_socket << ") timeout already set to " << timeout_ms << " ms";
    return true;
  }

  struct timeval tv{.tv_sec  = timeout_ms / 1000,
                    .tv_usec = static_cast<__suseconds_t>(timeout_ms) * 1000};
  LOG(DEBUG) << "Setting timeout (fd: " << m_socket << ") to " << tv.tv_sec << "." << tv.tv_usec;
  if (setsockopt(m_socket, SOL_SOCKET, option, &tv, sizeof(tv)) == -1) {
    LOG(WARN) << "Unable to setsockopt for timeout: " << my_strerror(errno);
    return false;
  }
  if (option == SO_RCVTIMEO) { m_recv_timeout = timeout_ms; }
  if (option == SO_SNDTIMEO) { m_send_timeout = timeout_ms; }
  LOG(DEBUG) << "Set socket (fd: " << m_socket << ") timeout to " << timeout_ms << "ms";
  return true;
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
  }
  LOG(DEBUG) << "Bound socket (fd: " << m_socket << ") to port " << port;
#ifdef REUSEADDR
  LOG(DEBUG) << "REUSEADDR defined, adding port (" << port << ") to in use";
  ports_in_use.emplace(port);
#endif
  return true;
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
  }
  LOG(DEBUG) << "Socket (fd: " << m_socket << ") accepted connection (new fd: " << ret << ")";
  return TCPSocket(ret);
}

bool TCPSocket::connect(const char* ip, uint16_t port) const {
  if (m_socket == -1) {
    LOG(WARN) << "Tried to connect on closed socket";
    return false;
  }

  LOG(INFO) << "Attempting to connect to " << ip << ":" << port;

  struct sockaddr_in server_address{
      .sin_family = AF_INET, .sin_port = htons(port), .sin_addr{}, .sin_zero{}};

  int ret = inet_pton(AF_INET, ip, &server_address.sin_addr);
  if (ret == 0) {
    LOG(WARN) << "Connect (inet_pton) failed: " << ip << " is not a valid address";
    return false;
  }
  if (ret == -1) {
    LOG(WARN) << "Connect (inet_pton) failed: " << my_strerror(errno);
    return false;
  }

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  ret = ::connect(m_socket, reinterpret_cast<struct sockaddr*>(&server_address),
                  sizeof(server_address));
  if (ret == -1) {
    LOG(WARN) << "Connect (connect) failed: " << my_strerror(errno);
  } else {
    LOG(DEBUG) << "Connect succeeded (Address: " << ip << ":" << port << ")";
  }
  return ret != -1;
}

std::string TCPSocket::getIP(const std::string& domain, uint16_t port) {
  struct addrinfo  hints{};
  struct addrinfo* res = nullptr;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family   = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  const std::string port_str = std::to_string(port);

  const int status = getaddrinfo(domain.c_str(), port_str.c_str(), &hints, &res);
  if (status != 0) {
    LOG(ERROR) << "Error in `getaddrinfo` - Code: " << status << " (see `man getaddrinfo`)";
    freeaddrinfo(res);
    return "";
  }

  if (res != nullptr) {
    std::array<char, INET_ADDRSTRLEN> ipstr{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* ipv4 = reinterpret_cast<sockaddr_in*>(res->ai_addr);
    inet_ntop(res->ai_family, &(ipv4->sin_addr), ipstr.data(), sizeof(ipstr));
    freeaddrinfo(res);
    return {ipstr.begin(), ipstr.end()};
  }

  freeaddrinfo(res);
  return "";
}

bool SocketStream::hasNext() {
  unsigned int tmp_pos = m_pos;
  grab_if_needed(tmp_pos);
  while (tmp_pos < m_buffer.length() && (isspace(m_buffer[tmp_pos]) != 0)) {
    grab_if_needed(++tmp_pos);
  }
  return tmp_pos < m_buffer.length();
}

std::string SocketStream::nextWord() {
  // Increment to start of next word
  grab_if_needed(m_pos);
  while (m_pos < m_buffer.length() && (isspace(m_buffer[m_pos]) != 0)) { grab_if_needed(++m_pos); }

  // Increment to end of word
  const unsigned int start_pos = m_pos;
  while (m_pos < m_buffer.length() && (isspace(m_buffer[m_pos]) == 0)) { grab_if_needed(++m_pos); }
  return m_buffer.substr(start_pos, m_pos - start_pos);
}

std::string SocketStream::nextLine(bool skip_whitespace) {
  grab_if_needed(m_pos);
  if (skip_whitespace) {
    while (m_pos < m_buffer.length() && (isspace(m_buffer[m_pos]) != 0)) {
      grab_if_needed(++m_pos);
    }
  }
  const unsigned int start_pos = m_pos;
  while (m_pos < m_buffer.length() && m_buffer[m_pos] != '\n') { grab_if_needed(++m_pos); }
  std::string line = m_buffer.substr(start_pos, (m_pos++) - start_pos);
  if (line.back() == '\r') { line.pop_back(); }    // HTTP uses CRLF
  return line;
}

std::string SocketStream::remaining() {
  grab_if_needed(m_pos);
  const unsigned int start_pos = m_pos;
  while (m_pos < m_buffer.length()) {
    m_pos = m_buffer.length();
    grab_if_needed(m_pos);
  }
  return m_buffer.substr(start_pos);
}

void SocketStream::grab() {
  const std::string next = m_socket.recv();
  m_buffer += next;
  m_timed_out = m_timed_out || next.empty();
}

void SocketStream::grab_if_needed(unsigned int buffer_pos) {
  if (buffer_pos == m_buffer.length() && !m_timed_out) { grab(); }
}
