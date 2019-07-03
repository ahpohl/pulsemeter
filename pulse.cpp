#include <iostream>
#include "pulse.hpp"

using namespace std;

// constructor
Pulse::Pulse(const char * t_file, const char * t_socket, double t_meter, 
  int t_rev, const char * t_apikey, const char * t_sysid) :
  m_file(t_file), 
  m_socket(t_socket),
  m_rev(t_rev),
  m_apikey(t_apikey),
  m_sysid(t_sysid)
{
  // calculate last energy counter
  m_last_energy = t_meter * t_rev;

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
    if (m_serial > 0)
    {
        close(m_serial);
        if (m_debug)
            cout << "Serial port closed" << endl;
    }

    if (m_debug)
        cout << "Pulse destructor method called" << endl;
}

void Pulse::setDebug()
{
    m_debug = true;
}
