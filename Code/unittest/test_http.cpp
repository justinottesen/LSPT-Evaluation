#include <gtest/gtest.h>

#include "HTTPServer.h"
#include "TestUtil.hpp"

static constexpr uint16_t PORT_NUM = 8080;

TEST(ParseHTTPTest, ParseEmptyHTTPRequest) {
  std::pair<TCPSocket, TCPSocket> sockets = get_server_and_client(PORT_NUM);
  EXPECT_TRUE(sockets.first.setTimeout<SO_RCVTIMEO>(100));

  std::optional<HTTPRequest> request = HTTPWorker::parseRequest(sockets.first);
  EXPECT_FALSE(request.has_value());
}

TEST(ParseHTTPTest, ParseExampleGetAutofill) {
  std::pair<TCPSocket, TCPSocket> sockets = get_server_and_client(PORT_NUM);
  EXPECT_TRUE(sockets.first.setTimeout<SO_RCVTIMEO>(100));

  std::string getAutofill =
      "GET /v0/GetAutofill HTTP/1.1\r\n"
      "Num-Suggestions: 10\r\n"
      "Partial-Query: How do I make\r\n"
      "\r\n";
  sockets.second.send(getAutofill);

  std::optional<HTTPRequest> request_op = HTTPWorker::parseRequest(sockets.first);
  EXPECT_TRUE(request_op.has_value());

  HTTPRequest request = request_op.value();

  const std::unordered_map<std::string, std::string> header_map = {
      {"num-suggestions",            "10"},
      {  "partial-query", "How do I make"}
  };
  EXPECT_EQ(request.method, HTTPRequest::GET);
  EXPECT_EQ(request.resource, "/v0/GetAutofill");
  EXPECT_EQ(request.version, "HTTP/1.1");
  EXPECT_EQ(request.headers, header_map);
  EXPECT_TRUE(request.body.empty());
}

TEST(ParseHTTPTest, ParseExampleReportSearchResults) {
  std::pair<TCPSocket, TCPSocket> sockets = get_server_and_client(PORT_NUM);
  EXPECT_TRUE(sockets.first.setTimeout<SO_RCVTIMEO>(100));

  std::string getAutofill =
      "POST /v0/ReportSearchResults HTTP/1.1\r\n"
      "Content-Type: application/json\r\n"
      "Content-Length: 176\r\n"
      "\r\n"
      "{\r\n"
      "  \"query_ID\": 1234\r\n"
      "  \"raw_query\": \"How do I do a thing\"\r\n"
      "  \"results\": [ \"link1\", \"link2\", \"link3\" ]\r\n"
      "  \"clicked\": 1\r\n"
      "  \"query_timestamp\": \"Tue, 29 Oct 2024 16:56:32 GMT\"\r\n"
      "}";
  sockets.second.send(getAutofill);

  std::optional<HTTPRequest> request_op = HTTPWorker::parseRequest(sockets.first);
  EXPECT_TRUE(request_op.has_value());

  HTTPRequest request = request_op.value();

  const std::unordered_map<std::string, std::string> header_map = {
      {  "content-type", "application/json"},
      {"content-length",              "176"}
  };
  EXPECT_EQ(request.method, HTTPRequest::POST);
  EXPECT_EQ(request.resource, "/v0/ReportSearchResults");
  EXPECT_EQ(request.version, "HTTP/1.1");
  EXPECT_EQ(request.headers, header_map);
  EXPECT_EQ(request.body, getAutofill.substr(94));
}

TEST(HTTPTest, ToStringSendAndReceive) {
  std::pair<TCPSocket, TCPSocket> sockets = get_server_and_client(PORT_NUM);
  EXPECT_TRUE(sockets.first.setTimeout<SO_RCVTIMEO>(100));

  HTTPRequest client_request(HTTPRequest::POST, "/v0/ReportSearchResults",
                             {
                                 {       "query_ID",                            1234},
                                 {      "raw_query",           "How do I do a thing"},
                                 {        "results",     {"link1", "link2", "link3"}},
                                 {        "clicked",                               1},
                                 {"query_timestamp", "Tue, 29 Oct 2024 16:56:32 GMT"}
  });

  sockets.second.send(client_request);

  std::optional<HTTPRequest> request_op = HTTPWorker::parseRequest(sockets.first);
  EXPECT_TRUE(request_op.has_value());

  HTTPRequest server_request = request_op.value();
  EXPECT_EQ(client_request.method, server_request.method);
  EXPECT_EQ(client_request.resource, server_request.resource);
  EXPECT_EQ(client_request.version, server_request.version);
  EXPECT_EQ(client_request.body, server_request.body);
}

TEST(HTTPTest, ResponseConstructors) {
  HTTPResponse resp = HTTPResponse::makeErrorResponse(400, "Bad Request", "Error parsing request");

  nlohmann::json body = {
      {  "error",           "Bad Request"},
      {"message", "Error parsing request"}
  };
  HTTPResponse resp2(400, "Bad Request", body);

  HTTPResponse resp3(400, "Bad Request");
  resp3.body    = body.dump(2);
  resp3.headers = {
      {"Content-Length", std::to_string(resp3.body.length())},
      {  "Content-Type",                  "application/json"}
  };

  EXPECT_EQ(resp.version, resp2.version);
  EXPECT_EQ(resp.code, resp2.code);
  EXPECT_EQ(resp.status, resp2.status);
  EXPECT_EQ(resp.headers, resp2.headers);
  EXPECT_EQ(resp.body, resp2.body);

  EXPECT_EQ(resp2.version, resp3.version);
  EXPECT_EQ(resp2.code, resp3.code);
  EXPECT_EQ(resp2.status, resp3.status);
  EXPECT_EQ(resp2.headers, resp3.headers);
  EXPECT_EQ(resp2.body, resp3.body);
}

class ShutdownPipeWrapper {
 public:
  bool init() {
    if (initialized) {
      LOG(DEBUG) << "Already InitializeD";
      return true;
    }
    if (pipe(shutdown_pipe.data()) == -1) {
      LOG(ERROR) << "Unable to open shutdown pipe: " << my_strerror(errno);
      return false;
    }
    LOG(DEBUG) << "Initialized shutdown pipe";
    return initialized = true;
  }

  bool shutdown() {
    if (!initialized) {
      LOG(WARN) << "Called shutdown on uninitialized pipe";
      return false;
    }
    int ret = write(shutdown_pipe[1], "1", 1);
    if (ret == -1) {
      LOG(ERROR) << "Unable to write to shutdown pipe: " << my_strerror(errno);
    } else if (ret == 0) {
      LOG(ERROR) << "Wrote 0 bytes to shutdown pipe";
    } else {
      LOG(INFO) << "Sent shutdown on pipe";
    }
    return ret == 1;
  }

  int get_fd() const { return shutdown_pipe[0]; }

  ~ShutdownPipeWrapper() {
    LOG(DEBUG) << "Closing shutdown pipe (fds: " << shutdown_pipe[0] << " " << shutdown_pipe[1]
               << ")";
    if (close(shutdown_pipe[0]) == -1) {
      LOG(WARN) << "Unable to close shutdown_pipe[0]: " << my_strerror(errno);
    }
    if (close(shutdown_pipe[1]) == -1) {
      LOG(WARN) << "Unable to close shutdown_pipe[1]: " << my_strerror(errno);
    }
  }

 private:
  bool               initialized   = false;
  std::array<int, 2> shutdown_pipe = {-1, -1};
};

TEST(HTTPTest, TestShutdown) {
  LOG(INFO) << "Creating Pipe";

  ShutdownPipeWrapper pipe;
  EXPECT_TRUE(pipe.init());

  LOG(INFO) << "Pipe Created";

  HTTPServer server(PORT_NUM, 1);

  LOG(INFO) << "Server created";

  auto run_server = [](HTTPServer& server, int shutdown_fd) -> int {
    LOG(INFO) << "Starting Thread";
    return server.run(shutdown_fd);
  };

  std::thread server_thread(run_server, std::ref(server), pipe.get_fd());

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  LOG(INFO) << "Sending Shutdown";
  EXPECT_TRUE(pipe.shutdown());

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  LOG(INFO) << "Thread should be joinable";
  EXPECT_TRUE(server_thread.joinable());

  LOG(INFO) << "Joining Thread";
  server_thread.join();
}

class HTTPServerWrapper {
 public:
  HTTPServerWrapper(uint16_t port_num, int backlog)
      : m_server(port_num, backlog) {}
  ~HTTPServerWrapper() { shutdown(); }

  bool init() { return m_pipe.init(); }

  void run() {
    auto run_server = [](HTTPServer& server, int shutdown_fd) -> int {
      LOG(INFO) << "Starting Thread";
      return server.run(shutdown_fd);
    };

    m_server_thread = std::thread(run_server, std::ref(m_server), m_pipe.get_fd());
  }

  bool shutdown() {
    if (!m_pipe.shutdown()) { return false; }
    m_server_thread.join();
    return true;
  }

 private:
  ShutdownPipeWrapper m_pipe;
  HTTPServer          m_server;

  std::thread m_server_thread;
};

TEST(HTTPTest, TestAccept) {
  HTTPServerWrapper server(PORT_NUM, 1);

  EXPECT_TRUE(server.init());

  server.run();

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  TCPSocket client;
  EXPECT_TRUE(client.create());
  EXPECT_TRUE(client.connect("127.0.0.1", PORT_NUM));
}

TEST(HTTPTest, BadlyFormattedRequest) {
  HTTPServerWrapper server(PORT_NUM, 1);

  EXPECT_TRUE(server.init());

  server.run();

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  TCPSocket client;
  EXPECT_TRUE(client.create());
  EXPECT_TRUE(client.connect("127.0.0.1", PORT_NUM));

  EXPECT_TRUE(client.send("Not an HTTP Request"));

  std::optional<HTTPResponse> response_opt = HTTPWorker::parseResponse(client);

  EXPECT_TRUE(response_opt.has_value());

  HTTPResponse& response = response_opt.value();

  EXPECT_EQ(response.version, "HTTP/1.1");
  EXPECT_EQ(response.code, 400u);
  EXPECT_EQ(response.status, "Bad Request");

  nlohmann::json expected_json = {
      {  "error",            "Bad Request"},
      {"message", "Error parsing request."}
  };
  EXPECT_EQ(response.body, expected_json.dump(2));
}

TEST(HTTPTest, BadHTTPVersion) {
  HTTPServerWrapper server(PORT_NUM, 1);

  EXPECT_TRUE(server.init());

  server.run();

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  TCPSocket client;
  EXPECT_TRUE(client.create());
  EXPECT_TRUE(client.connect("127.0.0.1", PORT_NUM));

  HTTPRequest request(HTTPRequest::GET, "/v0/GetQueryID");
  request.version = "HTTP/1.0";

  EXPECT_TRUE(client.send(request));

  std::optional<HTTPResponse> response_opt = HTTPWorker::parseResponse(client);

  EXPECT_TRUE(response_opt.has_value());

  HTTPResponse& response = response_opt.value();

  EXPECT_EQ(response.version, "HTTP/1.1");
  EXPECT_EQ(response.code, 505u);
  EXPECT_EQ(response.status, "HTTP Version Not Supported");

  nlohmann::json expected_json = {
      {  "error", "HTTP Version Not Supported"},
      {"message",     "HTTP/1.1 Must be Used."}
  };
  EXPECT_EQ(response.body, expected_json.dump(2));
}

TEST(HTTPTest, BodyNoContentLength) {
  HTTPServerWrapper server(PORT_NUM, 1);

  EXPECT_TRUE(server.init());

  server.run();

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  TCPSocket client;
  EXPECT_TRUE(client.create());
  EXPECT_TRUE(client.connect("127.0.0.1", PORT_NUM));

  HTTPRequest request(HTTPRequest::POST, "/v0/ReportSearchResults",
                      {
                          {       "query_ID",                            1234},
                          {      "raw_query",                     "How do I?"},
                          {        "results",     {"link1", "link2", "link3"}},
                          {        "clicked",                               1},
                          {"query_timestamp", "Tue, 29 Oct 2024 16:56:32 GMT"}
  });
  request.headers.erase("Content-Length");

  EXPECT_TRUE(client.send(request));

  std::optional<HTTPResponse> response_opt = HTTPWorker::parseResponse(client);

  EXPECT_TRUE(response_opt.has_value());

  HTTPResponse& response = response_opt.value();

  EXPECT_EQ(response.version, "HTTP/1.1");
  EXPECT_EQ(response.code, 411u);
  EXPECT_EQ(response.status, "Length Required");

  nlohmann::json expected_json = {
      {  "error",                                                     "Length Required"},
      {"message", "Content-Length header must be specified when sending a request body"}
  };
  EXPECT_EQ(response.body, expected_json.dump(2));
}

TEST(HTTPTest, BodyBadContentLength) {
  HTTPServerWrapper server(PORT_NUM, 1);

  EXPECT_TRUE(server.init());

  server.run();

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  TCPSocket client;
  EXPECT_TRUE(client.create());
  EXPECT_TRUE(client.connect("127.0.0.1", PORT_NUM));

  HTTPRequest request(HTTPRequest::POST, "/v0/ReportSearchResults",
                      {
                          {       "query_ID",                            1234},
                          {      "raw_query",                     "How do I?"},
                          {        "results",     {"link1", "link2", "link3"}},
                          {        "clicked",                               1},
                          {"query_timestamp", "Tue, 29 Oct 2024 16:56:32 GMT"}
  });
  request.headers["Content-Length"] = "ABCDEFG";

  EXPECT_TRUE(client.send(request));

  std::optional<HTTPResponse> response_opt = HTTPWorker::parseResponse(client);

  EXPECT_TRUE(response_opt.has_value());

  HTTPResponse& response = response_opt.value();

  EXPECT_EQ(response.version, "HTTP/1.1");
  EXPECT_EQ(response.code, 400u);
  EXPECT_EQ(response.status, "Bad Request");

  nlohmann::json expected_json = {
      {  "error",                                   "Bad Request"},
      {"message", "Specified content length (ABCDEFG) is invalid"}
  };
  EXPECT_EQ(response.body, expected_json.dump(2));
}

TEST(HTTPTest, BodyWrongContentLength) {
  HTTPServerWrapper server(PORT_NUM, 1);

  EXPECT_TRUE(server.init());

  server.run();

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  TCPSocket client;
  EXPECT_TRUE(client.create());
  EXPECT_TRUE(client.connect("127.0.0.1", PORT_NUM));

  HTTPRequest request(HTTPRequest::POST, "/v0/ReportSearchResults",
                      {
                          {       "query_ID",                            1234},
                          {      "raw_query",                     "How do I?"},
                          {        "results",     {"link1", "link2", "link3"}},
                          {        "clicked",                               1},
                          {"query_timestamp", "Tue, 29 Oct 2024 16:56:32 GMT"}
  });

  request.headers["Content-Length"] = "69";

  EXPECT_TRUE(client.send(request));

  std::optional<HTTPResponse> response_opt = HTTPWorker::parseResponse(client);

  EXPECT_TRUE(response_opt.has_value());

  HTTPResponse& response = response_opt.value();

  EXPECT_EQ(response.version, "HTTP/1.1");
  EXPECT_EQ(response.code, 400u);
  EXPECT_EQ(response.status, "Bad Request");

  nlohmann::json expected_json = {
      {  "error",                                                "Bad Request"},
      {"message", "Provided content length 69 does not match actual content length "
 + std::to_string(request.body.length())                     }
  };
  EXPECT_EQ(response.body, expected_json.dump(2));
}

TEST(HTTPTest, BadResource) {
  HTTPServerWrapper server(PORT_NUM, 1);

  EXPECT_TRUE(server.init());

  server.run();

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  TCPSocket client;
  EXPECT_TRUE(client.create());
  EXPECT_TRUE(client.connect("127.0.0.1", PORT_NUM));

  HTTPRequest request(HTTPRequest::GET, "/Fake/ResourceDoesntExist");

  EXPECT_TRUE(client.send(request));

  std::optional<HTTPResponse> response_opt = HTTPWorker::parseResponse(client);

  EXPECT_TRUE(response_opt.has_value());

  HTTPResponse& response = response_opt.value();

  EXPECT_EQ(response.version, "HTTP/1.1");
  EXPECT_EQ(response.code, 404u);
  EXPECT_EQ(response.status, "Not Found");

  nlohmann::json expected_json = {
      {  "error",                         "Not Found"},
      {"message", "Resource (API function) not found"}
  };
  EXPECT_EQ(response.body, expected_json.dump(2));
}
