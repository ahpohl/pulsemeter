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
        { "version", no_argument, NULL, 'V' },
        { "debug", no_argument, NULL, 'd' },
        { "trigger", no_argument, NULL, 't' },
        { "raw", no_argument, NULL, 'r' },
        { NULL, 0, NULL, 0 }
    };

    const char* optString = "hVdtr";
    int opt = 0;
    int longIndex = 0;
	char mode = '\0'; // raw: R, trigger: T
	bool debug = false;

    do {
        opt = getopt_long( argc, argv, optString, longOpts, &longIndex );
        switch( opt ) {
        case 'h':
            break;

        case 'V':
            break;

        case 'd':
			debug = true;
            break;

        case 't':
            mode = 'T';
            break;

        case 'r':
            mode = 'R';
            break;

		default:
            break;
        }

	} while (opt != -1);
	
	// create object
	Pulse meter;

	// set debug flag
	if (debug)
	{
		meter.SetDebug();
	}

    // print help if no mode was selected
    if (mode == '\0')
    {
        // print help
        return 0;
    }

	// open serial port
    meter.OpenSerialPort("/dev/ttyACM0");	

	// read raw sensor data
	if (mode == 'R')
	{
		meter.ReadSerialRaw();
	}

	// read trigger data
	else if (mode == 'T')
	{
		meter.ReadSerialTrigger();
	}

	else
	{
		cerr << "Unkown mode selected" << endl;
		return 1;
	}
	
	return 0;
}
