
#include <unistd.h>

#include <array>
#include <cerrno>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <string>

#include "HTTPServer.h"
#include "Logger.h"
#include "Util.h"

// Default values for arguments
static constexpr uint16_t DEFAULT_LISTENER_PORT = 8080;
static constexpr int      DEFAULT_BACKLOG_SIZE  = 10;

// Clang tidy hates getopt so it is a bit messy here
// NOLINTBEGIN
#include <getopt.h>

// Command line option info
constexpr const char*   short_options  = ":p:b:";
constexpr struct option long_options[] = {
    {   "port", required_argument, 0, 'p'},
    {"backlog", required_argument, 0, 'b'},
    {        0,                 0, 0,   0}
};

// Extern variable declarations
extern char* optarg;
extern int   optind;
extern int   opterr;
extern int   optopt;

// Graceful Shutdown handling

namespace {

std::array<int, 2> shutdown_pipe;

void close_shutdown_pipe() {
  LOG(TRACE) << "Closing shutdown pipe (fds: " << shutdown_pipe[0] << " " << shutdown_pipe[1]
             << ")";
  if (close(shutdown_pipe[0]) == -1) {
    LOG(WARN) << "Unable to close shutdown_pipe[0]: " << my_strerror(errno);
  }
  if (close(shutdown_pipe[1]) == -1) {
    LOG(WARN) << "Unable to close shutdown_pipe[1]: " << my_strerror(errno);
  }
}

void signal_handler(int signum) {
  if (signum == SIGINT) {
    LOG(INFO) << "SIGINT received, sending shutdown to HTTP server...";
    write(shutdown_pipe[1], "1", 1);
  }
}

bool set_up_signal_handling() {
  // Create pipe for signal handling
  if (pipe(shutdown_pipe.data()) == -1) {
    LOG(CRITICAL) << "Unable to create shutdown pipe: " << my_strerror(errno);
    return false;
  }
  LOG(TRACE) << "Opened shutdown pipe (fds: " << shutdown_pipe[0] << " " << shutdown_pipe[1] << ")";
  if (atexit(close_shutdown_pipe) != 0) {
    // Not a huge deal, just won't get cleaned up
    LOG(WARN) << "Unable to register `close_shutdown_pipe` at program exit";
  }

  // Register signal handler for SIGINT
  struct sigaction action{};
  action.sa_handler = signal_handler;
  sigemptyset(&action.sa_mask);
  action.sa_flags = 0;

  if (sigaction(SIGINT, &action, nullptr) == -1) {
    LOG(CRITICAL) << "Unable to register signal handler for shutdown: " << my_strerror(errno);
    return false;
  }
  return true;
}

}    // namespace
// NOLINTEND

int main(int argc, char* argv[]) {
  // Enable logging
  Logger::addConsole(TRACE);
  Logger::addFile("log/info.log", INFO);
  Logger::addFile("log/trace.log", TRACE);

  // Set default values
  uint16_t listener_port = DEFAULT_LISTENER_PORT;
  int      backlog_size  = DEFAULT_BACKLOG_SIZE;

  // Read command line options
  int option = -1;
  try {
    // NOLINTNEXTLINE
    while ((option = getopt_long(argc, argv, short_options,
                                 static_cast<const struct option*>(long_options), nullptr))
           != -1) {
      LOG(DEBUG) << "Read option " << static_cast<char>(option) << " with arg " << optarg;
      switch (option) {
        case 'p': listener_port = std::stoul(optarg); continue;
        case 'b':
          backlog_size = std::stoi(optarg);
          if (backlog_size < 1) {
            LOG(CRITICAL) << "Negative value provided for backlog size (" << optarg << " -> "
                          << backlog_size << "), must be positive";
          }
          continue;
        case '?':
          LOG(CRITICAL) << "Command line option: " << static_cast<char>(optopt) << " is invalid";
          return EXIT_FAILURE;
        case ':':
          LOG(CRITICAL) << "Command line option " << static_cast<char>(optopt)
                        << " missing required argument";
          return EXIT_FAILURE;
        default:
          LOG(CRITICAL) << "This should not happen so I am crashing. Option value was " << option
                        << " with arg " << optarg << " (optopt: " << optopt << ", opterr" << opterr
                        << ")";
          return EXIT_FAILURE;
      }
    }
  } catch (const std::exception& e) {
    LOG(CRITICAL) << "Caught exception while handling command line arguments: " << e.what();
    return EXIT_FAILURE;
  }

  if (!set_up_signal_handling()) {
    LOG(CRITICAL) << "Falied to set up signal handling";
    return EXIT_FAILURE;
  }

  // Set up HTTP server
  HTTPServer server(listener_port, backlog_size);
  return (server.run(shutdown_pipe[0]) ? EXIT_SUCCESS : EXIT_FAILURE);
}
