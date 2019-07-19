#ifndef PULSE_HPP
#define PULSE_HPP

#include <ctime>

class Pulse
{
public:
  Pulse(void);
  ~Pulse(void);
  void setDebug(void);
  void runRaw(void);
  void runTrigger(void);
  void runPVOutput(void);

  void openSerialPort(char const* t_device);
  void setRawMode(void);
  void setTriggerMode(short int const& t_low, short int const& t_high) const;
  int getSensorValue(void) const;

  void createFile(char const* t_file, char const* t_socket,
    int const& t_rev, double const& t_meter);
  unsigned long getEnergyCounter(void) const;
  void setEnergyCounter(void) const;
  void getEnergyAndPower(time_t const& t_time, int const& t_step,
      time_t* t_endtime, double* t_energy, double* t_power) const;
  void logAverage(void) const; 
 
  void setPVOutput(char const* t_apikey, char const* t_sysid,
    char const* t_url, int const& t_interval);
  void uploadXport(void) const;

private:
  char const* m_file;           // filename of RRD database
  char const* m_socket;         // socket of rrdcached daemon
  char const* m_apikey;         // PVOutput api key
  char const* m_sysid;          // PVOutput system id
  char const* m_url;            // PVOutput add status url
  int m_interval;               // PVOutput interval setting
  int m_rev;                    // revolutions per kWh
  bool m_debug;                 // debug flag
  int m_serialport;             // serial port
  bool m_raw;                   // flag for raw sensor mode
  bool m_pvoutput;              // flag for pvoutput

  unsigned short crc16(unsigned char const* t_packet, int t_length) const;
  void configureSerialPort(unsigned char const& t_vmin,
    unsigned char const& t_vtime) const;
  void sendCommand(unsigned char const* t_cmd, int const& t_length) const;
  bool syncPacket(void) const;
  void receivePacket(unsigned char * t_packet, int const& t_size) const;

  static size_t curlCallback(void * t_contents, size_t t_size,
    size_t t_nmemb, void * t_user);
};

#endif // PULSE_HPP
