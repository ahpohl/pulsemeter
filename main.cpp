#include <iostream>
#include <string>
#include <iomanip>
#include <csignal>
#include <getopt.h> // for getopt_long
#include "pulse.h"

using namespace std;

int main(int argc, char* argv[])
{
	// parse command line
	const struct option longOpts[] = {
        { "help", no_argument, NULL, 'h' },
        { "debug", no_argument, NULL, 'D' },
		{ "device", required_argument, NULL, 'd' },
		{ "raw", no_argument, NULL, 'R' },
        { "trigger", no_argument, NULL, 'T' },
        { "low", required_argument, NULL, 'L' },
		{ "high", required_argument, NULL, 'H' },
		{ "create", no_argument, NULL, 'c' },
		{ "file", required_argument, NULL, 'f' },
		{ "address", required_argument, NULL, 'l'},
		{ "rev", required_argument, NULL, 'r'},
		{ "meter", required_argument, NULL, 'm'},
		{ "energy", no_argument, NULL, 'e' },
		{ "power", no_argument, NULL, 'p' },
		{ "cache", no_argument, NULL, 'C' },
        { NULL, 0, NULL, 0 }
    };

    const char * optString = "hDd:RTL:H:cf:a:r:m:C";
    int opt = 0;
    int longIndex = 0;
	char mode = '\0'; // raw: R, trigger: T
	bool debug = false;
	bool help = false;
	bool create_rrd_file = false;
	bool energy = false, power = false;
	bool rrdcached = false;
	double meter_reading = 0;
	

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
			energy = true;
			break;

		case 'p':
			power = true;
			break;

		case 'C':
			rrdcached = true;

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
  -e --energy           Get last energy [Wh]\n\
  -p --power            Get last power [W]\n\
  -C --cache            Use rrdcached"
		<< endl;
		return 0;
	}

    // create pulsemeter object
    Pulse meter(rrd_file, meter_reading, rev_per_kWh);

	// set debug flag
    if (debug)
    {
        meter.SetDebug();
    }

	// get energy in meterN format
	if (energy)
	{
		meter.RRDGetEnergy();
		return 0;
	}
	
	// get power in meterN format
	if (power)
	{
		meter.RRDGetPower();
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

	// open serial port
	meter.OpenSyncSerialPort(serial_device);

	// read raw sensor data
	if (mode == 'R')
	{
		// set raw mode
		meter.SetRawMode();

		// read sensor values
		while (1)
		{
			meter.ReadSensorValue();
		}
	}

	// read trigger data
	else if (mode == 'T')
	{
		// connect to rrdcached daemon
		if (rrdcached)
			meter.RRDConnect(rrdcached_address);

		// create RRD file
		if (create_rrd_file)
    		meter.RRDCreate();
		
		// get current meter reading from RRD file
        meter.RRDGetLastEnergyCounter();		

		// set trigger mode
		meter.SetTriggerMode(trigger_level_low, trigger_level_high);
	
		// read sensor values
		while (1)
		{
			// read sensor value
			meter.ReadSensorValue();
			
			// update rrd file
    		meter.RRDUpdateEnergyCounter();
		}
	}

	return 0;
}
