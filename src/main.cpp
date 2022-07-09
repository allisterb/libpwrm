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

#define DEBUGFS_MAGIC          0x64626720

#define NR_OPEN_DEF 1024 * 1024

int main(int argc, char **argv)
{
	setlocale (LC_ALL, "");
    return 0;
}


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
	process_update_display();
	report_summary();
	w_display_cpu_cstates();
	w_display_cpu_pstates();
	//if (reporttype != REPORT_OFF) {
	//	report_display_cpu_cstates();
	//	report_display_cpu_pstates();
	//}
	report_process_update_display();
	tuning_update_display();
	//wakeup_update_display();
	end_process_data();

	global_power();
	compute_bundle();

	show_report_devices();
	report_show_open_devices();

	report_devices();
	display_devfreq_devices();
	report_devfreq_devices();
	ahci_create_device_stats_table();
	store_results(measurement_time);
	end_cpu_data();
}