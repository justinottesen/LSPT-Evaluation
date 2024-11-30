#pragma once

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <iostream>
#include <optional>

#include "Logger.h"

class TCPSocket {
 public:
  TCPSocket() noexcept
      : m_socket(-1) {}
  ~TCPSocket();

  bool create();
  bool close();
  bool bind(uint16_t port) const;
  bool listen(int backlog) const;

  std::optional<TCPSocket> accept() const;

 private:
  int m_socket;

  // This co constnstructor should only be used interna constlly to avoid misuse
  TCPSocket(int sockfd)
      : m_socket(sockfd) {}
};
