#include <iostream>
#include <math.h>
#include "pulse.h"

using namespace std;

// constructor
Pulse::Pulse(const char * rrd_file, const char * rrdcached_address,
	double meter_reading, int rev_per_kWh)
{
    this -> RRDFile = rrd_file;
	this -> RRDCachedAddress = rrdcached_address;
    this -> RevPerKiloWattHour = rev_per_kWh;
    this -> LastEnergyCounter = lround(meter_reading * rev_per_kWh);
    this -> Debug = false;
}

// destructor
Pulse::~Pulse(void)
{
    if (SerialPort > 0)
    {
        close(SerialPort);
        if (Debug)
            cout << "Serial port closed" << endl;
    }

    if (Debug)
        cout << "Pulse destructor method called" << endl;
}

void Pulse::SetDebug()
{
    this -> Debug = true;
}
