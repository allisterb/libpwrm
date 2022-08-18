#include "reporting.h"

using json = nlohmann::json;


void report(const string* base_file,  std::vector<string> devices,  std::map<string, double> measurements)
{
    info("Loading base reporting data file {}...", *base_file);
    std::ifstream f(*base_file);
    string str = "";
    if(f) {
      ostringstream ss;
      ss << f.rdbuf(); 
      f.close();
      str = ss.str();
    }
    else {
      error("Could not open file {} for reading.", *base_file);
      return;
    }
    json data = json::parse(str);

    
    time_t now = time(0);
    string dt = ctime(&now);
    data["date"] = dt;
    data["devices"] = devices;
    data["measurements"] = measurements;
    std::string s = data.dump(); 
    debug("JSON data document:{}", s);
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