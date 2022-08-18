#ifndef __INCLUDE_GUARD_REPORTING_H
#define __INCLUDE_GUARD_REPORTING_H

#include <fstream>
#include <iostream>
#include <sstream>
#include <ctime>

#include "../lib.h"
#include "../json.hpp"
#include "../httplib.h"

void report(const string* base_file, std::vector<string> devices, std::map<string, double> measurements, const string* ceramic_url, const string* did);

#endif