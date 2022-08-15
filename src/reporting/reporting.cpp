#include "../lib.h"
#include "../json.hpp"
#include <fstream>

using json = nlohmann::json;


void report()
{
    std::ifstream f("example.json");
    json data = json::parse(f);

}