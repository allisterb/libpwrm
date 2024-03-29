/* rapl_interface.cpp: rapl interface for power top implementation
 *
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 *
 * Author Name <Srinivas.Pandruvada@linux.intel.com>
 *
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <dirent.h>
#include "../../lib.h"
#include "rapl_interface.h"

#ifdef DEBUG
#define RAPL_DBG_PRINT printf
#define RAPL_ERROR_PRINT printf
#else
#define RAPL_DBG_PRINT(...)	((void) 0)
#define RAPL_ERROR_PRINT(...) ((void) 0)
#endif
#define RAPL_INFO_PRINT(format, m) fprintf(stderr, format, m)

#define MAX_TEMP_STR_SIZE	20

// RAPL interface
#define MSR_RAPL_POWER_UNIT	0x606
#define MSR_PKG_POWER_LIMIT	0x610

#define MSR_PKG_ENERY_STATUS	0x611
#define MSR_PKG_POWER_INFO	0x614
#define MSR_PKG_PERF_STATUS	0x613

#define MSR_DRAM_POWER_LIMIT	0x618
#define MSR_DRAM_ENERY_STATUS	0x619
#define MSR_DRAM_PERF_STATUS	0x61B
#define MSR_DRAM_POWER_INFO	0x61c

#define MSR_PP0_POWER_LIMIT	0x638
#define MSR_PP0_ENERY_STATUS	0x639
#define MSR_PP0_POLICY		0x63A
#define MSR_PP0_PERF_STATUS	0x63B

#define MSR_PP1_POWER_LIMIT	0x640
#define MSR_PP1_ENERY_STATUS	0x641
#define MSR_PP1_POLICY		0x642

#define PKG_DOMAIN_PRESENT	0x01
#define DRAM_DOMAIN_PRESENT	0x02
#define PP0_DOMAIN_PRESENT	0x04
#define PP1_DOMAIN_PRESENT	0x08

c_rapl_interface::c_rapl_interface(const char *dev_name, int cpu) :
	powercap_sysfs_present(false),
	powercap_core_path(),
	powercap_uncore_path(),
	powercap_dram_path(),
	first_cpu(cpu),
	measurment_interval(def_sampling_interval),
	last_pkg_energy_status(0.0),
	last_dram_energy_status(0.0),
	last_pp0_energy_status(0.0),
	last_pp1_energy_status(0.0)
{
	uint64_t value;
	int ret;
	string package_path;
	DIR *dir;
	struct dirent *entry;

	debug("Getting RAPL interface for CPU package {}...", dev_name);

	rapl_domains = 0;

	if (dev_name) {
		string base_path = "/sys/class/powercap/intel-rapl/";
		if ((dir = opendir(base_path.c_str())) != NULL) {
			while ((entry = readdir(dir)) != NULL) {
				string path = base_path + entry->d_name + "/name";
				string str = read_sysfs_string(path);
				if (str.length() > 0) {
					if (str == dev_name) {
						package_path = base_path + entry->d_name + "/";
						powercap_sysfs_present = true;
						rapl_domains |= PKG_DOMAIN_PRESENT;
						break;
					}
				}
			}
			closedir(dir);
		}
	}

	if (powercap_sysfs_present) {
		if ((dir = opendir(package_path.c_str())) != NULL) {
			while ((entry = readdir(dir)) != NULL) {
				string path = package_path + entry->d_name;
				string str = read_sysfs_string(path + "/name");
				if (str.length() > 0) {
					if (str == "core") {
						rapl_domains |= PP0_DOMAIN_PRESENT;
						powercap_core_path = path + "/";
					}
					else if (str == "dram") {
						rapl_domains |= DRAM_DOMAIN_PRESENT;
						powercap_dram_path = path + "/";
					}
					else if (str == "uncore") {
						rapl_domains |= PP1_DOMAIN_PRESENT;
						powercap_uncore_path = path + "/";
					}
				}
			}
			closedir(dir);
		}

		info("RAPL Using PowerCap Sysfs : Domain Mask {}.", rapl_domains);
		return;
	}

	// Fallback to using MSRs

	// presence of each domain
	// Check presence of PKG domain
	ret = read_msr(first_cpu, MSR_PKG_ENERY_STATUS, &value);
	if (ret > 0) {
		rapl_domains |= PKG_DOMAIN_PRESENT;
		debug("Domain : PKG present.");
	} else {
		debug("Domain : PKG Not present.");
	}

	// Check presence of DRAM domain
	ret = read_msr(first_cpu, MSR_DRAM_ENERY_STATUS, &value);
	if (ret > 0) {
		rapl_domains |= DRAM_DOMAIN_PRESENT;
		debug("Domain : DRAM present.");
	} else {
		debug("Domain : DRAM Not present.");
	}

	// Check presence of PP0 domain
	ret = read_msr(first_cpu, MSR_PP0_ENERY_STATUS, &value);
	if (ret > 0) {
		rapl_domains |= PP0_DOMAIN_PRESENT;
		debug("Domain : PP0 present.");
	} else {
		debug("Domain : PP0 Not present.");
	}

	// Check presence of PP1 domain
	ret = read_msr(first_cpu, MSR_PP1_ENERY_STATUS, &value);
	if (ret > 0) {
		rapl_domains |= PP1_DOMAIN_PRESENT;
		debug("Domain : PP1 present.");
	} else {
		debug("Domain : PP1 Not present.");
	}

	power_units = get_power_unit();
	energy_status_units = get_energy_status_unit();
	time_units = get_time_unit();

	debug("RAPL Domain mask: {}.", rapl_domains);
}

bool c_rapl_interface::pkg_domain_present()
{
	if ((rapl_domains & PKG_DOMAIN_PRESENT)) {
		return true;
	}

	return false;
}

bool c_rapl_interface::dram_domain_present()
{
	if ((rapl_domains & DRAM_DOMAIN_PRESENT)) {
		return true;
	}

	return false;
}

bool c_rapl_interface::pp0_domain_present()
{
	if ((rapl_domains & PP0_DOMAIN_PRESENT)) {
		return true;
	}

	return false;
}

bool c_rapl_interface::pp1_domain_present()
{
	if ((rapl_domains & PP1_DOMAIN_PRESENT)) {
		return true;
	}

	return false;
}

int c_rapl_interface::read_msr(int cpu, unsigned int idx, uint64_t *val)
{
	return ::read_msr(cpu, idx, val);
}

int c_rapl_interface::write_msr(int cpu, unsigned int idx, uint64_t val)
{
	return ::write_msr(cpu, idx, val);
}

int c_rapl_interface::get_rapl_power_unit(uint64_t *value)
{
	int ret;

	ret = read_msr(first_cpu, MSR_RAPL_POWER_UNIT, value);

	return ret;
}

double c_rapl_interface::get_power_unit()
{
	int ret;
	uint64_t value;

	ret = get_rapl_power_unit(&value);
	if(ret < 0)
	{
		return ret;
	}

	return (double) 1/pow((double)2, (double)(value & 0xf));
}

double c_rapl_interface::get_energy_status_unit()
{
	int ret;
	uint64_t value;

	ret = get_rapl_power_unit(&value);
	if(ret < 0)
	{
		return ret;
	}

	return (double)1/ pow((double)2, (double)((value & 0x1f00) >> 8));
}

double c_rapl_interface::get_time_unit()
{
	int ret;
	uint64_t value;

	ret = get_rapl_power_unit(&value);
	if(ret < 0)
	{
		return ret;
	}

	return (double)1 / pow((double)2, (double)((value & 0xf0000) >> 16));
}

int c_rapl_interface::get_pkg_energy_status(double *status)
{
	int ret;
	uint64_t value;

	if (!pkg_domain_present()) {
		return -1;
	}

	ret = read_msr(first_cpu, MSR_PKG_ENERY_STATUS, &value);
	if(ret < 0)
	{
		error("get_pkg_energy_status failed.");
		return ret;
	}

	*status = (double) (value & 0xffffffff) * get_energy_status_unit();

	return ret;
}

int c_rapl_interface::get_pkg_power_info(double *thermal_spec_power,
			double *max_power, double *min_power, double *max_time_window)
{
	int ret;
	uint64_t value;

	if (!pkg_domain_present()) {
		return -1;
	}
	ret = read_msr(first_cpu, MSR_PKG_POWER_INFO, &value);
	if(ret < 0)
	{
		error("get_pkg_power_info failed.");
		return ret;
	}
	*thermal_spec_power =  (value & 0x7FFF) * power_units;
	*min_power =  ((value & 0x7FFF0000) >> 16) * power_units;
	*max_power =  ((value & 0x7FFF00000000) >> 32) * power_units;
	*max_time_window = ((value & 0x3f000000000000)>>48) * time_units;

	return ret;
}

int c_rapl_interface::get_pkg_power_limit(uint64_t *value)
{
	int ret;

	if (!pkg_domain_present()) {
		return -1;
	}

	ret = read_msr(first_cpu, MSR_PKG_POWER_LIMIT, value);
	if(ret < 0)
	{
		RAPL_ERROR_PRINT("get_pkg_power_limit failed\n");
		return ret;
	}

	return ret;
}

int c_rapl_interface::set_pkg_power_limit(uint64_t value)
{
	int ret;

	if (!pkg_domain_present()) {
		return -1;
	}

	ret = write_msr(first_cpu, MSR_PKG_POWER_LIMIT, value);
	if(ret < 0)
	{
		RAPL_ERROR_PRINT("set_pkg_power_limit failed\n");
		return ret;
	}

	return ret;
}

int c_rapl_interface::get_dram_energy_status(double *status)
{
	int ret;
	uint64_t value;

	if (!dram_domain_present()) {
		return -1;
	}

	if (powercap_sysfs_present) {
		string str = read_sysfs_string(powercap_dram_path + "energy_uj");
		if (str.length() > 0) {
			*status =  atof(str.c_str()) / 1000000; // uj to Js
			return 0;
		}

		return -EINVAL;
	}

	ret = read_msr(first_cpu, MSR_DRAM_ENERY_STATUS, &value);
	if(ret < 0)
	{
		RAPL_ERROR_PRINT("get_dram_energy_status failed\n");
		return ret;
	}

	*status = (double) (value & 0xffffffff) * get_energy_status_unit();

	return ret;
}

int c_rapl_interface::get_dram_power_info(double *thermal_spec_power,
			double *max_power, double *min_power, double *max_time_window)
{
	int ret;
	uint64_t value;

	if (!dram_domain_present()) {
		return -1;
	}
	ret = read_msr(first_cpu, MSR_DRAM_POWER_INFO, &value);
	if(ret < 0)
	{
		RAPL_ERROR_PRINT("get_dram_power_info failed\n");
		return ret;
	}

	*thermal_spec_power =  (value & 0x7FFF) * power_units;
	*min_power =  ((value & 0x7FFF0000) >> 16) * power_units;
	*max_power =  ((value & 0x7FFF00000000) >> 32) * power_units;
	*max_time_window = ((value & 0x3f000000000000)>>48) * time_units;

	return ret;
}

int c_rapl_interface::get_dram_power_limit(uint64_t *value)
{
	int ret;

	if (!dram_domain_present()) {
		return -1;
	}

	ret = read_msr(first_cpu, MSR_DRAM_POWER_LIMIT, value);
	if(ret < 0)
	{
		RAPL_ERROR_PRINT("get_dram_power_limit failed\n");
		return ret;
	}

	return ret;
}

int c_rapl_interface::set_dram_power_limit(uint64_t value)
{
	int ret;

	if (!dram_domain_present()) {
		return -1;
	}

	ret = write_msr(first_cpu, MSR_DRAM_POWER_LIMIT, value);
	if(ret < 0)
	{
		RAPL_ERROR_PRINT("set_dram_power_limit failed\n");
		return ret;
	}

	return ret;
}

int c_rapl_interface::get_pp0_energy_status(double *status)
{
	int ret;
	uint64_t value;

	if (!pp0_domain_present()) {
		return -1;
	}

	if (powercap_sysfs_present) {
		string str = read_sysfs_string(powercap_core_path + "energy_uj");
		info("powercap {}", str);
		if (str.length() > 0) {
			*status = atof(str.c_str()) / 1000000; // uj to Js
			return 0;
		}

		return -EINVAL;
	}

	

	ret = read_msr(first_cpu, MSR_PP0_ENERY_STATUS, &value);
	if(ret < 0)
	{
		error("get_pp0_energy_status failed");
		return ret;
	}
	
	*status = (double) (value & 0xffffffff) * get_energy_status_unit();

	return ret;
}

int c_rapl_interface::get_pp0_power_limit(uint64_t *value)
{
	int ret;

	if (!pp0_domain_present()) {
		return -1;
	}

	ret = read_msr(first_cpu, MSR_PP0_POWER_LIMIT, value);
	if(ret < 0)
	{
		error("get_pp0_power_limit failed\n");
		return ret;
	}

	return ret;
}

int c_rapl_interface::set_pp0_power_limit(uint64_t value)
{
	int ret;

	if (!pp0_domain_present()) {
		return -1;
	}

	ret = write_msr(first_cpu, MSR_PP0_POWER_LIMIT, value);
	if(ret < 0)
	{
		error("set_pp0_power_limit failed\n");
		return ret;
	}

	return ret;
}

int c_rapl_interface::get_pp0_power_policy(unsigned int *pp0_power_policy)
{
	int ret;
	uint64_t value;

	if (!pp0_domain_present()) {
		return -1;
	}

	ret = read_msr(first_cpu, MSR_PP0_POLICY, &value);
	if(ret < 0)
	{
		error("get_pp0_power_policy failed\n");
		return ret;
	}

	*pp0_power_policy =  value & 0x0f;

	return ret;
}

int c_rapl_interface::get_pp1_energy_status(double *status)
{
	int ret;
	uint64_t value;

	if (!pp1_domain_present()) {
		return -1;
	}

	if (powercap_sysfs_present) {
		string str = read_sysfs_string(powercap_uncore_path + "energy_uj");
		if (str.length() > 0) {
			*status =  atof(str.c_str()) / 1000000; // uj to Js
			return 0;
		}

		return -EINVAL;
	}

	ret = read_msr(first_cpu, MSR_PP1_ENERY_STATUS, &value);
	if(ret < 0)
	{
		error("get_pp1_energy_status failed");
		return ret;
	}

	*status = (double) (value & 0xffffffff) * get_energy_status_unit();

	return ret;
}

int c_rapl_interface::get_pp1_power_limit(uint64_t *value)
{
	int ret;

	if (!pp1_domain_present()) {
		return -1;
	}

	ret = read_msr(first_cpu, MSR_PP1_POWER_LIMIT, value);
	if(ret < 0)
	{
		error("get_pp1_power_info failed");
		return ret;
	}

	return ret;
}

int c_rapl_interface::set_pp1_power_limit(uint64_t value)
{
	int ret;

	if (!pp1_domain_present()) {
		return -1;
	}

	ret = write_msr(first_cpu, MSR_PP1_POWER_LIMIT, value);
	if(ret < 0)
	{
		error("set_pp1_power_limit failed.");
		return ret;
	}

	return ret;
}

int c_rapl_interface::get_pp1_power_policy(unsigned int *pp1_power_policy)
{
	int ret;
	uint64_t value;

	if (!pp1_domain_present()) {
		return -1;
	}

	ret = read_msr(first_cpu, MSR_PP1_POLICY, &value);
	if(ret < 0)
	{
		error("get_pp1_power_policy failed.");
		return ret;
	}

	*pp1_power_policy =  value & 0x0f;

	return ret;
}

void c_rapl_interface::rapl_measure_energy()
{
#ifdef RAPL_TEST_MODE
	int ret;
	double energy_status;
	double thermal_spec_power;
	double max_power;
	double min_power;
	double max_time_window;
	double pkg_watts = 0;
	double dram_watts = 0;
	double pp0_watts = 0;
	double pp1_watts = 0;
	double pkg_joules = 0;
	double dram_joules = 0;
	double pp0_joules = 0;
	double pp1_joules = 0;

	get_pkg_power_info(&thermal_spec_power, &max_power, &min_power, &max_time_window);
	RAPL_DBG_PRINT("Pkg Power Info: Thermal spec %f watts, max %f watts, min %f watts, max time window %f seconds\n", thermal_spec_power, max_power, min_power, max_time_window);
	get_dram_power_info(&thermal_spec_power, &max_power, &min_power, &max_time_window);
	RAPL_DBG_PRINT("DRAM Power Info: Thermal spec %f watts, max %f watts, min %f watts, max time window %f seconds\n", thermal_spec_power, max_power, min_power, max_time_window);

	for (;;) {
		if (pkg_domain_present()) {
			ret = get_pkg_energy_status(&energy_status);
			if (last_pkg_energy_status == 0)
				last_pkg_energy_status = energy_status;
			if (ret > 0) {
				pkg_joules = energy_status;
				pkg_watts = (energy_status-last_pkg_energy_status)/measurment_interval;
			}
			last_pkg_energy_status = energy_status;
		}
		if (dram_domain_present()) {
			ret = get_dram_energy_status(&energy_status);
			if (last_dram_energy_status == 0)
				last_dram_energy_status = energy_status;
			if (ret > 0){
				dram_joules = energy_status;
				dram_watts = (energy_status-last_dram_energy_status)/measurment_interval;
			}
			last_dram_energy_status = energy_status;
		}
		if (pp0_domain_present()) {
			ret = get_pp0_energy_status(&energy_status);
			if (last_pp0_energy_status == 0)
				last_pp0_energy_status = energy_status;
			if (ret > 0){
				pp0_joules = energy_status;
				pp0_watts = (energy_status-last_pp0_energy_status)/measurment_interval;
			}
			last_pp0_energy_status = energy_status;
		}
		if (pp1_domain_present()) {
			ret = get_pp1_energy_status(&energy_status);
			if (last_pp1_energy_status == 0)
				last_pp1_energy_status = energy_status;
			if (ret > 0){
				pp1_joules = energy_status;
				pp1_watts = (energy_status-last_pp1_energy_status)/measurment_interval;
			}
			last_pp1_energy_status = energy_status;
		}
		RAPL_DBG_PRINT("%f, %f, %f, %f\n", pkg_watts, dram_watts, pp0_watts, pp1_watts);
		sleep(measurment_interval);
	}
#endif
}
