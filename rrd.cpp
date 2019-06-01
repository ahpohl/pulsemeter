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

	// energy is stored in counts (GAUGE)
	// energy = counts * revolutions per kWh [kWh]
	// power is stored in counts per second (COUNTER)
	// power = counts per second * 48000 [W]
	// 48000 = 1000 * 3600 / 75 revolutions per kWh
		
	const char * ds_schema[] = {
		"DS:energy:GAUGE:120:0:U",
    	"DS:power:COUNTER:120:0:48000",
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
	
    // set last meter reading
    const int RRD_DS_LEN = 1;
    const int RRD_BUF_SIZE = 64;
    char * argv[RRD_BUF_SIZE];
	timestamp_start += 60;
    
	*argv = (char *) malloc(RRD_BUF_SIZE * sizeof(char));
    memset(*argv, '\0', RRD_BUF_SIZE);
    snprintf(*argv, RRD_BUF_SIZE, "%ld:%ld:%ld", timestamp_start, LastEnergyCounter, LastEnergyCounter);

    ret = rrd_update_r(RRDFile, "energy:power", RRD_DS_LEN, (const char **) argv);
    if (ret)
    {
    	throw runtime_error(rrd_get_error());
    }

	if (Debug)
    {
		cout << "RRD: revolutions per kWh (" << dec << RevPerKiloWattHour << ")" << endl;
	}

	free(*argv);
}

// update meter reading in the rrd file (in kWh)
void Pulse::RRDUpdateEnergyCounter(void)
{
	int ret = 0;
	time_t timestamp = time(nullptr);

	// total energy counter
	static unsigned long energy_counter = LastEnergyCounter;

	// rrd update string to write into rrd file
	//ostringstream rrd_update;
	const int RRD_DS_LEN = 1;
    const int RRD_BUF_SIZE = 64;
    char * argv[RRD_BUF_SIZE];
	*argv = (char *) malloc(RRD_BUF_SIZE * sizeof(char));

    // output for rrd update
    if (SensorValue == 1)
    {
        energy_counter++;
        
		/*
		rrd_update << timestamp << ":" << scientific << meter_reading << ":" 
				   << energy_per_rev << endl;

		if (Debug)
			cout << rrd_update.str();
		*/
	
		// rrd format N : energy (Wh) : power (Ws)
		memset(*argv, '\0', RRD_BUF_SIZE);
		snprintf(*argv, RRD_BUF_SIZE, "%ld:%ld:%ld", timestamp, energy_counter, 
			energy_counter);
	
		// output sensor value in rrd format	
		if (Debug)
			cout << argv[0] << endl;

		// update rrd file
		ret = rrd_update_r(RRDFile, "energy:power", RRD_DS_LEN, (const char **) argv);
		if (ret)
    	{
        	//throw runtime_error(rrd_get_error());
			cerr << rrd_get_error() << endl;
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

	// ds_data[0]: energy, ds_data[1]: power
    for (unsigned long i = 0; i < ds_count; i++)
	{
    	if (strcmp(ds_names[i], "energy") == 0)
		{
        	LastEnergyCounter = atol(ds_data[i]);
    	}
      	rrd_freemem(ds_names[i]);
      	rrd_freemem(ds_data[i]);
    }
    rrd_freemem(ds_names);
	rrd_freemem(ds_data);
	
    if (Debug)
    {
        cout << "RRD: last energy counter ("
             << dec << LastEnergyCounter << ")" << endl;
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
double Pulse::RRDGetEnergy(void)
{
	double energy = static_cast<double>(RRDGetLastEnergyCounter()) / 
		RevPerKiloWattHour * 1000;

	if (Debug)
		cout << "RRD: energy (" << energy << ")" << endl;	

	cout.precision(1);
	cout << "pulse(" << fixed << energy << "*Wh)" << endl;

	return energy;
}

// get total power (in W) from rrd file in meterN format
// e.g. pulse(1234.5*W) 
double Pulse::RRDGetPower(void)
{
	double power = 0;

    return power;
}

