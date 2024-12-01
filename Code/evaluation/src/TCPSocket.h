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
  TCPSocket();
  ~TCPSocket();

  // Allow move consturctor & assignment
  TCPSocket(TCPSocket&& sock) noexcept;
  TCPSocket& operator=(TCPSocket&& sock) noexcept;

  // DO NOT allow copy construcor
  TCPSocket(const TCPSocket&)            = delete;
  TCPSocket& operator=(const TCPSocket&) = delete;

  // General Functions
  bool create();
  bool close();

  // full_msg - send until all bytes are sent
  unsigned int send(std::string_view msg, bool full_msg = true) const;

  // 0 for no timeout, option: SO_RCVTIMEO or SO_SNDTIMEO
  template <int option> bool setTimeout(unsigned int timeout);
  std::string                recv() const;
  int                        fd() const { return m_socket; }

  // Server Side Functions
  bool                     bind(uint16_t port) const;
  bool                     listen(int backlog) const;
  std::optional<TCPSocket> accept() const;

  // Client Side Functions
  bool connect(const char* ip, uint16_t port) const;

 private:
  int m_socket;

  // Current timeout in seconds, or 0 if no timeout
  unsigned int m_send_timeout;
  unsigned int m_recv_timeout;

  // This constnstructor should only be used internally to avoid misuse
  explicit TCPSocket(int sockfd)
      : m_socket(sockfd)
      , m_send_timeout(0)
      , m_recv_timeout(0) {}

  bool setTimeout(unsigned int timeout, int option) const;

#ifdef REUSEADDR
  static std::mutex                   ports_mutex;
  static std::unordered_set<uint16_t> ports_in_use;
#endif
};

class SocketStream {
 public:
  SocketStream(const TCPSocket& sock)
      : m_socket(sock) {}

  bool        hasNext();
  std::string nextWord();
  std::string nextLine();

  const std::string& str() { return m_buffer; }

 private:
  void grab();
  void grab_if_needed(unsigned int buffer_pos);

  std::string  m_buffer;
  unsigned int m_pos       = 0;
  bool         m_timed_out = false;

  const TCPSocket& m_socket;
};
