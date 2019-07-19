#include <iostream>
#include <unistd.h>

#include "pulse.hpp"
#include "rrd.hpp"

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
  m_interval = 0;
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
  int const STEPS = (60 / m_interval);
  int upload[STEPS] = {0};
  int *p = upload;
  int step = 0;
  time_t rawtime = 0;
  struct tm* tm = nullptr;

  while (1) {
    rawtime = time(nullptr);
    tm = localtime(&rawtime);
    for (int i = 0; i < STEPS; ++i, ++p) {
      *p = step;

      // upload at interval[steps] plus the offset in minutes
      // because the rrd average consolitation won't be ready at
      // exact intervals, so upload will happen later.
      // PVOutput.org will correct the offsets and saves the 
      // data at exact intervals
      if ((*p + Con::RRD_MIN_OFFSET == tm->tm_min) && 
          (tm->tm_sec == 0)) {
        uploadXport();
        if (m_debug) {
          logAverage();
        }
        break;
      }
      step += m_interval;
    }
    step = 0;
    p = upload;
    sleep(1);
  }
}
