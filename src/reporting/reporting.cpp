#include "reporting.h"

using json = nlohmann::json;

void report(std::vector<string> devices,  std::map<string, double> measurements, const string* wt_user, const string* wt_pass)
{
    string str = "";   
    json data = json::parse(str);
    time_t now = time(0);
    string dt = ctime(&now);
    data["date"] = dt;
    data["devices"] = devices;
    data["measurements"] = measurements;
    std::string header = R"("header": { "family": "test"})";
    std::string body = R"(
    {
      "type": 0,
      "genesis": {
        "header": {
          "family": "test"
        },
        "Document": 
            )" + data.dump() + "} }";
    /*
    data = json::parse(body); 
    string k = "did:key:" + (*did);
    string controllers[] = {k};
    data["genesis"]["header"]["controllers"] = controllers;
    debug("JSON data document:{}", data.dump());

    // HTTPS
    httplib::Client client(*ceramic_url);
    info("Posting TileDocument using Ceramic API at {}...", *ceramic_url);
    if (auto res = client.Post("/api/v0/streams", data.dump(), "application/json")) {
      if (res->status == 200) {
        info("API response: {}", res->body);
      }
    } else {
      auto err = res.error();
      std::cout << "HTTP error: " << httplib::to_string(err) << std::endl;
      */
  
    //r.
    //json data = R"(
  //{
  //  "location": {
  //      "lat": 0.0,
  //      "long": 0.0
  //  },
  //  "devices":[]
  //}
//)"_json;
//f.close();

}