#include <iostream>
#include <math.h>     // contains lround function
#include <unistd.h>   // write(), read(), close()

#include "pulse.hpp"

using namespace std;

// constructor
Pulse::Pulse(const char * t_file, const char * t_socket, const char * t_apikey, 
  const char * t_sysid, int t_rev, double t_meter) :
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

void Pulse::setDebug()
{
  m_debug = true;
}
