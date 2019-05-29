#include <iostream>
#include <ctime>
#include <rrd.h>
#include <rrd_client.h>

#include "pulse.h"

using namespace std;

// create rrd database
void Pulse::CreateRRD(void)
{
	int ret = 0;
	time_t timestamp_start = time(nullptr);
	const int num_ds_rrd = 8;
	const int step_size = 60;
	const char * rrd_argv[] = {
		"DS:counter:GAUGE:86400:0:1000000",
    	"DS:consum:ABSOLUTE:86400:0:1000000",
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

void Pulse::UpdateRRD(int trigger_state)
{
	double trigger_step = 1.0 / RevPerKiloWattHour;
	double kwh_count = 0.0;
	
	// get total kWh from rrd file
	

	if (trigger_state == 1)
	{
		
	}
}
