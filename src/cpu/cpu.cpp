/*
 * Copyright 2010, Intel Corporation
 *
 * This file is part of PowerTOP
 *
 * This program file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program in a file named COPYING; if not, write to the
 * Free Software Foundation, Inc,
 * 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 * or just google for it.
 *
 * Authors:
 *	Arjan van de Ven <arjan@linux.intel.com>
 */
#include <iostream>
#include <fstream>
#include <vector>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "cpu.h"
#include "cpudevice.h"
#include "cpu_rapl_device.h"
#include "dram_rapl_device.h"
#include "intel_cpus.h"
#include "../parameters/parameters.h"

#include "../perf/perf_bundle.h"
#include "../lib.h"


static class abstract_cpu system_level;

vector<class abstract_cpu *> all_cpus;

static	class perf_bundle * perf_events;



class perf_power_bundle: public perf_bundle
{
	virtual void handle_trace_point(void *trace, int cpu, uint64_t time);

};


static class abstract_cpu * new_package(int package, int cpu, char * vendor, int family, int model)
{
	class abstract_cpu *ret = NULL;
	class cpudevice *cpudev;
	class cpu_rapl_device *cpu_rapl_dev;
	class dram_rapl_device *dram_rapl_dev;

	char packagename[128];
	if (strcmp(vendor, "GenuineIntel") == 0)
		if (family == 6)
			if (is_supported_intel_cpu(model, cpu)) {
				ret = new class nhm_package(model);
				ret->set_intel_MSR(true);
			}

	if (!ret) {
		ret = new class cpu_package;
		ret->set_intel_MSR(false);
	}

	ret->set_number(package, cpu);
	ret->set_type("Package");
	ret->childcount = 0;

	snprintf(packagename, sizeof(packagename), _("cpu package %i"), cpu);
	cpudev = new class cpudevice(_("cpu package"), packagename, ret);
	all_devices.push_back(cpudev);

	snprintf(packagename, sizeof(packagename), _("package-%i"), cpu);
	cpu_rapl_dev = new class cpu_rapl_device(cpudev, _("cpu rapl package"), packagename, ret);
	if (cpu_rapl_dev->device_present())
		all_devices.push_back(cpu_rapl_dev);
	else
		delete cpu_rapl_dev;

	snprintf(packagename, sizeof(packagename), _("package-%i"), cpu);
	dram_rapl_dev = new class dram_rapl_device(cpudev, _("dram rapl package"), packagename, ret);
	if (dram_rapl_dev->device_present())
		all_devices.push_back(dram_rapl_dev);
	else
		delete dram_rapl_dev;

	return ret;
}

static class abstract_cpu * new_core(int core, int cpu, char * vendor, int family, int model)
{
	class abstract_cpu *ret = NULL;

	if (strcmp(vendor, "GenuineIntel") == 0)
		if (family == 6)
			if (is_supported_intel_cpu(model, cpu)) {
				ret = new class nhm_core(model);
				ret->set_intel_MSR(true);
			}

	if (!ret) {
		ret = new class cpu_core;
		ret->set_intel_MSR(false);
	}

	ret->set_number(core, cpu);
	ret->childcount = 0;
	ret->set_type("Core");

	return ret;
}

static class abstract_cpu * new_i965_gpu(void)
{
	class abstract_cpu *ret = NULL;

	ret = new class i965_core;
	ret->childcount = 0;
	ret->set_type("GPU");

	return ret;
}

static class abstract_cpu * new_cpu(int number, char * vendor, int family, int model)
{
	class abstract_cpu * ret = NULL;

	if (strcmp(vendor, "GenuineIntel") == 0)
		if (family == 6)
			if (is_supported_intel_cpu(model, number)) {
				ret = new class nhm_cpu;
				ret->set_intel_MSR(true);
			}

	if (!ret) {
		ret = new class cpu_linux;
		ret->set_intel_MSR(false);
	}
	ret->set_number(number, number);
	ret->set_type("CPU");
	ret->childcount = 0;

	return ret;
}





static void handle_one_cpu(unsigned int number, char *vendor, int family, int model)
{
	char filename[PATH_MAX];
	ifstream file;
	unsigned int package_number = 0;
	unsigned int core_number = 0;
	class abstract_cpu *package, *core, *cpu;

	snprintf(filename, sizeof(filename), "/sys/devices/system/cpu/cpu%i/topology/core_id", number);
	file.open(filename, ios::in);
	if (file) {
		file >> core_number;
		file.close();
	}

	snprintf(filename, sizeof(filename), "/sys/devices/system/cpu/cpu%i/topology/physical_package_id", number);
	file.open(filename, ios::in);
	if (file) {
		file >> package_number;
		if (package_number == (unsigned int) -1)
			package_number = 0;
		file.close();
	}


	if (system_level.children.size() <= package_number)
		system_level.children.resize(package_number + 1, NULL);

	if (!system_level.children[package_number]) {
		system_level.children[package_number] = new_package(package_number, number, vendor, family, model);
		system_level.childcount++;
	}

	package = system_level.children[package_number];
	package->parent = &system_level;

	if (package->children.size() <= core_number)
		package->children.resize(core_number + 1, NULL);

	if (!package->children[core_number]) {
		package->children[core_number] = new_core(core_number, number, vendor, family, model);
		package->childcount++;
	}

	core = package->children[core_number];
	core->parent = package;

	if (core->children.size() <= number)
		core->children.resize(number + 1, NULL);
	if (!core->children[number]) {
		core->children[number] = new_cpu(number, vendor, family, model);
		core->childcount++;
	}

	cpu = core->children[number];
	cpu->parent = core;

	if (number >= all_cpus.size())
		all_cpus.resize(number + 1, NULL);
	all_cpus[number] = cpu;
	info("Detected CPU #{}: Vendor {}, Family {}, Model {}.", number, vendor, family, model);
}

static void handle_i965_gpu(void)
{
	unsigned int core_number = 0;
	class abstract_cpu *package;


	package = system_level.children[0];

	core_number = package->children.size();

	if (package->children.size() <= core_number)
		package->children.resize(core_number + 1, NULL);

	if (!package->children[core_number]) {
		package->children[core_number] = new_i965_gpu();
		package->childcount++;
	}
}


void enumerate_cpus(void)
{
	ifstream file;
	char line[4096];

	int number = -1;
	char vendor[128];
	int family = 0;
	int model = 0;

	file.open("/proc/cpuinfo",  ios::in);

	if (!file)
		return;
	/* Not all /proc/cpuinfo include "vendor_id\t". */
	vendor[0] = '\0';

	while (file) {

		file.getline(line, sizeof(line));
		if (strncmp(line, "vendor_id\t",10) == 0) {
			char *c;
			c = strchr(line, ':');
			if (c) {
				c++;
				if (*c == ' ')
					c++;
				pt_strcpy(vendor, c);
			}
		}
		if (strncmp(line, "processor\t",10) == 0) {
			char *c;
			c = strchr(line, ':');
			if (c) {
				c++;
				number = strtoull(c, NULL, 10);
			}
		}
		if (strncmp(line, "cpu family\t",11) == 0) {
			char *c;
			c = strchr(line, ':');
			if (c) {
				c++;
				family = strtoull(c, NULL, 10);
			}
		}
		if (strncmp(line, "model\t",6) == 0) {
			char *c;
			c = strchr(line, ':');
			if (c) {
				c++;
				model = strtoull(c, NULL, 10);
			}
		}
		/* on x86 and others 'bogomips' is last
		 * on ARM it *can* be bogomips, or 'CPU revision'
		 * on POWER, it's revision
		 */
		if (strncasecmp(line, "bogomips\t", 9) == 0
		    || strncasecmp(line, "CPU revision\t", 13) == 0
		    || strncmp(line, "revision", 8) == 0) {
			if (number == -1) {
				/* Not all /proc/cpuinfo include "processor\t". */
				number = 0;
			}
			if (number >= 0) {
				handle_one_cpu(number, vendor, family, model);
				set_max_cpu(number);
				number = -2;
			}
		}
	}


	file.close();

	if (access("/sys/class/drm/card0/power/rc6_residency_ms", R_OK) == 0)
		handle_i965_gpu();

	perf_events = new perf_power_bundle();

	if (!perf_events->add_event("power:cpu_idle")){
		perf_events->add_event("power:power_start");
		perf_events->add_event("power:power_end");
	}
	if (!perf_events->add_event("power:cpu_frequency"))
		perf_events->add_event("power:power_frequency");

}

void start_cpu_measurement(void)
{
	perf_events->start();
	system_level.measurement_start();
}

void end_cpu_measurement(void)
{
	system_level.measurement_end();
	perf_events->stop();
}

static void expand_string(char *string, unsigned int newlen)
{
	while (strlen(string) < newlen)
		strcat(string, " ");
}

static int has_state_level(class abstract_cpu *acpu, int state, int line)
{
	switch (state) {
		case PSTATE:
			return acpu->has_pstate_level(line);
			break;
		case CSTATE:
			return acpu->has_cstate_level(line);
			break;
	}
	return 0;
}

static const char * fill_state_name(class abstract_cpu *acpu, int state, int line, char *buf)
{
	switch (state) {
		case PSTATE:
			return acpu->fill_pstate_name(line, buf);
			break;
		case CSTATE:
			return acpu->fill_cstate_name(line, buf);
			break;
	}
	return "-EINVAL";
}

static const char * fill_state_line(class abstract_cpu *acpu, int state, int line,
					char *buf, const char *sep = "")
{
	switch (state) {
		case PSTATE:
			return acpu->fill_pstate_line(line, buf);
			break;
		case CSTATE:
			return acpu->fill_cstate_line(line, buf, sep);
			break;
	}
	return "-EINVAL";
}

static int get_cstates_num(void)
{
	unsigned int package, core, cpu;
	class abstract_cpu *_package, * _core, * _cpu;
	unsigned int i;
	int cstates_num;

	for (package = 0, cstates_num = 0;
			package < system_level.children.size(); package++) {
		_package = system_level.children[package];
		if (_package == NULL)
			continue;

		/* walk package cstates and get largest cstates number */
		for (i = 0; i < _package->cstates.size(); i++)
			cstates_num = std::max(cstates_num,
						(_package->cstates[i])->line_level);

		/*
		 * for each core in this package, walk core cstates and get
		 * largest cstates number
		 */
		for (core = 0; core < _package->children.size(); core++) {
			_core = _package->children[core];
			if (_core == NULL)
				continue;

			for (i = 0; i <  _core->cstates.size(); i++)
				cstates_num = std::max(cstates_num,
						(_core->cstates[i])->line_level);

			/*
			 * for each core, walk the logical cpus in case
			 * there is are more linux cstates than hw cstates
			 */
			 for (cpu = 0; cpu < _core->children.size(); cpu++) {
				_cpu = _core->children[cpu];
				if (_cpu == NULL)
					continue;

				for (i = 0; i < _cpu->cstates.size(); i++)
					cstates_num = std::max(cstates_num,
						(_cpu->cstates[i])->line_level);
			}
		}
	}

	return cstates_num;
}

struct power_entry {
#ifndef __i386__
	int dummy;
#endif
	int64_t	type;
	int64_t	value;
} __attribute__((packed));


void perf_power_bundle::handle_trace_point(void *trace, int cpunr, uint64_t time)
{
	struct event_format *event;
        struct pevent_record rec; /* holder */
	class abstract_cpu *cpu;
	int type;

	rec.data = trace;

	type = pevent_data_type(perf_event::pevent, &rec);
	event = pevent_find_event(perf_event::pevent, type);

	if (!event)
		return;

	if (cpunr >= (int)all_cpus.size()) {
		cout << "INVALID cpu nr in handle_trace_point\n";
		return;
	}

	cpu = all_cpus[cpunr];

#if 0
	unsigned int i;
	printf("Time is %llu \n", time);
	for (i = 0; i < system_level.children.size(); i++)
		if (system_level.children[i])
			system_level.children[i]->validate();
#endif
	unsigned long long val;
	int ret;
	if (strcmp(event->name, "cpu_idle")==0) {

		ret = pevent_get_field_val(NULL, event, "state", &rec, &val, 0);
                if (ret < 0) {
                        fprintf(stderr, _("cpu_idle event returned no state?\n"));
                        exit(-1);
                }

		if (val == (unsigned int)-1)
			cpu->go_unidle(time);
		else
			cpu->go_idle(time);
	}

	if (strcmp(event->name, "power_frequency") == 0
	|| strcmp(event->name, "cpu_frequency") == 0){

		ret = pevent_get_field_val(NULL, event, "state", &rec, &val, 0);
		if (ret < 0) {
			fprintf(stderr, _("power or cpu_frequency event returned no state?\n"));
			exit(-1);
		}

		cpu->change_freq(time, val);
	}

	if (strcmp(event->name, "power_start")==0)
		cpu->go_idle(time);
	if (strcmp(event->name, "power_end")==0)
		cpu->go_unidle(time);

#if 0
	unsigned int i;
	for (i = 0; i < system_level.children.size(); i++)
		if (system_level.children[i])
			system_level.children[i]->validate();
#endif
}

void process_cpu_data(void)
{
	unsigned int i;
	system_level.reset_pstate_data();

	perf_events->process();

	for (i = 0; i < system_level.children.size(); i++)
		if (system_level.children[i])
			system_level.children[i]->validate();

}

void end_cpu_data(void)
{
	system_level.reset_pstate_data();

	perf_events->clear();
}

void clear_cpu_data(void)
{
	if (perf_events)
		perf_events->release();
	delete perf_events;
}


void clear_all_cpus(void)
{
	unsigned int i;
	for (i = 0; i < all_cpus.size(); i++) {
		delete all_cpus[i];
	}
	all_cpus.clear();
}

void get_rapl_info() {
	for (ulong i = 0; i < all_devices.size(); i++) {
		if (strcmp(all_devices[i]->class_name(), "cpu rapl package") == 0) {
			auto rapl_dev =  static_cast<cpu_rapl_device*>(all_devices[i]);
			if (rapl_dev->device_present())
			{
				info("Printing Intel RAPL info...");
				auto rapl = new c_rapl_interface();
				info("PKG Domain present: {}", rapl->pkg_domain_present());
				info("DRAM Domain present: {}", rapl->dram_domain_present());
				info("PP0 Domain present: {}", rapl->pp0_domain_present());
				info("PP1 Domain present: {}", rapl->pp1_domain_present());
				info("Power unit : {:01.3f}W.", rapl->get_power_unit());
				info("CPU Energy unit : {:01.8f}W.", rapl->get_energy_status_unit());
				uint64_t v = 10;
				double s = 110.0;
				double p1 = -1.0;
				double p2 = -1.0;
				double p3 = -1.0;
				double p4 = -1.0;
				rapl->get_pkg_power_info(&p1, &p2, &p3, &p4);
				info("PKG thermal spec power: {:01.3f}W.", p1);
				info("PKG maximum power: {:01.3f}W.", p2);
				info("PKG minimum power: {:01.3f}W.", p3);
				info("PKG maximum time window: {:01.3f}W.", p4);
				//info("PKG params: {} {} {} {}", p1, p2, p3, p4);
			}
			return;
		}
	}
}

bool get_rapl_device_present() {
	for (ulong i = 0; i < all_devices.size(); i++) {
		if (strcmp(all_devices[i]->class_name(), "cpu rapl package") == 0) {
			auto rapl_dev =  static_cast<cpu_rapl_device*>(all_devices[i]);
			if (rapl_dev->device_present()) {
				info("Intel RAPL interface detected.");
				return true;
			}
		}
	}
	if (!is_root) {	
		error("No Intel RAPL interface detected. Try running as root.");
	}
	else {
		error("No Intel RAPL interface detected.");
	}
	return false;
}

void start_rapl_cpu_measurement() {
		for (ulong i = 0; i < all_devices.size(); i++) {
			if (strcmp(all_devices[i]->class_name(), "cpu rapl package") == 0) {
				auto rapl_dev =  static_cast<cpu_rapl_device*>(all_devices[i]);
				if (rapl_dev->device_present())
				{
					rapl_dev->start_measurement();
				}
				return;
			}
		}
}

double end_rapl_cpu_measurement() {
		for (ulong i = 0; i < all_devices.size(); i++) {
		if (strcmp(all_devices[i]->class_name(), "cpu rapl package") == 0) {
			auto rapl_dev =  static_cast<cpu_rapl_device*>(all_devices[i]);
			if (rapl_dev->device_present())
			{
				rapl_dev->end_measurement();
				return rapl_dev->power_usage(nullptr, nullptr);
			}
		}
	}
	error("No Intel RAPL interface present.");
	exit(EXIT_FAILURE);
	return 0.0;
}