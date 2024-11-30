#pragma once

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <iostream>
#include <optional>

#include "Logger.h"

#ifdef REUSEADDR
  #include <mutex>
  #include <unordered_set>
#endif

class TCPSocket {
 public:
  TCPSocket() noexcept
      : m_socket(-1) {}
  ~TCPSocket();

  // Allow move consturctor & assignment
  TCPSocket(TCPSocket&& sock)
      : m_socket(sock.m_socket) {
    sock.m_socket = -1;
  }
  TCPSocket& operator=(TCPSocket&& sock) {
    m_socket      = sock.m_socket;
    sock.m_socket = -1;
    return *this;
  }

  // DO NOT allow copy construcor
  TCPSocket(const TCPSocket&)            = delete;
  TCPSocket& operator=(const TCPSocket&) = delete;

  // General Functions
  bool create();
  bool close();
  // full_msg will send until all bytes are sent
  unsigned int send(std::string_view msg, bool full_msg = true) const;
  std::string  recv() const;

  // Server Side Functions
  bool                     bind(uint16_t port) const;
  bool                     listen(int backlog) const;
  std::optional<TCPSocket> accept() const;

  // Client Side Functions
  bool connect(const char* ip, uint16_t port) const;

 private:
  int m_socket;

  // This constnstructor should only be used internally to avoid misuse
  explicit TCPSocket(int sockfd)
      : m_socket(sockfd) {}

#ifdef REUSEADDR
  static std::mutex                   ports_mutex;
  static std::unordered_set<uint16_t> ports_in_use;
#endif
};
