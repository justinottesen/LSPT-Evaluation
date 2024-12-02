#include <gtest/gtest.h>

#include "TCPSocket.h"
#include "TestUtil.hpp"

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

  EXPECT_TRUE(client.send("Hello Server!"));
  EXPECT_EQ(server_messager.value().recv(), "Hello Server!");

  EXPECT_TRUE(server_messager.value().send("Hello Client!"));
  EXPECT_EQ(client.recv(), "Hello Client!");
}

TEST(TCPTest, TestSplitSends) {
  std::pair<TCPSocket, TCPSocket> sockets = get_server_and_client(PORT_NUM);

  EXPECT_TRUE(sockets.second.send("Happy "));
  EXPECT_TRUE(sockets.second.send("Birthday!"));
  EXPECT_EQ(sockets.first.recv(), "Happy Birthday!");
}

TEST(TCPTest, TestTimeout) {
  std::pair<TCPSocket, TCPSocket> sockets = get_server_and_client(PORT_NUM);

  EXPECT_TRUE(sockets.first.setTimeout<SO_RCVTIMEO>(100));

  int sockfd = sockets.first.fd();
  struct timeval tv{.tv_sec = 0, .tv_usec = 0};
  socklen_t len = sizeof(tv);
  if (getsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, &len) == -1) {
    LOG(ERROR) << "Getsockopt failed: " << my_strerror(errno);
  };
  LOG(INFO) << "Timeout: " << tv.tv_sec << "." << tv.tv_usec;
  EXPECT_EQ(tv.tv_usec, 100 * 1000);

  EXPECT_EQ(sockets.first.recv(), "");

  EXPECT_TRUE(sockets.second.send("Happy Birthday!"));
  EXPECT_EQ(sockets.first.recv(), "Happy Birthday!");
}

TEST(TCPTest, TestSockStream) {
  std::pair<TCPSocket, TCPSocket> sockets = get_server_and_client(PORT_NUM);
  EXPECT_TRUE(sockets.first.setTimeout<SO_RCVTIMEO>(100));

  SocketStream ss(sockets.first);

  std::string line1 = "Once upon a midnight dreary, while I pondered weak and weary";
  std::string line2 = "\nOver many a quaint and curious volume of forgotten lore";
  EXPECT_TRUE(sockets.second.send(line1));

  std::string combined = "";
  while (ss.hasNext()) {
    std::string word = ss.nextWord();
    if (!combined.empty()) { combined += " "; }
    combined += word;
    if (word == "while") { EXPECT_TRUE(sockets.second.send(line2)); }
  }

  EXPECT_EQ(combined, line1 + ' ' + line2.substr(1));

  EXPECT_EQ(ss.str(), line1 + line2);
}
