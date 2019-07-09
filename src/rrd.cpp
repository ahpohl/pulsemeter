#include <iostream>
#include <iomanip>
#include <vector>
#include <ctime>
#include <curl/curl.h>
#include <math.h>

extern "C" {
#include <rrd.h>
#include <rrd_client.h>
}

#include "pulse.hpp"
#include "rrd.hpp"

using namespace std;

void Pulse::createFile(char const* t_file, char const* t_socket,
  int const& t_rev, double const& t_meter)
{
  if (!t_file) {
    throw runtime_error("RRD file location not set");
  }

  if (!t_socket) {
    throw runtime_error("RRD cached socket not set");
  }

  if (t_rev == 0) {
    throw runtime_error("Revolutions per kWh not set");
  }

  m_file = t_file;
  m_socket = t_socket;
  m_rev = t_rev;

  int ret = rrdc_connect(t_socket);
  if (ret) {
    throw runtime_error(rrd_get_error());
  }

	time_t timestamp_start = time(nullptr) - 120;
	const int ds_count = 10;
	const int step_size = 60;
	const int no_overwrite = 1;

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

	ret = rrdc_create(t_file, step_size, timestamp_start, no_overwrite, 
		ds_count, ds_schema);
	if (!ret) {
    cout << "Round Robin Database \"" << t_file << "\" created" << endl;
  }
  
  char * argv[Con::RRD_BUF_SIZE];
  time_t timestamp = time(nullptr);
  unsigned long requested_counter = lround(t_meter * t_rev);
  m_counter = getEnergyCounter();

  if (m_counter < requested_counter)
  {
	  *argv = (char *) malloc(Con::RRD_BUF_SIZE * sizeof(char));
    memset(*argv, '\0', Con::RRD_BUF_SIZE);
    snprintf(*argv, Con::RRD_BUF_SIZE, "%ld:%ld:%ld", timestamp, 
      requested_counter, requested_counter);

    ret = rrdc_update(m_file, Con::RRD_DS_LEN, (const char **) argv);
    if (ret) {
      throw runtime_error(rrd_get_error());
    }
  
    m_counter = requested_counter;
    free(*argv);
  }

  ret = rrdc_disconnect();
  if (ret) {
    throw runtime_error(rrd_get_error());
  }

  cout << "Meter reading: " << fixed << setprecision(1)
    << static_cast<double>(m_counter) / t_rev << " kWh, "
    << dec << m_counter << " counts"  << endl;
}

unsigned long Pulse::getEnergyCounter(void) const
{
  int ret = rrdc_flush_if_daemon(m_socket, m_file);
  if (ret) {
    throw runtime_error(rrd_get_error());
  }
  
  char **ds_names = 0;
  char **ds_data = 0;
  time_t last_update;
  unsigned long ds_count = 0;

  ret = rrd_lastupdate_r(m_file, &last_update, &ds_count,
    &ds_names, &ds_data);
  if (ret) {
    throw runtime_error(rrd_get_error());
  }
  
  unsigned long counter = 0;

  // ds_data[0]: energy, ds_data[1]: power
  for (unsigned long i = 0; i < ds_count; i++) {
    if (strcmp(ds_names[i], "energy") == 0) {
      counter = atol(ds_data[i]);
    }
    rrd_freemem(ds_names[i]);
    rrd_freemem(ds_data[i]);
  }

  return counter;
}

void Pulse::setEnergyCounter(void)
{
  int ret = rrdc_connect(m_socket);
  if (ret) {
    throw runtime_error(rrd_get_error());
  }

	time_t timestamp = time(nullptr);
  char * argv[Con::RRD_BUF_SIZE];
	*argv = (char *) malloc(Con::RRD_BUF_SIZE * sizeof(char));

  if (m_sensor == 1)
  {
    ++m_counter;
        
    // rrd format: "timestamp : energy (Wh) : power (Ws)"
		memset(*argv, '\0', Con::RRD_BUF_SIZE);
		snprintf(*argv, Con::RRD_BUF_SIZE, "%ld:%ld:%ld", timestamp, 
      m_counter, m_counter);
	
		if (m_debug) {
			cout << argv[0] << endl;
    }

		ret = rrdc_update(m_file, Con::RRD_DS_LEN, (const char **) argv);
		if (ret) {
      throw runtime_error(rrd_get_error());
    }
	}

  ret = rrdc_disconnect();
  if (ret) {
    throw runtime_error(rrd_get_error());
  }

	free(*argv);
}

void Pulse::getEnergyAndPower(time_t const& t_time, double* t_energy, 
  double* t_power) const
{
  if (!t_time) {
    throw runtime_error("Timestamp not set");
  }

  int ret = rrdc_connect(m_socket);
  if (ret) {
    throw runtime_error(rrd_get_error());
  }

  ret = rrdc_flush_if_daemon(m_socket, m_file);
  if (ret) {
    throw runtime_error(rrd_get_error());
  }
 
  int const OFFSET = 60;
  time_t req_time = t_time - OFFSET; 
	unsigned long step_size = 300;
	vector<char const*> args;	

  args.push_back("xport");
	args.push_back("--daemon");
	args.push_back(m_socket); 
	args.push_back("--step");
	string step = to_string(step_size);
	args.push_back(step.c_str());
	args.push_back("--start");
	string start = to_string(req_time - step_size);
	args.push_back(start.c_str());
	args.push_back("--end");
	string end = to_string(req_time);
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

	time_t start_time = 0;
  time_t end_time = 0;
  int no_output = 0;
  unsigned long ds_count = 0;
  char ** ds_legend = 0;
  rrd_value_t * ds_data = 0;
  
  ret = rrd_xport(args.size(), (char**)args.data(), &no_output, &start_time,
		&end_time, &step_size, &ds_count, &ds_legend, &ds_data);    
	if (ret) {
    throw runtime_error(rrd_get_error());
  }

  ret = rrdc_disconnect();
  if (ret) {
    throw runtime_error(rrd_get_error());
  }

  if ((t_time != end_time) || 
    ((t_time - static_cast<time_t>(step_size)) != start_time))
  {
    if (m_debug) {
      cout << dec << "Start time: " << (t_time - step_size)
        << " ret " << start_time << endl
        << "End time: " << t_time << " ret " << end_time 
        << endl;
    }
    throw runtime_error("Requested time does not match returned time");
  }

	// ds_data[0]: energy in Wh, ds_data[1]: power in W
  memcpy(t_energy, ds_data, sizeof(double));
  memcpy(t_power, ++ds_data, sizeof(double));

  struct tm * tm = localtime(&t_time);
  char time_buffer[20] = {0};
  strftime(time_buffer, 19, "%F %R", tm);

  cout << "Date: " << time_buffer
    << ", energy: " << fixed << setprecision(1) << *t_energy / 1000
    << " kWh, power: " << setprecision(1) << *t_power << " W, sys id: "
    << m_sysid << endl;
}
