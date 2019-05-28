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
		{ "raw", no_argument, NULL, 'r' },
        { "trigger", no_argument, NULL, 't' },
        { "low", required_argument, NULL, 'l' },
		{ "high", required_argument, NULL, 'g' },
        { NULL, 0, NULL, 0 }
    };

    const char* optString = "hDtrl:g:";
    int opt = 0;
    int longIndex = 0;
	char mode = '\0'; // raw: R, trigger: T
	bool debug = false;
	bool help = false;

	// set default trigger levels
	int trigger_level_low = 75, trigger_level_high = 90;

    do {
        opt = getopt_long(argc, argv, optString, longOpts, &longIndex);
        switch (opt) {
        case 'h':
			help = true;
            break;

        case 'D':
			debug = true;
            break;

        case 'r':
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
  -r --raw             Select raw mode\n\
  -t --trigger         Select trigger mode\n\
  -l --low             Set trigger level low\n\
  -g --high            Set trigger level high\n"
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
    Pulse meter("/dev/ttyACM0");

	// set debug flag
    if (debug)
    {
        meter.SetDebug();
    }

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
