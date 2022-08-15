#include <stdexcept>
#include "nvml_power.h"

unsigned int deviceCount = 0;
char deviceNameStr[64];

nvmlReturn_t nvmlResult;
nvmlDevice_t nvmlDeviceID;
nvmlPciInfo_t nvmPCIInfo;
nvmlEnableState_t pmmode;
nvmlComputeMode_t computeMode;

bool nvmlInitialized = false;
pthread_t powerPollThread;

bool init_nvml()
{
	if (nvmlInitialized)
	{
		throw new std::runtime_error("NVML is already initialized.");
	}
	nvmlResult = nvmlInit();
	if (NVML_SUCCESS != nvmlResult) {
		error("NVML init fail: {}", nvmlErrorString(nvmlResult));
		return false;
	}
	else {
		nvmlInitialized = true;
		return true;
	}
}

bool shutdown_nvml()
{
	if (!nvmlInitialized)
	{
		throw new std::runtime_error("NVML is not initialized.");
	}
	nvmlResult = nvmlShutdown();
	if (NVML_SUCCESS != nvmlResult) {
		error("Failed to shut down NVML: {}", nvmlErrorString(nvmlResult));
		return false;
	}
	else {
		return true;
	}
}

void print_nv_devices_info() {
	info ("Printing NVIDIA GPU devices info...");
	nvmlResult = nvmlInit();
	if (NVML_SUCCESS != nvmlResult)
	{
		error("NVML init fail: {}", nvmlErrorString(nvmlResult));
		return;
	}

	nvmlResult = nvmlDeviceGetCount(&deviceCount);
	if (NVML_SUCCESS != nvmlResult)
	{
		error("Failed to query device count: {}", nvmlErrorString(nvmlResult));
		return;
	}

	for (uint i = 0; i < deviceCount; i++)
	{
		nvmlResult = nvmlDeviceGetHandleByIndex(i, &nvmlDeviceID);
		if (NVML_SUCCESS != nvmlResult)
		{
			error("Failed to get handle for device {}: {}", i, nvmlErrorString(nvmlResult));
			return;
		}

		nvmlResult = nvmlDeviceGetName(nvmlDeviceID, deviceNameStr, sizeof(deviceNameStr)/sizeof(deviceNameStr[0]));
		if (NVML_SUCCESS != nvmlResult)
		{
			error("Failed to get name of device {}: {}", i, nvmlErrorString(nvmlResult));
			return;
		}

		// Get PCI information of the device.
		nvmlResult = nvmlDeviceGetPciInfo(nvmlDeviceID, &nvmPCIInfo);
		if (NVML_SUCCESS != nvmlResult)
		{
			error("Failed to get PCI info of device {}: {}", i, nvmlErrorString(nvmlResult));
			return;
		}

		// Get the compute mode of the device which indicates CUDA capabilities.
		nvmlResult = nvmlDeviceGetComputeMode(nvmlDeviceID, &computeMode);
		bool cuda_capable = nvmlResult != NVML_ERROR_NOT_SUPPORTED;
		nvmlResult = nvmlDeviceGetPowerManagementMode(nvmlDeviceID, &pmmode);
		bool pmm_enabled = pmmode == NVML_FEATURE_ENABLED;
		info("GPU Device #{}: {}. CUDA capable: {}. Power mgmt mode enabled: {}.", i, deviceNameStr, cuda_capable, pmm_enabled);
	}

	nvmlResult = nvmlShutdown();
	if (NVML_SUCCESS != nvmlResult)
	{
		error("Failed to shut down NVML: {}", nvmlErrorString(nvmlResult));
		return;
	}
}

bool measure_nv_device_power(int devid, int time, unsigned int* r) {
	if (!nvmlInitialized)
	{
		error("NVML is not initialized.");
		return false;
	}
	nvmlResult = nvmlDeviceGetHandleByIndex(devid, &nvmlDeviceID);
	if (nvmlResult != NVML_SUCCESS) {
		error("Could not get handle for device id {}: {}.", devid, nvmlErrorString(nvmlResult));
		return false;
	}
	nvmlResult = nvmlDeviceGetName(nvmlDeviceID, deviceNameStr, sizeof(deviceNameStr)/sizeof(deviceNameStr[0]));
	if (NVML_SUCCESS != nvmlResult) {
		error("Failed to get name of device {}: {}", devid, nvmlErrorString(nvmlResult));
	}
	else {
		info("GPU Device #{}: {}.", devid, deviceNameStr);
	}
	nvmlResult = nvmlDeviceGetPowerUsage(nvmlDeviceID, r);
	return NVML_SUCCESS == nvmlResult;
}