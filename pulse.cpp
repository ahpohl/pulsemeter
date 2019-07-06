#include <iostream>
#include <math.h>     // contains lround function
#include <unistd.h>   // write(), read(), close()

#include "pulse.hpp"

using namespace std;

// constructor
Pulse::Pulse(const char * t_file, const char * t_socket, const char * t_apikey, 
  const char * t_sysid, const int &t_rev, const double &t_meter) :
  m_file(t_file), 
  m_socket(t_socket),
  m_apikey(t_apikey),
  m_sysid(t_sysid),
  m_rev(t_rev)
{
  // calculate last energy counter from meter reading
  m_last_energy = lround(t_meter * t_rev);

	// initialise private variables
	m_debug = false;
  m_raw = false;
	m_serialport = 0;
	m_energy = 0;
	m_power = 0;
	m_time = 0;
}

// destructor
Pulse::~Pulse(void)
{
  if (m_serialport > 0)
  {
    close(m_serialport);
    if (m_debug)
    {
      cout << "Serial port closed" << endl;
    }
  }

  if (m_debug)
  {
    cout << "Pulse destructor method called" << endl;
  }
}

void Pulse::setDebug(void)
{
  m_debug = true;
}

void Pulse::runRaw(void)
{
  while (1)
  {
    // read sensor values
    readSensorValue();
  }
}

void Pulse::runTrigger(void)
{
  while (1)
  {
    // read sensor value
    readSensorValue();

    // update rrd file
    updateEnergyCounter();
  }
}

void Pulse::runPVOutput(void)
{
  int const STEPS = 12; 
  int const OFFSET = 60;
  int interval[STEPS] = {0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55};
  int *p = interval;
  time_t rawtime;
  struct tm * tm;
  
  while (1)
  {
    rawtime = time(nullptr);
    tm = localtime(&rawtime);

    for (int i = 0; i < STEPS; ++i, ++p)
    {
      if ((*p == tm->tm_min) && (tm->tm_sec == 0))
      {
        // set the timestamp minus some offset
        setTime(rawtime - OFFSET);
  
        // get energy in Wh and power in W
        getEnergyAndPower();

        // upload energy and power to PVOutput.org
        uploadToPVOutput();
        
        break;
      }
    }
    
    // reset interval pointer
    p = interval;

    sleep(1);
  }
} 
