#include "HTTPServer.h"

#include "Logger.h"

bool HTTPServer::init() const {
  LOG(INFO) << "I am an HTTP server and I am setting up on port " << m_listener_port
            << " with backlog " << m_backlog_size;
  return true;
}
