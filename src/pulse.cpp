#include <iostream>
#include <unistd.h>

#include "pulse.hpp"

using namespace std;

Pulse::Pulse(void)
{
  m_file = nullptr;
  m_socket = nullptr;
  m_apikey = nullptr;
  m_sysid = nullptr;
  m_url = nullptr;
  m_rev = 0;
	m_debug = false;
  m_serialport = 0;
  m_raw = false;
  m_pvoutput = false;
}

Pulse::~Pulse(void)
{
  if (m_serialport > 0) {
    close(m_serialport);
    
    if (m_debug) {
      cout << "Serial port closed" << endl;
    }
  }

  if (m_debug) {
    cout << "Pulse destructor method called" << endl;
  }
}

void Pulse::setDebug(void)
{
  m_debug = true;
}

void Pulse::runRaw(void)
{
  while (1) {
    getSensorValue();
  }
}

void Pulse::runTrigger(void)
{
  while (1) {
    setEnergyCounter();
  }
}

void Pulse::runPVOutput(void)
{
  while (1) {
    uploadToPVOutput();
    if (m_debug) {
      logXport();
    }
    sleep(1);
  }
} 
