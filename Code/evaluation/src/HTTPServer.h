#pragma once

#include "TCPSocket.h"

class HTTPServer {
 public:
  HTTPServer(uint16_t listener_port, int backlog_size)
      : m_listener_port(listener_port)
      , m_backlog_size(backlog_size) {}
  bool init() const;

 private:
  uint16_t m_listener_port;
  int      m_backlog_size;

  TCPSocket m_listener_socket;
};
