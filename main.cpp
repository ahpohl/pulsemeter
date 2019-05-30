#include <iostream>
#include <string>
#include <iomanip>
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
		{ "raw", no_argument, NULL, 'a' },
        { "trigger", no_argument, NULL, 't' },
        { "low", required_argument, NULL, 'l' },
		{ "high", required_argument, NULL, 'g' },
		{ "rrd", required_argument, NULL, 'r'},
		{ "kwh", required_argument, NULL, 'k'},
        { NULL, 0, NULL, 0 }
    };

    const char * optString = "hDd:tal:g:r:k:";
    int opt = 0;
    int longIndex = 0;
	char mode = '\0'; // raw: R, trigger: T
	bool debug = false;
	bool help = false;

	// set default RRD database filename
	const char * rrd_file = "pulse.rrd";

	// set default serial device
	const char * serial_device = "/dev/ttyACM0";

	// set default trigger levels
	int trigger_level_low = 75, trigger_level_high = 90;

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

        case 'a':
            mode = 'R';
            break;

	    case 't':
            mode = 'T';
            break;

		case 'l':
			trigger_level_low = atoi(optarg);
			break;
		
		case 'g':
			trigger_level_high = atoi(optarg);
			break;

		case 'r':
			rrd_file = optarg;
			break;

		case 'k':
			rev_per_kWh = atoi(optarg);
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
  -h --help            Show help message\n\
  -D --debug           Show debug messages\n\
  -d --device [dev]    Set serial device\n\
  -r --raw             Select raw mode\n\
  -t --trigger         Select trigger mode\n\
  -l --low [int]       Set trigger level low\n\
  -g --high [int]      Set trigger level high\n\
  -r --rrd [fd]        Set round robin database\n\
  -k --kwh [int]       Set revolutions per kWh\n"
		<< endl;
		return 0;
	}

	// print message if no mode was selected
    if (mode == '\0')
    {
        cout << "Please select raw or trigger mode for normal operation." << endl;
        return 0;
    }

	// check trigger levels	
	if (trigger_level_low > trigger_level_high)
	{
		cerr << "Trigger level low larger than level high (" 
			 << trigger_level_low << " < " << trigger_level_high << ")"
			 << endl;
		return 1;
	}

    // create pulsemeter object
    Pulse meter(serial_device,
			    rrd_file,
				rev_per_kWh);

	// set debug flag
    if (debug)
    {
        meter.SetDebug();
    }

	// create RRD file if not exist
	meter.RRDCreate();

	// sync communication with sensor
	meter.SyncSerial();	

	// read raw sensor data
	if (mode == 'R')
	{
		meter.SetRawMode();
		while (1)
		{
			meter.ReadSensorValue();
		}
	}

	// read trigger data
	else if (mode == 'T')
	{
		meter.SetTriggerMode(trigger_level_low, trigger_level_high);
		while (1)
		{
			meter.ReadSensorValue();
		}
	}

	return 0;
}
