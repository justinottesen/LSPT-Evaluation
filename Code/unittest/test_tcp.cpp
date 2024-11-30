#include <gtest/gtest.h>

#include "TCPSocket.h"

static constexpr uint16_t PORT_NUM = 9999;

TEST(TCPTest, SimpleOpenClose) {

  TCPSocket sock;

  EXPECT_TRUE(sock.create());
  EXPECT_TRUE(sock.close());
}

TEST(TCPTest, SimpleBind) {

  TCPSocket sock, sock2;

  EXPECT_TRUE(sock.create());
  EXPECT_TRUE(sock.bind(PORT_NUM));

  // Other sockets shouldn't be able to bind to same port
  EXPECT_TRUE(sock2.create());
  EXPECT_FALSE(sock2.bind(PORT_NUM));

  EXPECT_TRUE(sock.close());
  EXPECT_TRUE(sock2.close());
}

TEST(TCPTest, SocketClosesInDestructor) {

  {
    TCPSocket sock;

    EXPECT_TRUE(sock.create());
    EXPECT_TRUE(sock.bind(PORT_NUM));
  }    // End of scope should call destructor on socket

  TCPSocket sock2;

  EXPECT_TRUE(sock2.create());
  EXPECT_TRUE(sock2.bind(PORT_NUM));

  EXPECT_TRUE(sock2.close());
}

TEST(TCPTest, TestListenConnectAccept) {

  TCPSocket server, client;

  EXPECT_TRUE(server.create());
  EXPECT_TRUE(server.bind(PORT_NUM));
  EXPECT_TRUE(server.listen(1));

  // std::this_thread::sleep_for(std::chrono::milliseconds(50));

  EXPECT_TRUE(client.create());
  EXPECT_TRUE(client.connect("127.0.0.1", PORT_NUM));

  // std::this_thread::sleep_for(std::chrono::milliseconds(50));

  std::optional<TCPSocket> connected = server.accept();

  EXPECT_TRUE(connected.has_value());
}
