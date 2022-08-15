#ifndef __INCLUDE_GUARD_REPORTING_H
#define __INCLUDE_GUARD_REPORTING_H

#include "../lib.h"
#include "../json.hpp"
#include <fstream>

#include "../devices/device.h"

void report(const string* base_file, std::map<string, double> measurements);

#endif