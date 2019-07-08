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
	m_energy = 0;
	m_power = 0;
  m_time = 0;
  m_sensor = 0;
  m_counter = 0;
  m_raw = false;
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
    readSensorValue();
  }
}

void Pulse::runTrigger(void)
{
  while (1) {
    readSensorValue();
    setEnergyCounter();
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
  
  while (1) {
    rawtime = time(nullptr);
    tm = localtime(&rawtime);

    for (int i = 0; i < STEPS; ++i, ++p) {
      if ((*p == tm->tm_min) && (tm->tm_sec == 0)) {
        getEnergyAndPower(rawtime - OFFSET);
        uploadToPVOutput();
        break;
      }
    }
    
    p = interval;
    sleep(1);
  }
} 
