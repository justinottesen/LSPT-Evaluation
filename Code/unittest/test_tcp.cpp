#include <gtest/gtest.h>

#include "TCPSocket.h"

static constexpr uint16_t PORT_NUM = 8080;

static std::pair<TCPSocket, TCPSocket> get_client_and_server() {
  std::pair<TCPSocket, TCPSocket> sockets;

  EXPECT_TRUE(sockets.first.create());
  EXPECT_TRUE(sockets.first.bind(PORT_NUM));
  EXPECT_TRUE(sockets.first.listen(1));

  EXPECT_TRUE(sockets.second.create());
  EXPECT_TRUE(sockets.second.connect("127.0.0.1", PORT_NUM));

  std::optional<TCPSocket> server_messager = sockets.first.accept();
  EXPECT_TRUE(server_messager.has_value());

  sockets.first = std::move(server_messager.value());

  return sockets;
}

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

TEST(TCPTest, TestSplitSends) {
  std::pair<TCPSocket, TCPSocket> sockets = get_client_and_server();

  EXPECT_EQ(sockets.second.send("Happy "), strlen("Happy "));
  EXPECT_EQ(sockets.second.send("Birthday!"), strlen("Birthday!"));
  EXPECT_EQ(sockets.first.recv(), "Happy Birthday!");
}

TEST(TCPTest, TestTimeout) {
  std::pair<TCPSocket, TCPSocket> sockets = get_client_and_server();

  EXPECT_TRUE(sockets.first.setTimeout<SO_RCVTIMEO>(1));
  EXPECT_EQ(sockets.first.recv(), "");

  EXPECT_EQ(sockets.second.send("Happy Birthday!"), strlen("Happy Birthday!"));
  EXPECT_EQ(sockets.first.recv(), "Happy Birthday!");
}

TEST(TCPTest, TestSockStream) {
  std::pair<TCPSocket, TCPSocket> sockets = get_client_and_server();
  EXPECT_TRUE(sockets.first.setTimeout<SO_RCVTIMEO>(1));

  SocketStream ss(sockets.first);

  std::string line1 = "Once upon a midnight dreary, while I pondered weak and weary";
  std::string line2 = "\nOver many a quaint and curious volume of forgotten lore";
  EXPECT_EQ(sockets.second.send(line1), line1.length());

  std::string combined = "";
  while (ss.hasNext()) {
    std::string word = ss.nextWord();
    if (!combined.empty()) { combined += " "; }
    combined += word;
    if (word == "while") { EXPECT_EQ(sockets.second.send(line2), line2.length()); }
  }

  EXPECT_EQ(combined, line1 + ' ' + line2.substr(1));

  EXPECT_EQ(ss.str(), line1 + line2);
}
