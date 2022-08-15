#ifndef __INCLUDE_GUARD_NVML_POWER_H
#define __INCLUDE_GUARD_NVML_POWER_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <nvml.h>
#include <pthread.h>
#include <cuda_runtime.h>
#include <time.h>
#include <unistd.h>

#include "../lib.h"

int getNVMLError(nvmlReturn_t resultToCheck);
bool init_nvml();
bool shutdown_nvml();
void print_nv_devices_info();
bool measure_nv_device_power(int devid, int time, unsigned int* r);
#endif