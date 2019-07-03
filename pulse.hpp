#ifndef PULSE_HPP
#define PULSE_HPP

/*
extern "C" {
#include <rrd.h>
#include <rrd_client.h>
}
*/

#include <ctime>

class Pulse
{
public:
  Pulse(const char * t_file, const char * t_socket, double t_meter, 
    int t_rev, const char * t_apikey, const char * t_sysid);
  ~Pulse(void);
  void setDebug(void);

  // sensor methods
  void openSyncSerialPort(const char * t_serial);
  void setRawMode(void);
  void setTriggerMode(short int t_low, short int t_high);
  int readSensorValue(void);

  // rrd methods
  void createFile(void);
  void updateEnergyCounter(void);
  unsigned long getLastEnergyCounter(void);
  void setTime(time_t t_rrdtime);
  void getEnergyAndPower(void);
  
  // PVOutput methods
  void uploadToPVOutput(void);

private:
  // variables
  bool m_debug;         // debug flag
  int m_serial;         // serial port
  int m_rev;            // revolutions per kWh
  int m_sensor;         // sensor value
  unsigned long m_last_energy;  // last energy counter
  const char * m_socket;      // socket of rrdcached daemon
  const char * m_file;      // filename of RRD database
  time_t m_rrdtime;       // timestamp of energy and power
  double m_energy;        // energy [Wh]
  double m_power;         // power [W]
  const char * m_apikey;      // PVOutput api key
  const char * m_sysid;     // PVOutput system id

  // methods
  unsigned short crc16(unsigned char * t_data, int t_length);
  void configureSerialPort(unsigned char t_vmin, unsigned char t_vtime);
  void sendCommand(unsigned char * t_cmd, int t_length);
  bool syncPacket(void);
  void receivePacket(unsigned char * t_packet, int t_size);
};

#endif // PULSE_HPP
