#include "reporting.h"

using json = nlohmann::json;


void report(const string* base_file, std::map<string, double> measurements)
{
    info("Loading base reporting data file {}...", *base_file);
    std::ifstream f(*base_file);
    if (!f.good()) {
      error("Could not open file {} for reading.", *base_file);
      return;
    }
    json data = json::parse(f);

    
    time_t now = time(0);
    string dt = ctime(&now);
    data["date"] = dt;
    std::string s = data.dump(); 
    f.close();
    //json data = R"(
  //{
  //  "location": {
  //      "lat": 0.0,
  //      "long": 0.0
  //  },
  //  "devices":[]
  //}
//)"_json;
f.close();

}