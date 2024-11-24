#include <iostream> 
#include <sqlite3.h>

#include <nlohmann/json.hpp>

using namespace std;
using json = nlohmann::json;
  
int main() 
{ 
    // Pointer to SQLite connection
    sqlite3* db; 
    
    // Save the connection result
    int exit = 0;
    exit = sqlite3_open("example.db", &db); 
  
    // Test if there was an error
    if (exit) { 
        
        cout << "DB Open Error: " << sqlite3_errmsg(db) << endl; 
        
    } else {

        cout << "Opened Database Successfully!" << endl; 
    }
    
    // Close the connection
    sqlite3_close(db); 

    // create a JSON object
    json j =
    {
        {"pi", 3.141},
        {"happy", true},
        {"name", "Niels"},
        {"nothing", nullptr},
        {
            "answer", {
                {"everything", 42}
            }
        },
        {"list", {1, 0, 2}},
        {
            "object", {
                {"currency", "USD"},
                {"value", 42.99}
            }
        }
    };

    // add new values
    j["new"]["key"]["value"] = {"another", "list"};

    // count elements
    auto s = j.size();
    j["size"] = s;

    // pretty print with indent of 4 spaces
    std::cout << std::setw(4) << j << '\n';
    
    return (0); 
} 