#include <iostream>
#include <ctime>
#include <iomanip>
#include <vector>

extern "C" {
#include <rrd.h>
#include <rrd_client.h>
#include <curl/curl.h>
}

#include "pulse.hpp"
#include "rrd.hpp"

using namespace std;

// create rrd database (use rrdcached)
void Pulse::createFile(void) const
{
	int ret = 0;
	time_t timestamp_start = time(nullptr) - 120;
	const int ds_count = 10;
	const int step_size = 60;
	const int no_overwrite = 1;

	// connect to rrdcached daemon socket
  ret = rrdc_connect(m_socket);
  if (ret)
  {
    throw runtime_error(rrd_get_error());
  }

	// energy is stored in counts (GAUGE)
	// energy = counts * revolutions per kWh [kWh]
	// power is stored in counts per second (COUNTER)
	// power = counts per second * 48000 [W]
	// 48000 = 1000 * 3600 / 75 revolutions per kWh
		
	char const* ds_schema[] = {
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
		nullptr};

	// RRAs
	// keep 1 day in 1 min resolution
	// keep 1 month in 1 hour resolution
	// keep 1 year in 1 day resolution
	// consolidate LAST (energy) and AVERAGE (power)

	ret = rrdc_create(m_file, step_size, timestamp_start, no_overwrite, 
		ds_count, ds_schema);
	if (ret)
	{
		throw runtime_error(rrd_get_error());
	}

	if (m_debug)
  {
		cout << "RRD: file created (" << m_file << ")" << endl;
	}

  // set last meter reading
  char * argv[Con::RRD_BUF_SIZE];
	timestamp_start += 60;
    
	*argv = (char *) malloc(Con::RRD_BUF_SIZE * sizeof(char));
  memset(*argv, '\0', Con::RRD_BUF_SIZE);
  snprintf(*argv, Con::RRD_BUF_SIZE, "%ld:%ld:%ld", timestamp_start, 
    m_last_energy, m_last_energy);

  ret = rrdc_update(m_file, Con::RRD_DS_LEN, (const char **) argv);
  if (ret)
  {
    throw runtime_error(rrd_get_error());
  }

	if (m_debug)
  {
		cout << "RRD: revolutions per kWh (" << dec << m_rev << ")" << endl;
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
void Pulse::updateEnergyCounter(void)
{
	int ret = 0;
	time_t timestamp = time(nullptr);

	// get total energy counter
	static unsigned long energy_counter = m_last_energy;

	// rrd update string to write into rrd file
  char * argv[Con::RRD_BUF_SIZE];
	*argv = (char *) malloc(Con::RRD_BUF_SIZE * sizeof(char));

  // connect to daemon
  ret = rrdc_connect(m_socket);
  if (ret)
  {
    throw runtime_error(rrd_get_error());
  }	

  // update energy counter if sensor is triggered
  if (m_sensor == 1)
  {
    ++energy_counter;
        
    // rrd format, timestamp : energy (Wh) : power (Ws)
		memset(*argv, '\0', Con::RRD_BUF_SIZE);
		snprintf(*argv, Con::RRD_BUF_SIZE, "%ld:%ld:%ld", timestamp, energy_counter, 
			energy_counter);
	
		// output sensor value in rrd format	
		if (m_debug)
    {
			cout << argv[0] << endl;
    }

		// update rrd file
		ret = rrdc_update(m_file, Con::RRD_DS_LEN, (const char **) argv);
		if (ret)
    {
      throw runtime_error(rrd_get_error());
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
// works both with and without rrdcached
unsigned long Pulse::getLastEnergyCounter(void)
{
  int ret = 0;
	char **ds_names = 0;
  char **ds_data = 0;
  time_t last_update;
  unsigned long ds_count = 0;

	// flush if connected to rrdcached daemon
	ret = rrdc_flush_if_daemon(m_socket, m_file);
	if (ret)
  {
    throw runtime_error(rrd_get_error());
  }	

	// get energy value from rrd file
	ret = rrd_lastupdate_r(m_file, &last_update, &ds_count, 
    &ds_names, &ds_data);

	if (ret)
  {
    throw runtime_error(rrd_get_error());
  }

	// ds_data[0]: energy, ds_data[1]: power
  for (unsigned long i = 0; i < ds_count; i++)
	{
    if (strcmp(ds_names[i], "energy") == 0)
    {
      m_last_energy = atol(ds_data[i]);
    }
    rrd_freemem(ds_names[i]);
    rrd_freemem(ds_data[i]);
  }
  rrd_freemem(ds_names);
	rrd_freemem(ds_data);
	
  if (m_debug)
  {
    cout << "RRD: last energy counter ("
      << dec << m_last_energy << " counts, "
			<< fixed << setprecision(1) 
			<< static_cast<double>(m_last_energy) / m_rev
			<< " kWh)" << endl;
  }

	return m_last_energy;
}

// get energy and power from rrd file
void Pulse::getEnergyAndPower(void)
{
	int ret = 0;
	int no_output = 0;
	unsigned long step_size = 300;
	time_t start_time = 0;
  unsigned long ds_count = 0;
	char ** ds_legend = 0;
	rrd_value_t * ds_data = 0;

  // check if time has been set
  if (m_time == 0)
  {
    throw runtime_error("setTime(): timestamp not set");
  }

	// construct vector of <const char *> for rrd_xport
	vector<char const*> args;
	args.push_back("xport");
	
	args.push_back("--daemon");
	args.push_back(m_socket); 
	
	args.push_back("--step");
	string step = to_string(step_size);
	args.push_back(step.c_str());

	args.push_back("--start");
	string start = to_string(m_time - step_size);
	args.push_back(start.c_str());

	args.push_back("--end");
	string end = to_string(m_time);
	args.push_back(end.c_str());

	string def_counts = string("DEF:counts=") + m_file + ":energy:LAST";
	args.push_back(def_counts.c_str());

	string def_value = string("DEF:value=") + m_file + ":power:AVERAGE";
  args.push_back(def_value.c_str());

	string cdef_energy_kwh = string("CDEF:energy_kwh=counts,") 
		+ to_string(m_rev) + ",/";
	args.push_back(cdef_energy_kwh.c_str());

	args.push_back("CDEF:energy=energy_kwh,1000,*");

	string cdef_power = string("CDEF:power=value,") 
		+ to_string(3600 * 1000 / m_rev) + ",*";
	args.push_back(cdef_power.c_str());	

	args.push_back("XPORT:energy");
	args.push_back("XPORT:power");

  // flush if connected to rrdcached daemon
  ret = rrdc_flush_if_daemon(m_socket, m_file);
  if (ret)
  {
    throw runtime_error(rrd_get_error());
  }

	// export power from rrd file
	ret = rrd_xport(args.size(), (char**)args.data(), &no_output, &start_time,
		&m_time, &step_size, &ds_count, &ds_legend, &ds_data);
    
	if (ret)
  {
    throw runtime_error(rrd_get_error());
  }

	// ds_data[0]: energy in Wh, ds_data[1]: power in W
  memcpy(&m_energy, ds_data++, sizeof(double));
  memcpy(&m_power, ds_data, sizeof(double));

  // format end_time
  struct tm * tm = localtime(&m_time);
  char time_buffer[20] = {0};
  strftime(time_buffer, 19, "%F %R", tm);

  // output time, energy and power values, system id
  cout << "Date: " << time_buffer
    << ", energy: " << fixed << setprecision(3) << m_energy
    << " Wh, power: " << setprecision(8) << m_power << " W, sys id: "
    << m_sysid << endl;
}

// set the time when energy and power are fetched from rrd
void Pulse::setTime(time_t const& t_time)
{
  m_time = t_time;
}
