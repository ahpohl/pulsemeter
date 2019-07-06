#ifndef PULSE_HPP
#define PULSE_HPP

#include <ctime>

class Pulse
{
public:
  Pulse(char const* t_file, char const* t_socket, char const* t_apikey, 
    char const* t_sysid, int const& t_rev, double const& t_meter);
  ~Pulse(void);
  void setDebug(void);
  void runTrigger(void);
  void runPVOutput(void);

  // sensor methods
  void openSerialPort(char const* t_device);
  void setRawMode(void) const;
  void setTriggerMode(short int const& t_low, short int const& t_high) const;
  int readSensorValue(void);

  // rrd methods
  void createFile(void) const;
  void updateEnergyCounter(void);
  unsigned long getLastEnergyCounter(void);
  void setTime(time_t const& t_time);
  void getEnergyAndPower(void);
  
  // PVOutput methods
  void uploadToPVOutput(void) const; 

private:
  // variables
  char const* m_file;           // filename of RRD database
  char const* m_socket;         // socket of rrdcached daemon
  char const* m_apikey;         // PVOutput api key
  char const* m_sysid;          // PVOutput system id
  int const m_rev;              // revolutions per kWh
  bool m_debug;                 // debug flag
  int m_serialport;             // serial port
  double m_energy;              // energy [Wh]
  double m_power;               // power [W]
  time_t m_time;                // timestamp of energy and power
  int m_sensor;                 // sensor value
  unsigned long m_last_energy;  // last energy counter

  // methods
  unsigned short crc16(unsigned char const* t_data, int t_length) const;
  void configureSerialPort(unsigned char const& t_vmin,
    unsigned char const& t_vtime) const;
  void sendCommand(unsigned char const* t_cmd, int const& t_length) const;
  bool syncPacket(void) const;
  void receivePacket(unsigned char * t_packet, int const& t_size) const;

  // callback function for CURL output
  static size_t curlCallback(void * t_contents, size_t const& t_size,
    size_t const& t_nmemb, void * t_user);
};

#endif // PULSE_HPP
