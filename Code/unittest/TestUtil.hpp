#pragma once

#include <utility>

#include "TCPSocket.h"
#include "HTTPServer.h"

#include <gtest/gtest.h>

static std::pair<TCPSocket, TCPSocket> get_server_and_client(uint16_t port) {
  std::pair<TCPSocket, TCPSocket> sockets;

  EXPECT_TRUE(sockets.first.create());
  EXPECT_TRUE(sockets.first.bind(port));
  EXPECT_TRUE(sockets.first.listen(1));

  EXPECT_TRUE(sockets.second.create());
  EXPECT_TRUE(sockets.second.connect("127.0.0.1", port));

  std::optional<TCPSocket> server_messager = sockets.first.accept();
  EXPECT_TRUE(server_messager.has_value());

  sockets.first = std::move(server_messager.value());

  return sockets;
}

void compare_http_responses(const HTTPResponse& h1, const HTTPResponse& h2) {
  EXPECT_EQ(h1.version, h2.version);
  EXPECT_EQ(h1.code, h2.code);
  EXPECT_EQ(h1.status, h2.status);
  EXPECT_EQ(h1.headers, h2.headers);
  EXPECT_EQ(h1.body, h2.body);
}