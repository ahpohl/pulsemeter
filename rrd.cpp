#include <iostream>
#include <sstream>
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

// update meter reading in the rrd file (in kWh)
void Pulse::RRDUpdateCounter(int trigger_state)
{
	int ret = 0;;

	// counter_step_size in kWh
	double counter_step_size = 1.0 / RevPerKiloWattHour;

	// total meter reading in kWh
	static double meter_reading = 0;

	// energy per revolution in Ws
	int energy_per_rev = static_cast<int>(counter_step_size * 3600 * 1000);
		
	// rrd update string to write into rrd file
	ostringstream rrd_update;

    // output for rrd update
    if (trigger_state == 1)
    {
        meter_reading += counter_step_size;
        rrd_update << "N:" << scientific << meter_reading << ":" 
				   << energy_per_rev << endl;

		if (Debug)
			cout << rrd_update.str();

		const char * argv[] = {rrd_update.str().c_str(), nullptr};
		ret = rrd_update_r(RRDFile, nullptr, 1, argv);

		if (ret)
    	{
        	throw runtime_error(string("Unable to update ")
 				+ rrd_get_error());
    	}
	}
}

/*
    int       rrd_update_r(
    const char *filename,
    const char *_template,
    int argc,
    const char **argv);

	int rrd_client_update(rrd_client_t *client, const char *filename, int values_num,
    	const char * const *values);
*/

// get meter reading from rrd file (in kWh)
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

// get total energy (in Wh) from rrd file in meterN format
// e.g. pulse(1234.5*Wh) 
void Pulse::RRDGetEnergyMeterN(char * energy_string, int length)
{

}

// get total power (in W) from rrd file in meterN format
// e.g. pulse(1234.5*W) 
void Pulse::RRDGetPowerMeterN(char * power_string, int length)
{

}

