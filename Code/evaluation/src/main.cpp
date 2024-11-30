#include <sqlite3.h>

#include <exception>
#include <nlohmann/json.hpp>
#include <numbers>

#include "Logger.h"

using namespace std;
using json = nlohmann::json;

int main() {
  Logger::addConsole();
  // Logger::addFile("log/info.log");
  // Logger::addFile("log/trace.log", TRACE);

  try {
    // Pointer to SQLite connection
    sqlite3* database = nullptr;

    // Save the connection result
    int exit = 0;
    exit     = sqlite3_open("example.db", &database);

    // Test if there was an error
    if (exit != 0) {
      LOG(ERROR) << "DB Open Error: " << sqlite3_errmsg(database);

    } else {
      LOG(INFO) << "Opened Database Successfully!";
    }

    // Close the connection
    LOG(DEBUG) << "Closing database";
    sqlite3_close(database);

    // create a JSON object
    // clang-format off
    json json_example =
    {
      {"pi", std::numbers::pi},
      {"happy", true},
      {"name", "Niels"},
      {"nothing", nullptr},
      {
        "answer", {
          {"everything", 42} // NOLINT(*magic-numbers)
        }
      },
      {"list", {1, 0, 2}},
      {
        "object", {
          {"currency", "USD"},
          {"value", 42.99} // NOLINT(*magic-numbers)
        }
      }
    };
    // clang-format on

    // add new values
    json_example["new"]["key"]["value"] = {"another", "list"};

    // count elements
    auto json_size       = json_example.size();
    json_example["size"] = json_size;

    // pretty print with indent of 4 spaces
    LOG(INFO) << json_example;
  } catch (const std::exception& e) {
    LOG(CRITICAL) << e.what();
    return 1;
  }

  LOG(CRITICAL) << "Not actually critical just testing colors";

  LOG(INFO) << "Trying with\nmultiple lines";

  // Empty log message
  LOG(INFO);
  return 0;
}
