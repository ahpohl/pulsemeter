// c++ headers
#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>

// c headers
#include <csignal>
#include <getopt.h> // for getopt_long

// program headers
#include "pulse.hpp"

using namespace std;

int main(int argc, char* argv[])
{
  // parse command line
  const struct option longOpts[] = {
    { "help", no_argument, nullptr, 'h' },
    { "debug", no_argument, nullptr, 'D' },
    { "device", required_argument, nullptr, 'd' },
    { "raw", no_argument, nullptr, 'R' },
    { "trigger", no_argument, nullptr, 'T' },
    { "low", required_argument, nullptr, 'L' },
    { "high", required_argument, nullptr, 'H' },
    { "create", no_argument, nullptr, 'c' },
    { "file", required_argument, nullptr, 'f' },
    { "address", required_argument, nullptr, 'a'},
    { "rev", required_argument, nullptr, 'r'},
    { "meter", required_argument, nullptr, 'm'},
    { "energy", no_argument, nullptr, 'e' },
    { "power", no_argument, nullptr, 'p' },
    { "pvoutput", no_argument, nullptr, 'P' },
    { "api-key", required_argument, nullptr, 'A' },
    { "sys-id", required_argument, nullptr, 's' },
    { nullptr, 0, nullptr, 0 }
  };

  const char * optString = "hDd:RTL:H:cf:a:r:m:epPA:s:";
  int opt = 0;
  int longIndex = 0;
  char mode = '\0'; // raw: R, trigger: T
  bool debug = false;
  bool help = false;
  bool create_rrd_file = false;
  bool get_energy = false, get_power = false;
  double meter_reading = 0;
  bool pvoutput = false;

  // set default path to rrd file
  const char * rrd_file = "/var/lib/rrdcached/pulse.rrd";

  // set default address of rrdcached daemon 
  const char * rrdcached_address = "unix:/run/rrdcached/rrdcached.sock";

  // set default serial device
  const char * serial_device = "/dev/ttyACM0";

  // set default trigger levels
  int trigger_level_low = 85, trigger_level_high = 100;

  // set revolutions per kWh of ferraris energy meter
  int rev_per_kWh = 75;

  // set default PVOutput.org api key
  const char * pvoutput_api_key = "212dc3361019148fdb63eb0ba53b8d2dfcc4e2ec";

  // set default PVOutput.org system id (Ilvesheim_test)
  const char * pvoutput_system_id = "67956";

  do {
    opt = getopt_long(argc, argv, optString, longOpts, &longIndex);
    switch (opt) {
    case 'h':
      help = true;
      break;

    case 'D':
      debug = true;
      break;

    case 'd':
      serial_device = optarg;
      break;

    case 'R':
      mode = 'R';
      break;

    case 'T':
      mode = 'T';
      break;

    case 'L':
      trigger_level_low = atoi(optarg);
      break;
    
    case 'H':
      trigger_level_high = atoi(optarg);
      break;

    case 'c':
      create_rrd_file = true;
      break;

    case 'f':
      rrd_file = optarg;
      break;

    case 'a':
      rrdcached_address = optarg;
      break;

    case 'r':
      rev_per_kWh = atoi(optarg);
      break;

    case 'm':
      meter_reading = atof(optarg);
      break;

    case 'e':
      get_energy = true;
      break;

    case 'p':
      get_power = true;
      break;

    case 'P':
      pvoutput = true;
      break;

    case 'A':
      pvoutput_api_key = optarg;
      break;

    case 's':
      pvoutput_system_id = optarg;
      break;

    default:
      break;
    }

  } while (opt != -1);

  // display help
  if (help)
  {
    cout << "Energy Pulsemeter Version <insert version>" << endl;
    cout << endl << "Usage: " << argv[0] << " [options]" << endl << endl;
    cout << "\
  -h --help             Show help message\n\
  -D --debug            Show debug messages\n\
  -d --device [dev]     Serial device\n\
  -R --raw              Select raw mode\n\
  -T --trigger          Select trigger mode\n\
  -L --low [int]        Set trigger level low\n\
  -H --high [int]       Set trigger level high\n\
  -c --create           Create new round robin database\n\
  -f --file [path]      Full path to rrd file\n\
  -a --address [sock]   Set address of rrdcached daemon\n\
  -r --rev [int]        Set revolutions per kWh\n\
  -m --meter [float]    Set initial meter reading [kWh]\n\
  -e --energy           Get energy [Wh]\n\
  -p --power            Get power [W]\n\
  -P --pvoutput         Upload to PVOutput.org\n\
  -A --api-key [key]    PVOutput.org api key\n\
  -s --sys-id [id]      PVOutput.org system id"
    << endl;
    return 0;
  }

  // create pulsemeter object
  Pulse meter(rrd_file, rrdcached_address, 
    meter_reading, rev_per_kWh, 
    pvoutput_api_key, pvoutput_system_id);

  // set debug flag
  if (debug)
  {
    meter.setDebug();
  }

  // get energy in Wh and power in W
  if (get_energy || get_power)
  {
    time_t current_time = time(nullptr) - 60;
    meter.setTime(current_time);
    meter.getEnergyAndPower();

    // upload energy and power to PVOutput.org
    if (pvoutput)
    {
      meter.uploadToPVOutput();
    }
    
    return 0;
  }

  // print message if no mode was selected
  if (mode == '\0')
  {
    cerr << "Please select raw or trigger mode for sensor operation." << endl;
    return 1;
  }

  // check trigger levels 
  if (trigger_level_low > trigger_level_high)
  {
    cerr << "Trigger level low larger than level high (" 
      << trigger_level_low << " < " << trigger_level_high << ")"
      << endl;
    return 1;
  }

  // read raw sensor data
  if (mode == 'R')
  {
    // open serial port
    meter.openSerialPort(serial_device);

    // set raw mode
    meter.setRawMode();

    // read sensor values
    while (1)
    {
      meter.readSensorValue();
    }
  }

  // read trigger data
  else if (mode == 'T')
  {
    // open serial port
    meter.openSerialPort(serial_device);

    // create RRD file
    if (create_rrd_file)
    {
      meter.createFile();
    }
    
    // get current meter reading from RRD file
    meter.getLastEnergyCounter();  

    // set trigger mode
    meter.setTriggerMode(trigger_level_low, trigger_level_high);

    // read sensor values
    while (1)
    {
      // read sensor value
      meter.readSensorValue();

      // update rrd file
      meter.updateEnergyCounter();
    }
  }

  return 0;
}
