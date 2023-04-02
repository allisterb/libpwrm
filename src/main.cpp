#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <filesystem>
#include <time.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <getopt.h>
#include <unistd.h>
#include <locale.h>
#include <sys/resource.h>
#include <limits.h>
#include <pthread.h>
#include <unistd.h>
#include <termios.h>
#include <poll.h>
#include <libgen.h>
#include "date.h"

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

#ifdef CUDAToolkit_FOUND
#include "nvidia/nvml_power.h"
#endif

#include "Figlet.hh"
#include "tclap/CmdLine.h"
#include "tclap/UnlabeledValueArg.h"
#include "subprocess.hpp"
#include "json.hpp"
#include "httplib.h"

#define DEBUGFS_MAGIC          0x64626720

#define NR_OPEN_DEF 1024 * 1024

using namespace date;
using namespace std::chrono;
using namespace spdlog;
using namespace TCLAP;
namespace sp=subprocess;
using json = nlohmann::json;

bool debug_enabled = false; 
int debug_learning = 0;
bool is_root = false;
std::map<string, double> measurements;
 // scheme + host

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
		is_root = false;
		warn("Not running with root privileges...some subsystems and devices may not be accessible.\n");
	}
	else {
		is_root = true;
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

static void init(int auto_tune)
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
		info(_("modprobe cpufreq_stats failed."));
#if defined(__i386__) || defined(__x86_64__)
	if (system("/sbin/modprobe msr > /dev/null 2>&1"))
		info(_("modprobe msr failed."));
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
				error(_("Failed to mount debugfs!\n"));
				error(_("exiting...\n"));
				exit(EXIT_FAILURE);
			} else {
				error(_("Failed to mount debugfs!\n"));
				error(_("Should still be able to auto tune...\n"));
			}
		}
	}

	srand(time(NULL));

	if (access("/var/cache/", W_OK) == 0)
		mkdir("/var/cache/powertop", 0600);
	else
		mkdir("/data/local/powertop", 0600);

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
	register_parameter("xwakes", 0.1);
	
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
	end_process_data();

	global_power();
	compute_bundle();

	store_results(measurement_time);
	end_cpu_data();
}

bool iskeypressed( unsigned timeout_ms = 0 )
  {
  //if (!initialized) return false;

  struct pollfd pls[ 1 ];
  pls[ 0 ].fd     = STDIN_FILENO;
  pls[ 0 ].events = POLLIN | POLLPRI;
  return poll( pls, 1, timeout_ms ) > 0;
  }

void get_info(const string subsystem) {
	if (subsystem == "hw") {
		create_all_devices();
		create_all_usb_devices();
		info("\nPrinting hardware devices...");
		if (debug_enabled) {
			for (ulong i = 0; i < all_devices.size(); i++) {
				info("HW device class: {}. HW device name: {}. Human name: {}.", all_devices[i]->class_name(), all_devices[i]->device_name(), all_devices[i]->human_name());
			}
		}
		else {
			for (ulong i = 0; i < all_devices.size(); i++) {
				info("HW device name: {}. Human name: {}.", all_devices[i]->device_name(), all_devices[i]->human_name());
			}
		}
	}
	else if (subsystem == "rapl") {
		init(0);
		if (!get_rapl_device_present())
		{
			return;
		}
		get_rapl_info();
	}
	else if (subsystem == "meter") {
		detect_power_meters();
		print_power_meter_info();
	}
	#ifdef CUDAToolkit_FOUND
	else if (subsystem == "nv") {
		info ("Printing NVIDIA GPU devices info...");
		print_nv_devices_info();
	}
	#endif
}

void measure(const string* subsystem, const string* devid) {
	create_all_devices();
	create_all_usb_devices();
	if (*subsystem == "rapl") {
		enumerate_cpus();
		create_all_devices();
		if (!get_rapl_device_present())
		{
			return;
		}
		
		info("Measuring CPU power usage using Intel RAPL interface...");
		start_rapl_cpu_measurement();
		auto p = end_rapl_cpu_measurement();
		measurements["rapl"] = p;
		info("Power usage {:03.2f}W.", p);
	}
	else if (*subsystem == "meter")
	{
		detect_power_meters();
		start_power_measurement();
		end_power_measurement();
		auto p = global_power();
		measurements["meter"] = p;
		info("System power usage: {:03.2f}W", p);
		
	}
	#ifdef CUDAToolkit_FOUND
	else if (*subsystem == "nv") {
		info ("Printing NVIDIA GPU device #{} power usage...", *devid);
		init_nvml();
		unsigned int r = -1;
		string name = "";
		measure_nv_device_power(atoi(devid->c_str()), 0, &name, &r);
		info("GPU Device #{}: {}.", *devid, name);
		auto p = r / 1000.0;
		measurements[name] = p;
		shutdown_nvml();
		info("Power usage: {:03.2f}W.", p);
	}
	#endif
}

void daemon(const string* subsystem, const string* devid) {
	create_all_devices();
	create_all_usb_devices();
	if (*subsystem == "rapl") {
		enumerate_cpus();
		if (!get_rapl_device_present())
		{
			return;
		}
		
		info("Measuring CPU power usage using Intel RAPL interface...");
		while(true) {
			cout << "\nDaemon running...press any key to exit.\n" << flush;
			start_rapl_cpu_measurement();
			auto p = end_rapl_cpu_measurement();
			measurements["rapl"] = p;
			info("CPU Power usage {:03.2f}W.", p);
		}
	}
	else if (*subsystem == "meter")
	{
		info("Measuring system power usage...");
		detect_power_meters();
		info("Starting daemon....");
		while(true) {
			cout << "\nDaemon running...press any key to exit.\n" << flush;
			start_power_measurement();
			end_power_measurement();
			auto p = global_power();
			measurements["meter"] = p;
			info("System power usage: {:03.2f}W", p);
			if (iskeypressed( 5000 )) break;
		}
	}
	#ifdef CUDAToolkit_FOUND
	else if (*subsystem == "nv") {
		info ("Measuring NVIDIA GPU device #{} power usage...", *devid);
		
		info("Starting daemon....");
		while (true) {		
			cout << "\nDaemon running...press any key to exit.\n" << flush;
			unsigned int r = -1;
			string name = "";
			init_nvml();
			measure_nv_device_power(atoi(devid->c_str()), 0, &name, &r);
			shutdown_nvml();
			info("GPU Device #{}: {}.", *devid, name);
			auto p = r / 1000.0;
			measurements[name] = p;
			info("Power usage: {:03.2f}W.", p);
			if (iskeypressed( 5000 )) break;
		}
		
	}
	#endif
}

std::string get_exec_path() {
   char rawPathName[PATH_MAX];
   realpath("/proc/self/exe", rawPathName);
   return  std::string(rawPathName);
}

std::string get_exec_dir() {
    std::string executablePath = get_exec_path();
    char *executablePathStr = new char[PATH_MAX];
    strcpy(executablePathStr, (executablePath).c_str());
    char* executableDir = dirname(executablePathStr);
    delete [] executablePathStr;
    return std::string(executableDir);
}

string get_ISO8601_current_timestamp()
{
    auto now = chrono::system_clock::now();
    return date::format("%FT%TZ", date::floor<chrono::seconds>(now));
}

void report_co2_storage(std::string timestamp, int duration, float energy, float emissions)
{
	info("Uploading power usage and emissions data to CO2.Storage...");
	auto st= sp::Popen({"node", "co2.storage", "upload", timestamp, std::to_string(duration), std::to_string(energy), std::to_string(emissions)}, sp::cwd{(get_exec_dir() + "/../src/co2.storage").c_str()});
	st.wait();
	auto out = st.communicate();
	auto std_out = std::string(out.first.buf.data());
	auto std_err = out.second.buf.size() == 0 ? "" : std::string(out.second.buf.data());
	debug("node process output is {} {}", std_out, std_err);
	
	if (std_out.find("Asset created") != std::string::npos)
	{
		auto asset_json = std_out.substr(std_out.find("Asset created") + 14);
		info("Asset created.");
		debug("Asset response is {}.", asset_json);
	}
	else
	{
		error("Did not create asset. CO2.Storage service response: {} {}.", std_out, std_err);
	}
}

std::string get_wattime_login(const std::string username, const std::string password)
{
	httplib::Client cli("https://api2.watttime.org");
	cli.set_basic_auth(username, password);
	auto res = cli.Get("/v2/login");
	if (res->status == 200)
	{
		json data = json::parse(res->body);
		debug("WattTime API returned token: {}", data["token"].dump());
		return data["token"].get<std::string>();
	} 
	else
	{
		error("Call to refresh WattTime auth token at https://api2.watttime.org/v2/login did not succeed. HTTP status code: {}.", res->status);
		return "";
	}
}

std::string get_emissions(const std::string wt_token, const std::string wt_ba)
{
	httplib::Client cli("https://api2.watttime.org");
	debug("Token is {}", wt_token);
	cli.set_default_headers(httplib::Headers {{ "Authorization", ("Bearer " + wt_token) }, {"Accept", "*/*"}, {"User-Agent", "libpwrm"}});
	auto res = cli.Get("/v2/index?ba=" + wt_ba);
	if (res->status == 200)
	{
		debug("Call to WattTime index endpoint returned {}.", res->body);
		return res->body;
	} 
	else
	{
		error("Call to WattTime index endpoint at https://api2.watttime.org/v2/index did not succeed. HTTP status code: {}.", res->status);
		return "";
	}

}

void report(std::vector<string> devices,  std::map<string, double> measurements, int duration, const string* wt_user, const string* wt_pass, const string* wt_ba)
{
 

}

int main(int argc, char *argv[])
{
	setlocale (LC_ALL, "");
	Figlet::small.print("pwrm");
	try
	{
		CmdLine cmdline("pwrm is a program for measuring and reporting power consumption by hardware devices in real-time.", ' ', "0.1", true);
		vector<string> _cmds {"measure", "info", "daemon"};
		ValuesConstraint<string> cmds(_cmds);
		UnlabeledValueArg<string> cmd("cmd", "The command to run.    \
		\nmeasure - Measure power consumption for the particular subsystem or device.     \
		\ninfo - Print out information for the specified subsystem or device.",  true, "measure", &cmds, cmdline, false);
		#ifdef CUDAToolkit_FOUND
		vector<string> _systems {"hw", "rapl", "meter", "nv"};
		#else
		vector<string> _systems {"hw", "rapl", "meter"};
		#endif
		ValuesConstraint<string> systems(_systems);
		UnlabeledValueArg<string> subsystem("subsystem", "The subsystem or device to measure or report on.\
		\nhw - Hardware devices detected by the operating system.\
		\nrapl - Intel Running Average Power Limit (RAPL).\
		\nmeter - ACPI or other power meters." 
		#ifdef CUDAToolkit_FOUND
		+ string("\nnv - NVIDIA GPUs.")
		#endif
		,true, "hw", &systems, cmdline, false);
		ValueArg<string> devid_arg("", "devid","The device id, if any. Default is 0.",false, "0", "string", cmdline);
		SwitchArg report_arg("r", "report","Report the power measurement data using the CO2.storage service.", cmdline, false);
		ValueArg<int> report_duration_arg("", "report-duration", "The time in seconds to measure power usage and emissions.", false, 60, "int", cmdline);
		ValueArg<string> wt_user_arg("", "wt-user", "The WattTime API user.", false, "", "string", cmdline);
		ValueArg<string> wt_pass_arg("", "wt-pass", "The WattTime API password.", false, "", "string", cmdline);
		ValueArg<string> wt_ba_arg("", "wt-ba", "The WattTime API balanced authority abbreviation.", false, "", "string", cmdline);
		SwitchArg debug_arg("d","debug","Enable debug logging.", cmdline, false);
		
		cmdline.parse(argc, argv);

		debug_enabled = debug_arg.getValue();
		if (debug_enabled)
		{
			spdlog::set_level(spdlog::level::debug);
			info("Debug mode enabled.");
		}
		if (cmd.getValue() == "info") {
			report_co2_storage(get_ISO8601_current_timestamp(), 120, 0.2, 0.3);
			get_info(subsystem.getValue());
		}
		else if (cmd.getValue() == "measure") {
			measure(&subsystem.getValue(), &devid_arg.getValue());
			if (report_arg.getValue())
			{
				std::vector<string> devices;
    			for (ulong i = 0; i < all_devices.size(); i++) {
					devices.push_back(all_devices[i]->human_name());
    			}
				report(devices, measurements, 0, &wt_user_arg.getValue(), &wt_pass_arg.getValue(), &wt_ba_arg.getValue());
			}
		}

		else if (cmd.getValue() == "daemon") {
			std::vector<string> devices;
			for (ulong i = 0; i < all_devices.size(); i++) {
				devices.push_back(all_devices[i]->human_name());
			}
			if (subsystem.getValue() == "rapl") {
				enumerate_cpus();
				create_all_devices();
				if (!get_rapl_device_present())
				{
					exit(EXIT_FAILURE);
				}
				
				info("Measuring CPU power usage using Intel RAPL interface...");
				while(true) {
					info ("Daemon running...press any key to exit.");
					start_rapl_cpu_measurement();
					auto p = end_rapl_cpu_measurement();
					measurements["rapl"] = p;
					info("CPU Power usage {:03.2f}W.", p);
					if (report_arg.getValue())
					{
						report(devices, measurements, 0, &wt_user_arg.getValue(), &wt_pass_arg.getValue(), &wt_ba_arg.getValue());
					}
					if (iskeypressed(report_duration_arg.getValue() * 1000)) break;
				}
			}
			else if (subsystem.getValue() == "meter")
			{
				info("Measuring system power usage...");
				detect_power_meters();
				info("Starting daemon....");
				while(true) {
					info("Daemon running...press any key to exit.");
					start_power_measurement();
					end_power_measurement();
					auto p = global_power();
					measurements["meter"] = p;
					info("System power usage: {:03.2f}W", p);
					if (report_arg.getValue())
					{
						report(devices, measurements, report_duration_arg.getValue(), &wt_user_arg.getValue(), &wt_pass_arg.getValue(), &wt_ba_arg.getValue());
					}
					if (iskeypressed(report_duration_arg.getValue() * 1000)) break;
				}
			}
			#ifdef CUDAToolkit_FOUND
			else if (subsystem.getValue() == "nv") {
				info ("Measuring NVIDIA GPU device #{} power usage...", devid_arg.getValue());
				
				info("Starting daemon....");
				while (true) {		
					cout << "\nDaemon running...press any key to exit.\n" << flush;
					unsigned int r = -1;
					string name = "";
					init_nvml();
					measure_nv_device_power(atoi(devid_arg.getValue().c_str()), 0, &name, &r);
					shutdown_nvml();
					info("GPU Device #{}: {}.", devid_arg.getValue(), name);
					auto p = r / 1000.0;
					measurements[name] = p;
					info("Power usage: {:03.2f}W.", p);
					if (report_arg.getValue())
					{
						report(devices, measurements, report_duration_arg.getValue(), &wt_user_arg.getValue(), &wt_pass_arg.getValue(), &wt_ba_arg.getValue());
					}
					if (iskeypressed(report_duration_arg.getValue() * 1000)) break;
				}
				
			}
			#endif
		}

		clean_shutdown();

		exit(EXIT_SUCCESS);
	}
	catch (ArgException &e) 
    { 
    	error("Error parsing option {0}: {1}.", e.argId(), e.error());
    	return 1;
    }
    catch (std::exception &e) 
    { 
   		error("Runtime error parsing options: {0}", e.what());
    	return 1; 
    }
	return EXIT_SUCCESS;
}
