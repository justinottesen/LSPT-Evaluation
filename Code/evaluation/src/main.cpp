#include <sqlite3.h>

#include <exception>
#include <iomanip>
#include <iostream>
#include <nlohmann/json.hpp>
#include <numbers>

using namespace std;
using json = nlohmann::json;

int main() {
  try {
    // Pointer to SQLite connection
    sqlite3* database = nullptr;

    // Save the connection result
    int exit = 0;
    exit     = sqlite3_open("example.db", &database);

    // Test if there was an error
    if (exit != 0) {
      cout << "DB Open Error: " << sqlite3_errmsg(database) << '\n';

    } else {
      cout << "Opened Database Successfully!" << '\n';
    }

    // Close the connection
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
    std::cout << std::setw(4) << json_example << '\n';
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
  }

  return 0;
}
