#ifndef PULSE_HPP
#define PULSE_HPP

#include <ctime>

class Pulse
{
public:
  Pulse(const char * t_file, const char * t_socket, const char * t_apikey, 
    const char * t_sysid, const int &t_rev, const double &t_meter);
  ~Pulse(void);
  void setDebug(void);

  // sensor methods
  void openSerialPort(const char * t_device);
  void setRawMode(void) const;
  void setTriggerMode(const short int &t_low, const short int &t_high) const;
  int readSensorValue(void);

  // rrd methods
  void createFile(void) const;
  void updateEnergyCounter(void);
  unsigned long getLastEnergyCounter(void);
  void setTime(const time_t &t_time);
  void getEnergyAndPower(void);
  
  // PVOutput methods
  void uploadToPVOutput(void) const; 

private:
  // variables
  const char * m_file;          // filename of RRD database
  const char * m_socket;        // socket of rrdcached daemon
  const char * m_apikey;        // PVOutput api key
  const char * m_sysid;         // PVOutput system id
  const int m_rev;              // revolutions per kWh
  bool m_debug;                 // debug flag
  int m_serialport;             // serial port
  double m_energy;              // energy [Wh]
  double m_power;               // power [W]
  time_t m_time;                // timestamp of energy and power
  int m_sensor;                 // sensor value
  unsigned long m_last_energy;  // last energy counter

  // methods
  unsigned short crc16(const unsigned char * t_data, int t_length) const;
  void configureSerialPort(const unsigned char &t_vmin,
    const unsigned char &t_vtime) const;
  void sendCommand(const unsigned char * t_cmd, const int &t_length) const;
  bool syncPacket(void) const;
  void receivePacket(unsigned char * t_packet, const int &t_size) const;

  // callback function for CURL output
  static size_t curlCallback(void * t_contents, const size_t &t_size,
    const size_t &t_nmemb, void * t_user);
};

#endif // PULSE_HPP
