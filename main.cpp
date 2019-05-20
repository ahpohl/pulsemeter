#include <iostream>
#include <string>
#include <getopt.h> // for getopt_long
#include "pulse.h"

using namespace std;

int main(int argc, char* argv[])
{
	const struct option longOpts[] = {
        { "help", no_argument, NULL, 'h' },
        { "version", no_argument, NULL, 'V' },
        { "verbose", no_argument, NULL, 'v' },
        { "trigger", no_argument, NULL, 't' },
        { "raw", no_argument, NULL, 'r' },
        { NULL, 0, NULL, 0 }
    };

    const char* optString = "hVvtr";
    int opt = 0;
    int longIndex = 0;
	char mode = '\0';

    do {
        opt = getopt_long( argc, argv, optString, longOpts, &longIndex );
        switch( opt ) {
        case 'h':
            break;

        case 'V':
            break;

        case 'v':
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

	Pulse meter;
    meter.OpenSerialPort("/dev/ttyACM0");
	meter.ReadSerialRaw();
	
	return 0;
}
