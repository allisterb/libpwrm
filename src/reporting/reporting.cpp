#include "reporting.h"

using json = nlohmann::json;


void report(const string* base_file, std::map<string, double> measurements)
{
    info("Loading base reporting data file {}...", *base_file);
    std::ifstream f(*base_file);
    json data = json::parse(f);
    //json data = R"(
  //{
  //  "location": {
  //      "lat": 0.0,
  //      "long": 0.0
  //  },
  //  "devices":[]
  //}
//)"_json;

}