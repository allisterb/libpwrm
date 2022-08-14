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

void nvmlAPIRun();
void nvmlAPIEnd();
void *powerPollingFunc(void *ptr);
int getNVMLError(nvmlReturn_t resultToCheck);

void print_nv_devices_info();
#endif