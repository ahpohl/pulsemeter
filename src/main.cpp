#include <iostream>
#include <thread>
#include <getopt.h>
#include "pulse.hpp"

using namespace std;

int main(int argc, char* argv[])
{
  bool raw_mode = false;
  bool debug = false;
  bool version = false;
  bool help = false;
  double meter_reading = 0;
  bool pvoutput = false;
  char const* rrd_file = nullptr;
  char const* rrdcached_socket = nullptr;
  char const* serial_device = nullptr;
  int trigger_level_low = 0;
  int trigger_level_high = 0;
  int rev_per_kwh = 0;
  char const* pvoutput_api_key = 0;
  char const* pvoutput_system_id = 0;
  char const* pvoutput_url = nullptr;

  const struct option longOpts[] = {
    { "help", no_argument, nullptr, 'h' },
    { "version", no_argument, nullptr, 'V' },
    { "debug", no_argument, nullptr, 'D' },
    { "device", required_argument, nullptr, 'd' },
    { "raw", no_argument, nullptr, 'R' },
    { "low", required_argument, nullptr, 'L' },
    { "high", required_argument, nullptr, 'H' },
    { "file", required_argument, nullptr, 'f' },
    { "socket", required_argument, nullptr, 's'},
    { "rev", required_argument, nullptr, 'r'},
    { "meter", required_argument, nullptr, 'm'},
    { "pvoutput", no_argument, nullptr, 'P' },
    { "api-key", required_argument, nullptr, 'A' },
    { "sys-id", required_argument, nullptr, 'S' },
    { "url", required_argument, nullptr, 'u' },
    { nullptr, 0, nullptr, 0 }
  };

  const char * optString = "hVDd:RL:H:f:s:r:m:PA:S:u:";
  int opt = 0;
  int longIndex = 0;

  do {
    opt = getopt_long(argc, argv, optString, longOpts, &longIndex);
    switch (opt) {
    case 'h':
      help = true;
      break;
    case 'V':
      version = true;
      break;
    case 'D':
      debug = true;
      break;
    case 'd':
      serial_device = optarg;
      break;
    case 'R':
      raw_mode = true;
      break;
    case 'L':
      trigger_level_low = atoi(optarg);
      break;
    case 'H':
      trigger_level_high = atoi(optarg);
      break;
    case 'f':
      rrd_file = optarg;
      break;
    case 's':
      rrdcached_socket = optarg;
      break;
    case 'r':
      rev_per_kwh = atoi(optarg);
      break;
    case 'm':
      meter_reading = atof(optarg);
      break;
    case 'P':
      pvoutput = true;
      break;
    case 'A':
      pvoutput_api_key = optarg;
      break;
    case 'S':
      pvoutput_system_id = optarg;
      break;
    case 'u':
      pvoutput_url = optarg;
      break;
    default:
      break;
    }

  } while (opt != -1);

  if (help)
  {
    cout << "Energy Pulsemeter Version " << VERSION_TAG << endl;
    cout << endl << "Usage: " << argv[0] << " [options]" << endl << endl;
    cout << "\
  -h --help             Show help message\n\
  -V --version          Show build info\n\
  -D --debug            Show debug messages\n\
  -d --device [dev]     Serial device\n\
  -R --raw              Select raw mode\n\
  -L --low [int]        Set trigger level low\n\
  -H --high [int]       Set trigger level high\n\
  -c --create           Create new Round Robin Database\n\
  -f --file [path]      Full path to rrd file\n\
  -s --socket [fd]      Set socket of rrdcached daemon\n\
  -r --rev [int]        Set revolutions per kWh\n\
  -m --meter [float]    Set meter reading [kWh]\n\
  -P --pvoutput         Upload to PVOutput.org\n\
  -A --api-key [key]    PVOutput.org api key\n\
  -S --sys-id [id]      PVOutput.org system id\n\
  -u --url [url]        PVOutput.org add status url"
    << endl;
    return 0;
  }

  if (version)
  {
      cout << "Version " << VERSION_TAG 
        << " (" << VERSION_BUILD << ") built " 
        << VERSION_BUILD_DATE 
        << " by " << VERSION_BUILD_MACHINE << endl;
      return 0;
  }

  cout << "Energy Pulse Meter " << VERSION_TAG
    << " (" << VERSION_BUILD << ")" << endl;

  shared_ptr<Pulse> meter(new Pulse());

  if (debug) {
    meter->setDebug();
  }
  
  thread sensor_thread;

  if (raw_mode) {
    meter->openSerialPort(serial_device);
    meter->setRawMode();
    sensor_thread = thread(&Pulse::runRaw, meter);
  } else {
    meter->createFile(rrd_file, rrdcached_socket,
      rev_per_kwh, meter_reading);
    meter->openSerialPort(serial_device);
    meter->setTriggerMode(trigger_level_low, trigger_level_high);
    sensor_thread = thread(&Pulse::runTrigger, meter);
  }

  thread pvoutput_thread;

  if (pvoutput) {
    meter->setPVOutput(pvoutput_api_key, pvoutput_system_id, pvoutput_url);
    pvoutput_thread = thread(&Pulse::runPVOutput, meter);
  }
  
  if (sensor_thread.joinable()) { 
    sensor_thread.join();
  }  
  if (pvoutput_thread.joinable()) { 
    pvoutput_thread.join(); 
  }

  return 0;
}
