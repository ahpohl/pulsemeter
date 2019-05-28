#include <iostream>
#include <ctime>
#include <rrd.h>
#include <rrd_client.h>

#include "pulse.h"

using namespace std;

// create rrd database
void Pulse::CreateRRD(const char * filename)
{
	time_t last_up = time(nullptr);
	const char * rrd_argv[] = {
		"DS:counter:GAUGE:86400:0:1000000",
    	"DS:consum:ABSOLUTE:86400:0:1000000",
		"RRA:LAST:0.5:1:4320",
		"RRA:AVERAGE:0.5:1:4320",
		"RRA:LAST:0.5:1440:30",
		"RRA:AVERAGE:0.5:1440:30",
		"RRA:LAST:0.5:10080:520",
		"RRA:AVERAGE:0.5:10080:520"};

	//rrdc_create(filename, 60, last_up, 0, 8, rrd_argv);
}
