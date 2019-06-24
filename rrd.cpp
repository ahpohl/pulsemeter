#include <iostream>
#include <ctime>
#include <iomanip>

extern "C" {
#include <rrd.h>
#include <rrd_client.h>
}

#include "pulse.h"

using namespace std;

// create rrd database (use rrdcached)
void Pulse::RRDCreate(void)
{
	int ret = 0;
	time_t timestamp_start = time(nullptr) - 120;
	const int ds_count = 10;
	const int step_size = 60;
	const int no_overwrite = 1;

	// connect to daemon
    ret = rrdc_connect(RRDCachedAddress);
    if (ret)
    {
        throw runtime_error(rrd_get_error());
    }

	// energy is stored in counts (GAUGE)
	// energy = counts * revolutions per kWh [kWh]
	// power is stored in counts per second (COUNTER)
	// power = counts per second * 48000 [W]
	// 48000 = 1000 * 3600 / 75 revolutions per kWh
		
	const char * ds_schema[] = {
		"DS:energy:GAUGE:3600:0:U",
    	"DS:power:COUNTER:3600:0:48000",
		"RRA:LAST:0.5:1:1500",
		"RRA:LAST:0.5:5:900",
		"RRA:LAST:0.5:60:750",
		"RRA:LAST:0.5:1440:375",
		"RRA:AVERAGE:0.5:1:1500",
		"RRA:AVERAGE:0.5:5:900",
		"RRA:AVERAGE:0.5:60:750",
        "RRA:AVERAGE:0.5:1440:375",
		NULL};

	// RRAs
	// keep 1 day in 1 min resolution
	// keep 1 month in 1 hour resolution
	// keep 1 year in 1 day resolution
	// consolidate LAST (energy) and AVERAGE (power)

	ret = rrdc_create(RRDFile, step_size, timestamp_start, no_overwrite, 
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

    ret = rrdc_update(RRDFile, RRD_DS_LEN, (const char **) argv);
    if (ret)
    {
    	throw runtime_error(rrd_get_error());
    }

	if (Debug)
    {
		cout << "RRD: revolutions per kWh (" << dec << RevPerKiloWattHour << ")" << endl;
	}

	// disconnect daemon
    ret = rrdc_disconnect();
    if (ret)
    {
        throw runtime_error(rrd_get_error());
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

    // connect to daemon
    ret = rrdc_connect(RRDCachedAddress);
    if (ret)
    {
        throw runtime_error(rrd_get_error());
    }	

    // output for rrd update
    if (SensorValue == 1)
    {
        energy_counter++;
        
		// rrd format, timestamp : energy (Wh) : power (Ws)
		memset(*argv, '\0', RRD_BUF_SIZE);
		snprintf(*argv, RRD_BUF_SIZE, "%ld:%ld:%ld", timestamp, energy_counter, 
			energy_counter);
	
		// output sensor value in rrd format	
		if (Debug)
			cout << argv[0] << endl;

		// update rrd file
		ret = rrdc_update(RRDFile, RRD_DS_LEN, (const char **) argv);
		if (ret)
    	{
        	//throw runtime_error(rrd_get_error());
			cerr << rrd_get_error() << endl;
			rrd_clear_error();
    	}
	}

	// disconnect daemon
    ret = rrdc_disconnect();
    if (ret)
    {
        throw runtime_error(rrd_get_error());
    }

	free(*argv);
}

// get meter reading from rrd file (in Wh)
// for both with and without rrdcached
unsigned long Pulse::RRDGetLastEnergyCounter(void)
{
    int ret = 0;
	char **ds_names = 0;
    char **ds_data = 0;
    time_t last_update;
    unsigned long ds_count = 0;

	// flush if connected to rrdcached daemon
	ret = rrdc_flush_if_daemon(RRDCachedAddress, RRDFile);
	if (ret)
    {
        throw runtime_error(rrd_get_error());
    }	

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
             << dec << LastEnergyCounter << " counts, "
			 << fixed << setprecision(1) 
			 << static_cast<double>(LastEnergyCounter) / RevPerKiloWattHour
			 << " kWh)" << endl;
    }

	return LastEnergyCounter;
}

// get total energy (in Wh) from rrd file in meterN format
// e.g. pulse(1234.5*Wh)
// don't need rrdcached connected 
double Pulse::RRDGetEnergyMeterN(void)
{
	double energy = static_cast<double>(RRDGetLastEnergyCounter()) / 
		RevPerKiloWattHour * 1000;

	if (Debug)
		cout << "RRD: energy (" << energy << ")" << endl;	

	cout.precision(1);
	cout << "pulse(" << fixed << energy << "*Wh)" << endl;

	return energy;
}

double Pulse::RRDGetEnergy(void)
{
	double energy = 0;
	unsigned long step_size = 300;
	time_t start_time = 1561200300;
	time_t end_time = start_time + step_size - 1;
	const char cf[] = "LAST";

    int ret = 0;
    char **ds_names = 0;
    rrd_value_t * ds_data = 0;
    unsigned long ds_count = 0;

    // flush if connected to rrdcached daemon
    ret = rrdc_flush_if_daemon(RRDCachedAddress, RRDFile);
    if (ret)
    {
        throw runtime_error(rrd_get_error());
    }

    // get energy value from rrd file
    ret = rrdc_fetch(RRDFile, cf, &start_time, 
		&end_time, &step_size, &ds_count, &ds_names, &ds_data);

    if (ret)
    {
        throw runtime_error(rrd_get_error());
    }

	// ds_data[0]: energy, ds_data[1]: power
    for (unsigned long i = 0; i < ds_count; i++)
    {
        if (strcmp(ds_names[i], "energy") == 0)
        {
			// calculate energy in Wh
			energy = ds_data[i] / RevPerKiloWattHour * 1000;

			if (Debug)
				cout << fixed << setprecision(3) << energy << " Wh" << endl;
        }
        rrd_freemem(ds_names[i]);
    }
    rrd_freemem(ds_names);
	rrd_freemem(ds_data);

	return energy;
}

double Pulse::RRDGetPower(void)
{
	double power = 0;
	unsigned long step_size = 300;
	time_t start_time = 1561200300;
	time_t end_time = start_time + step_size - 1;
	const char cf[] = "AVERAGE";

    int ret = 0;
    char **ds_names = 0;
    rrd_value_t * ds_data = 0;
    unsigned long ds_count = 0;

    // flush if connected to rrdcached daemon
    ret = rrdc_flush_if_daemon(RRDCachedAddress, RRDFile);
    if (ret)
    {
        throw runtime_error(rrd_get_error());
    }

    // get energy value from rrd file
    ret = rrdc_fetch(RRDFile, cf, &start_time, 
		&end_time, &step_size, &ds_count, &ds_names, &ds_data);

    if (ret)
    {
        throw runtime_error(rrd_get_error());
    }

	// ds_data[0]: energy, ds_data[1]: power
    for (unsigned long i = 0; i < ds_count; i++)
    {
        if (strcmp(ds_names[i], "power") == 0)
        {
			// calculate energy in Wh
			power = ds_data[i] * 3600 * 1000 / RevPerKiloWattHour;

			if (Debug)
				cout << fixed << setprecision(8) << power << " W" << endl;
        }
        rrd_freemem(ds_names[i]);
    }
    rrd_freemem(ds_names);
	rrd_freemem(ds_data);

	return power;
}
