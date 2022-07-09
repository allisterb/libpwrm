#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <getopt.h>
#include <unistd.h>
#include <locale.h>
#include <sys/resource.h>
#include <limits.h>
#include <pthread.h>

#include "cpu/cpu.h"
#include "process/process.h"
#include "perf/perf.h"
#include "perf/perf_bundle.h"
#include "lib.h"


#include "devices/device.h"
#include "devices/devfreq.h"
#include "devices/usb.h"
#include "devices/ahci.h"
#include "measurement/measurement.h"
#include "parameters/parameters.h"

#include "tuning/tuning.h"

#include "devlist.h"

#include "spdlog/spdlog.h"

#define DEBUGFS_MAGIC          0x64626720

#define NR_OPEN_DEF 1024 * 1024

using namespace spdlog;

extern "C" {
	static volatile bool end_thread;
	void* measure_background_thread(void *arg)
	{
		int sleep_time = *((int *) arg);
		while (!end_thread) {
			sleep(sleep_time);
			global_sample_power();
		}
		return 0;
	}
}

static void checkroot() {
	int uid;
	uid = getuid();

	if (uid != 0) {
		printf("PowerTOP  must be run with root privileges.\n");
		printf("exiting...\n");
		exit(EXIT_FAILURE);
	}

}

static int get_nr_open(void) {
	int nr_open = NR_OPEN_DEF;
	ifstream file;

	file.open("/proc/sys/fs/nr_open", ios::in);
	if (file) {
		file >> nr_open;
		file.close();
	}
	return nr_open;
}

static void powertop_init(int auto_tune)
{
	static char initialized = 0;
	int ret;
	struct statfs st_fs;
	struct rlimit rlmt;

	if (initialized)
		return;

	checkroot();

	rlmt.rlim_cur = rlmt.rlim_max = get_nr_open();
	setrlimit (RLIMIT_NOFILE, &rlmt);

	if (system("/sbin/modprobe cpufreq_stats > /dev/null 2>&1"))
		fprintf(stderr, _("modprobe cpufreq_stats failed\n"));
#if defined(__i386__) || defined(__x86_64__)
	if (system("/sbin/modprobe msr > /dev/null 2>&1"))
		fprintf(stderr, _("modprobe msr failed\n"));
#endif
	statfs("/sys/kernel/debug", &st_fs);

	if (st_fs.f_type != (long) DEBUGFS_MAGIC) {
		if (access("/bin/mount", X_OK) == 0) {
			ret = system("/bin/mount -t debugfs debugfs /sys/kernel/debug > /dev/null 2>&1");
		} else {
			ret = system("mount -t debugfs debugfs /sys/kernel/debug > /dev/null 2>&1");
		}
		if (ret != 0) {
			if (!auto_tune) {
				fprintf(stderr, _("Failed to mount debugfs!\n"));
				fprintf(stderr, _("exiting...\n"));
				exit(EXIT_FAILURE);
			} else {
				fprintf(stderr, _("Failed to mount debugfs!\n"));
				fprintf(stderr, _("Should still be able to auto tune...\n"));
			}
		}
	}

	srand(time(NULL));

	if (access("/var/cache/", W_OK) == 0)
		mkdir("/var/cache/powertop", 0600);
	else
		mkdir("/data/local/powertop", 0600);

	load_results("saved_results.powertop");
	load_parameters("saved_parameters.powertop");

	enumerate_cpus();
	create_all_devices();
	create_all_devfreq_devices();
	detect_power_meters();

	register_parameter("base power", 100, 0.5);
	register_parameter("cpu-wakeups", 39.5);
	register_parameter("cpu-consumption", 1.56);
	register_parameter("gpu-operations", 0.5576);
	register_parameter("disk-operations-hard", 0.2);
	register_parameter("disk-operations", 0.0);
	//register_parameter("xwakes", 0.1);
	//load_parameters("saved_parameters.powertop");

	initialized = 1;
}

void clean_shutdown()
{
	close_results();
	clean_open_devices();
	clear_all_devices();
	clear_all_devfreq();
	clear_all_cpus();

	return;
}

void one_measurement(int seconds, int sample_interval, char *workload)
{
	create_all_usb_devices();
	start_power_measurement();
	devices_start_measurement();
	start_devfreq_measurement();
	start_process_measurement();
	start_cpu_measurement();

	if (workload && workload[0]) {
		pthread_t thread = 0UL;
		end_thread = false;
		if (pthread_create(&thread, NULL, measure_background_thread, &sample_interval))
			fprintf(stderr, "ERROR: workload measurement thread creation failed\n");

		if (system(workload))
			fprintf(stderr, _("Unknown issue running workload!\n"));

		if (thread)
		{
			end_thread = true;
			pthread_join( thread, NULL);
		}
		global_sample_power();
	} else {
		while (seconds > 0)
		{
			sleep(sample_interval > seconds ? seconds : sample_interval);
			seconds -= sample_interval;
			global_sample_power();
		}
	}
	end_cpu_measurement();
	end_process_measurement();
	collect_open_devices();
	end_devfreq_measurement();
	devices_end_measurement();
	end_power_measurement();

	process_cpu_data();
	process_process_data();

	/* output stats */
	//process_update_display();
	//report_summary();
	//w_display_cpu_cstates();
	//w_display_cpu_pstates();
	//if (reporttype != REPORT_OFF) {
	//	report_display_cpu_cstates();
	//	report_display_cpu_pstates();
	//}
	//report_process_update_display();
	//tuning_update_display();
	//wakeup_update_display();
	end_process_data();

	global_power();
	compute_bundle();

	//show_report_devices();
	//report_show_open_devices();

	//report_devices();
	//display_devfreq_devices();
	//report_devfreq_devices();
	//ahci_create_device_stats_table();
	store_results(measurement_time);
	end_cpu_data();
}

int main(int argc, char **argv)
{
	setlocale (LC_ALL, "");
	powertop_init(0);
	//initialize_devfreq();
	//initialize_tuning();
	info("Welcome to spdlog!");
	//one_measurement(10, 1, nullptr);
    return 0;
}