
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <string>

#include "HTTPServer.h"
#include "Logger.h"

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

  // Set up HTTP server
  const HTTPServer server(listener_port, backlog_size);
  server.init();

  return EXIT_SUCCESS;
}
