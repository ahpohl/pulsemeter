#include <iostream>
#include <fstream>
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

	time_t timestamp_start = time(nullptr) - 120;
	const int ds_count = 14;
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
    "RRA:AVERAGE:0.5:1:1500",
		"RRA:LAST:0.5:5:900",
    "RRA:AVERAGE:0.5:5:900",
    "RRA:LAST:0.5:10:450",
    "RRA:AVERAGE:0.5:10:450",
    "RRA:LAST:0.5:15:300",
    "RRA:AVERAGE:0.5:15:300",
		"RRA:LAST:0.5:60:750",
    "RRA:AVERAGE:0.5:60:750",
		"RRA:LAST:0.5:1440:375",
    "RRA:AVERAGE:0.5:1440:375",
		nullptr};

	// RRAs
	// keep 1 day in 1 min resolution
	// keep 1 month in 1 hour resolution
	// keep 1 year in 1 day resolution
	// consolidate LAST (energy) and AVERAGE (power)

  int ret = rrdc_connect(t_socket);
  if (ret) {
    throw runtime_error(rrd_get_error());
  }

	ret = rrdc_create(t_file, step_size, timestamp_start, no_overwrite, 
		ds_count, ds_schema);
	if (!ret) {
    cout << "Round Robin Database \"" << t_file << "\" created" << endl;
  }

  ret = rrdc_disconnect();
  if (ret) {
    throw runtime_error(rrd_get_error());
  }
  
  char * argv[Con::RRD_BUF_SIZE];
  time_t timestamp = time(nullptr);
  unsigned long requested_counter = lround(t_meter * t_rev);
  unsigned long counter = getEnergyCounter();

  if (counter < requested_counter)
  {
	  *argv = (char *) malloc(Con::RRD_BUF_SIZE * sizeof(char));
    memset(*argv, '\0', Con::RRD_BUF_SIZE);
    snprintf(*argv, Con::RRD_BUF_SIZE, "%ld:%ld:%ld", timestamp, 
      requested_counter, requested_counter);

    ret = rrdc_connect(t_socket);
    if (ret) {
      throw runtime_error(rrd_get_error());
    }

    ret = rrdc_update(m_file, Con::RRD_DS_LEN, (const char **) argv);
    if (ret) {
      throw runtime_error(rrd_get_error());
    }

    ret = rrdc_disconnect();
    if (ret) {
      throw runtime_error(rrd_get_error());
    }

    counter = requested_counter;
    free(*argv);
  }
  
  cout << "Meter reading: " << fixed << setprecision(1)
    << static_cast<double>(counter) / t_rev << " kWh, "
    << dec << counter << " counts"  << endl;
}

unsigned long Pulse::getEnergyCounter(void) const
{
  int ret = rrdc_connect(m_socket);
  if (ret) {
    throw runtime_error(rrd_get_error());
  }  

  ret = rrdc_flush_if_daemon(m_socket, m_file);
  if (ret) {
    throw runtime_error(rrd_get_error());
  }
  
  char **ds_names = 0;
  char **ds_data = 0;
  time_t last_update = 0;
  unsigned long ds_count = 0;

  ret = rrd_lastupdate_r(m_file, &last_update, &ds_count,
    &ds_names, &ds_data);
  if (ret) {
    throw runtime_error(rrd_get_error());
  }

  ret = rrdc_disconnect();
  if (ret) {
    throw runtime_error(rrd_get_error());
  }  
  
  // ds_data[0]: energy counts, ds_data[1]: power counts
  unsigned long counter = atol(ds_data[0]);

  rrd_freemem(ds_names);
  rrd_freemem(ds_data);

  return counter;
}

void Pulse::setEnergyCounter(void) const
{
  if (!getSensorValue())
  {
    return;
  }

  static unsigned long counter = getEnergyCounter();
  ++counter;

	time_t timestamp = time(nullptr);
  char* argv[Con::RRD_BUF_SIZE];
	*argv = (char*) malloc(Con::RRD_BUF_SIZE * sizeof(char));

  // rrd format: "timestamp : energy (Wh) : power (Ws)"
  memset(*argv, '\0', Con::RRD_BUF_SIZE);
  snprintf(*argv, Con::RRD_BUF_SIZE, "%ld:%ld:%ld", timestamp, 
    counter, counter);
	
  if (m_debug) {
	  cout << argv[0] << endl;
  }

  int ret = rrdc_connect(m_socket);
  if (ret) {
    throw runtime_error(rrd_get_error());
  }    
    
  ret = rrdc_flush_if_daemon(m_socket, m_file);
  if (ret) {
    throw runtime_error(rrd_get_error());
  } 
 
  ret = rrdc_update(m_file, Con::RRD_DS_LEN, (const char **) argv);
  if (ret) {
    throw runtime_error(rrd_get_error());
  }
  
  ret = rrdc_disconnect();
  if (ret) {
    throw runtime_error(rrd_get_error());
  }

  if (m_debug) {
    ofstream log;
    log.open("pulse.log", ios::app);
    struct tm* tm = localtime(&timestamp);
    char time_buffer[32] = {0};
    strftime(time_buffer, 31, "%F %T", tm);

    static time_t previous_time = 0;
    double energy = 1000.0 * counter / m_rev;
    double power = 3600.0 * 1000.0 / (m_rev * (timestamp - previous_time));
    previous_time = timestamp;

    // Date,Timestamp,Counter,Energy [Wh],Power [W] 
    log << time_buffer << "," << timestamp << "," << counter
      << fixed << setprecision(1) << "," << energy << "," << power << endl; 
    
    log.close();
  }

	free(*argv);
}

void Pulse::getEnergyAndPower(time_t const& t_time, int const& t_interval,
  time_t* t_endtime, double* t_energy, double* t_power) const
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
 
  *t_endtime = t_time - (Con::RRD_MIN_OFFSET * 60); 
	unsigned long step_size = t_interval * 60;
	vector<char const*> args;	

  args.push_back("xport");
	args.push_back("--daemon");
	args.push_back(m_socket); 
	args.push_back("--step");
	string step = to_string(step_size);
	args.push_back(step.c_str());
	args.push_back("--start");
	string start = to_string(*t_endtime - step_size);
	args.push_back(start.c_str());
	args.push_back("--end");
	string end = to_string(*t_endtime);
	args.push_back(end.c_str());
	string def_counts = string("DEF:counts=") + m_file + ":energy:LAST";
	args.push_back(def_counts.c_str());
	string def_value = string("DEF:value=") + m_file + ":power:AVERAGE";
  args.push_back(def_value.c_str());
	string cdef_energy_kwh = string("CDEF:energy_kwh=counts,") 
		+ to_string(m_rev) + ",/";
	args.push_back(cdef_energy_kwh.c_str());
	args.push_back("CDEF:energy=energy_kwh,1000.0,*");
	string cdef_power = string("CDEF:power=value,") 
		+ to_string(3600.0 * 1000.0 / m_rev) + ",*";
	args.push_back(cdef_power.c_str());	
	args.push_back("XPORT:energy");
	args.push_back("XPORT:power");

	time_t start_time = 0;
  int no_output = 0;
  unsigned long ds_count = 0;
  char ** ds_legend = 0;
  rrd_value_t * ds_data = 0;
  
  ret = rrd_xport(args.size(), (char**)args.data(), &no_output, &start_time,
		t_endtime, &step_size, &ds_count, &ds_legend, &ds_data);    
	if (ret) {
    throw runtime_error(rrd_get_error());
  }

  ret = rrdc_disconnect();
  if (ret) {
    throw runtime_error(rrd_get_error());
  }

	// ds_data[0]: energy in Wh, ds_data[1]: power in W
  memcpy(t_energy, ds_data, sizeof(double));
  memcpy(t_power, ++ds_data, sizeof(double));
}
