#include <gtest/gtest.h>

#include "TCPSocket.h"

static constexpr uint16_t PORT_NUM = 8080;

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

  EXPECT_TRUE(client.create());
  EXPECT_TRUE(client.connect("127.0.0.1", PORT_NUM));

  std::optional<TCPSocket> connected = server.accept();

  EXPECT_TRUE(connected.has_value());
}

TEST(TCPTest, TestMessaging) {

  TCPSocket server_listener, client;

  EXPECT_TRUE(server_listener.create());
  EXPECT_TRUE(server_listener.bind(PORT_NUM));
  EXPECT_TRUE(server_listener.listen(1));

  EXPECT_TRUE(client.create());
  EXPECT_TRUE(client.connect("127.0.0.1", PORT_NUM));

  std::optional<TCPSocket> server_messager = server_listener.accept();
  EXPECT_TRUE(server_messager.has_value());

  EXPECT_EQ(client.send("Hello Server!"), strlen("Hello Server!"));
  EXPECT_EQ(server_messager.value().recv(), "Hello Server!");

  EXPECT_EQ(server_messager.value().send("Hello Client!"), strlen("Hello Client!"));
  EXPECT_EQ(client.recv(), "Hello Client!");
}
