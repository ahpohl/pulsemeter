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
	time_t timestamp_start = time(nullptr) - 120;
	const int ds_count = 8;
	const int step_size = 60;
	const char * ds_schema[] = {
		"DS:energy:GAUGE:3600:0:U",
    	"DS:power:COUNTER:3600:0:40000",
		"RRA:LAST:0.5:1:4320",
		"RRA:AVERAGE:0.5:1:4320",
		"RRA:LAST:0.5:1440:30",
		"RRA:AVERAGE:0.5:1440:30",
		"RRA:LAST:0.5:10080:520",
		"RRA:AVERAGE:0.5:10080:520"};

	ret = rrd_create_r(RRDFile, step_size, timestamp_start, 
		ds_count, ds_schema);
	if (ret)
	{
		throw runtime_error(rrd_get_error());
	}

	if (Debug)
		cout << "RRD: file created (" << RRDFile << ")" << endl;
	
    // set initial meter reading
    const int RRD_DS_LEN = 1;
    const int RRD_BUF_SIZE = 64;
    char * argv[RRD_BUF_SIZE];
	timestamp_start += 60;
    
	*argv = (char *) malloc(RRD_BUF_SIZE * sizeof(char));
    memset(*argv, '\0', RRD_BUF_SIZE);
    snprintf(*argv, RRD_BUF_SIZE, "%ld:%ld:%d", timestamp_start, InitialEnergyCounter, 0);

    ret = rrd_update_r(RRDFile, "energy:power", RRD_DS_LEN, (const char **) argv);
    if (ret)
    {
    	throw runtime_error(rrd_get_error());
    }

	if (Debug)
    {
		cout.precision(1);
        cout << "RRD: intial energy counter (" << fixed 
		     << InitialEnergyCounter << ")" << endl;
		cout << "RRD: revolutions per kWh (" << dec << RevPerKiloWattHour << ")" << endl;
	}

	free(*argv);
}

// update meter reading in the rrd file (in kWh)
void Pulse::RRDUpdateEnergyCounter(void)
{
	int ret = 0;
	time_t timestamp = time(nullptr);

	// counter resolution, energy per ferraris disk revolution (Wh)
	unsigned long counter_resolution = 1000 / RevPerKiloWattHour;

	// total energy counter (Wh)
	static unsigned long energy_counter = 0;

	// rrd update string to write into rrd file
	//ostringstream rrd_update;
	const int RRD_DS_LEN = 1;
    const int RRD_BUF_SIZE = 64;
    char * argv[RRD_BUF_SIZE];
	*argv = (char *) malloc(RRD_BUF_SIZE * sizeof(char));

    // output for rrd update
    if (SensorValue == 1)
    {
        energy_counter += counter_resolution;
        
		/*
		rrd_update << timestamp << ":" << scientific << meter_reading << ":" 
				   << energy_per_rev << endl;

		if (Debug)
			cout << rrd_update.str();
		*/
	
		// rrd format N : energy (Wh) : power (Ws)
		memset(*argv, '\0', RRD_BUF_SIZE);
		snprintf(*argv, RRD_BUF_SIZE, "%ld:%ld:%ld", timestamp, energy_counter, 
			counter_resolution * 3600);
	
		// output sensor value in rrd format	
		if (Debug)
			cout << argv[0] << endl;

		// update rrd file
		ret = rrd_update_r(RRDFile, "energy:power", RRD_DS_LEN, (const char **) argv);
		if (ret)
    	{
        	//throw runtime_error(string("Unable to update ")
 			//	+ rrd_get_error());
			cerr << "Unable to update " << rrd_get_error() << endl;
			rrd_clear_error();
    	}
	}

	free(*argv);
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

// get meter reading from rrd file (in Wh)
unsigned long Pulse::RRDGetLastEnergyCounter(void)
{
	int ret = 0;
    char **ds_names = 0;
    char **ds_data = 0;
    time_t last_update;
    unsigned long ds_count = 0;

	// get energy value from rrd file
	ret = rrd_lastupdate_r(RRDFile, &last_update, &ds_count, &ds_names, &ds_data);

	if (ret)
    {
    	throw runtime_error(rrd_get_error());
    }

	return LastEnergyCounter;
}

/*
int rrd_lastupdate_r (const char *filename,
            time_t *ret_last_update,
            unsigned long *ret_ds_count,
            char ***ret_ds_names,
            char ***ret_last_ds);
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

