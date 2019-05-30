#include <iostream>
#include <ctime>
#include <rrd.h>
#include <rrd_client.h>

#include "pulse.h"

using namespace std;

// connect to rrd cached daemon
void Pulse::RRDConnect(const char * daemon_address)
{

}

// create rrd database
void Pulse::RRDCreate(void)
{
	int ret = 0;
	time_t timestamp_start = time(nullptr);
	const int num_ds_rrd = 8;
	const int step_size = 60;
	const char * rrd_argv[] = {
		"DS:counter:GAUGE:86400:0:1000000",
    	"DS:energy:ABSOLUTE:86400:0:1000000",
		"RRA:LAST:0.5:1:4320",
		"RRA:AVERAGE:0.5:1:4320",
		"RRA:LAST:0.5:1440:30",
		"RRA:AVERAGE:0.5:1440:30",
		"RRA:LAST:0.5:10080:520",
		"RRA:AVERAGE:0.5:10080:520"};

	ret = rrd_create_r(RRDFile, step_size, timestamp_start, 
		num_ds_rrd, rrd_argv);
	if (ret)
	{
		throw runtime_error(string("Unable to create RRD file '") 
			+ RRDFile + "': " + rrd_get_error());
	}
	
	if (Debug)
    {
        cout << "RRD file created (" << RRDFile << ")" << endl;
		cout << "Revolutions per kWh (" << RevPerKiloWattHour << ")" << endl;
    }
}

void Pulse::RRDUpdateCounter(int trigger_state)
{
	double trigger_step = 1.0 / RevPerKiloWattHour;
	double kwh_count = 0.0;
	

	if (trigger_state == 1)
	{
		
	}
}

// get total kWh counter from rrd file
double Pulse::RRDGetCounter(void)
{
	double energy_counter = 0;

	//rrd_lastupdate_r(

	return energy_counter;
}

/*
int rrd_lastupdate_r (const char *filename,
            time_t *ret_last_update,
            unsigned long *ret_ds_count,
            char ***ret_ds_names,
            char ***ret_last_ds);
    		time_t    rrd_first_r(
    		const char *filename,
    		const int rraindex);
*/

// get total energy (in W) from rrd file in meterN format
// e.g. pulse(1234.5*W) 
void Pulse::RRDGetEnergyMeterN(char * energy_string, int length)
{

}

// get total power (in Wh) from rrd file in meterN format
// e.g. pulse(1234.5*Wh) 
void Pulse::RRDGetPowerMeterN(char * power_string, int length)
{

}

