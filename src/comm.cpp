#include <iostream>
#include <fstream>
#include <iomanip>
#include <chrono>

#include <cstring>
#include <termios.h>    // contains POSIX terminal control definition
#include <fcntl.h>      // contains file controls like 0_RDWR
#include <unistd.h>     // write(), read(), close()
#include <errno.h>      // error integer and strerror() function
#include <sys/ioctl.h>  // contains ioctl_tty

#include "pulse.hpp"
#include "comm.hpp"
#include "cobs.hpp"

using namespace std;

//  crc16
//                                       16   12   5
//  this is the CCITT CRC 16 polynomial X  + X  + X  + 1.
//  This is 0x1021 when x is 2, but the way the algorithm works
//  we use 0x8408 (the reverse of the bit pattern).  The high
//  bit is always assumed to be set, thus we only use 16 bits to
//  represent the 17 bit value.

unsigned short Pulse::crc16(unsigned char const* t_packet, int t_length) const
{
  unsigned int crc = 0xffff;

  if (!t_length) { 
    return (~crc);
  }

  unsigned int data;
  unsigned char i;

  do {
    for (i = 0, data = (unsigned int)0xff & *t_packet++; i < 8; i++, 
      data >>= 1)
    {
      if ((crc & 0x0001) ^ (data & 0x0001)) {
        crc = (crc >> 1) ^ Con::POLY;
      } else { 
        crc >>= 1;
      }
    }
  } while (--t_length);

  crc = ~crc;

  return (crc);
}

void Pulse::openSerialPort(char const* t_device)
{
  if (!t_device) {
    throw runtime_error("Serial device argument empty");
  }
 
  m_serialport = open(t_device, O_RDWR | O_NOCTTY);
  if (m_serialport < 0) {
    throw runtime_error(string("Error opening device ") + t_device + ": "
      + strerror(errno) + " (" + to_string(errno) + ")");
  }

  int ret = ioctl(m_serialport, TIOCEXCL);
  if (ret < 0) {
    throw runtime_error(string("Error getting device lock on") 
      + t_device + ": " + strerror(errno) + " (" + to_string(errno) + ")");
  }

  if (m_debug) {
    cout << "Opened serial device " << t_device << endl;
  }

  // configure serial port for non-blocking read
  // vmin: return immediately if characters are available
  // vtime: or wait for max 0.1 sec
  configureSerialPort(0, 1);

  unsigned char sync_command[Con::COMMAND_PACKET_SIZE] = {0};
  unsigned short crc = 0xFFFF;

  sync_command[0] = 0x30; // sync mode
  crc = crc16(sync_command, 5);
  sync_command[5] = crc >> 8; // crc, high byte
  sync_command[6] = crc & 0xFF; // crc, low byte 

  int byte_received = 0;
  int count = 0;
  char header = 0xFF;

  do {
    sendCommand(sync_command, Con::COMMAND_PACKET_SIZE);
    byte_received = read(m_serialport, &header, 1);
        
    if (byte_received == -1) {
      throw runtime_error(string("Error reading serial port: ")
        + strerror(errno) + " (" + to_string(errno) + ")");
    }
    
    if (count == 100) {
      throw runtime_error("Unable to sync serial port");
    }
    ++count;
  }
  while (byte_received == 0);

  if (m_debug) {
    cout << "Sync packets sent (" << dec << count << ")" << endl;
  }

  int garbage = 0;

  do {
    byte_received = read(m_serialport, &header, 1);

    if (byte_received == -1) {
      throw runtime_error(string("Error reading serial port: ")
        + strerror(errno) + " (" + to_string(errno) + ")");
    }
    ++garbage;
  }
  while (byte_received > 0);

  if (m_debug) {
    cout << "Serial input buffer empty (" << dec << garbage << ")" << endl;
  }

  sleep(2);

  // set vmin and vtime for blocking read
  // vmin: returning when max 16 bytes are available
  // vtime: wait for up to 1 second
  configureSerialPort(Con::BUF_SIZE, 10);

  sendCommand(sync_command, Con::COMMAND_PACKET_SIZE);

  unsigned char sync_buffer[Con::BUF_SIZE];
  
  memset(sync_buffer, '\0', Con::BUF_SIZE);
  receivePacket(sync_buffer, Con::BUF_SIZE);

  if (sync_buffer[0] != Con::SYNC_OK) {
    throw runtime_error("Communication failed: packets not in sync");
  }
  
  if (m_debug) { 
    cout << "Packet sync successful" << endl;
  }
}

void Pulse::configureSerialPort(unsigned char const& t_vmin, 
  unsigned char const& t_vtime) const
{
  struct termios tty;

  memset(&tty, 0, sizeof(tty));
  int ret = tcgetattr(m_serialport, &tty);
  if (ret) {
    throw runtime_error(string("Error getting serial port attributes: ") 
      + strerror(errno) + " (" + to_string(errno) + ")");
  }

  cfmakeraw(&tty);
  tty.c_cc[VMIN] = t_vmin;
  tty.c_cc[VTIME] = t_vtime;
  cfsetispeed(&tty, B9600);
  cfsetospeed(&tty, B9600);

  ret = tcsetattr(m_serialport, TCSANOW, &tty);
  if (ret != 0) {
    throw runtime_error(string("Error setting serial port attributes: ") 
      + strerror(errno) + " (" + to_string(errno) + ")");
  }
}

bool Pulse::syncPacket(void) const
{
  unsigned char header = 0xFF;
  int count = 0;
  int byte_received = 0;

  do {
    byte_received = read(m_serialport, &header, 1);

    if (byte_received == -1) {
      throw runtime_error(string("Error reading serial port: ")
        + strerror(errno) + " (" + to_string(errno) + ")");
    }
    ++count;

    if (count == 100) {
      cerr << "Unable to sync packets" << endl;
      return false;
    }  
  } while (header != 0x00);

  if (m_debug) {
    cout << "Packet re-sync successful (" << count << ")" << endl;
  }

  return true;
}

void Pulse::sendCommand(unsigned char const* t_cmd, int const& t_length) const
{
  unsigned char cobs_command[Con::BUF_SIZE] = {0}; 
  int cobs_command_length = cobs_encode(t_cmd, t_length, cobs_command);

  if (m_debug) {
    cout << "Cmd: ";
    for (int i = 0; i < t_length; ++i) {
      cout << hex << setfill('0') << setw(2) 
        << (unsigned short) t_cmd[i] << " ";
    } 
    cout << ": cobs ";
    for (int i = 0; i < cobs_command_length; ++i) {
      cout << hex << setfill('0') << setw(2) 
        << (unsigned short) cobs_command[i] << " ";
    }
    cout << endl;
  }

  int bytes_sent = 0;

  for (int i = 0; i < 1; ++i) {
    bytes_sent = write(m_serialport, cobs_command, cobs_command_length);
  }

  if (bytes_sent == -1) {
    throw runtime_error(string("Error reading serial port: ") 
      + strerror(errno) + " (" + to_string(errno) + ")");
  } else if (bytes_sent != cobs_command_length) {
    throw runtime_error(string("Error sending encoded packet: ") 
      + to_string(bytes_sent) + " bytes transmitted ");
  }
}

void Pulse::receivePacket(unsigned char * t_packet, int const& t_size) const
{
  unsigned char cobs_packet[Con::BUF_SIZE];
  memset(cobs_packet, '\0', Con::BUF_SIZE);

  int bytes_received = read(m_serialport, cobs_packet, 
    Con::COBS_DATA_PACKET_SIZE);
  
  bool is_synced = true;

  if (bytes_received == -1) {
    throw runtime_error(string("Error reading serial port: ") 
      + strerror(errno) + " (" + to_string(errno) + ")");
  } else if (bytes_received != Con::COBS_DATA_PACKET_SIZE) {
    throw runtime_error(string("Wrong encoded packet length (") 
      + to_string(bytes_received) + ")");
  } else if (cobs_packet[bytes_received-1] != 0x00) {
    if (is_synced != syncPacket()) {
      throw runtime_error("Packet framing error: out of sync");
    }
  }

  memset(t_packet, '\0', t_size);
  int packet_length = cobs_decode(cobs_packet, bytes_received, t_packet);

  if (!packet_length) {
    throw runtime_error("Error decoding serial packet");
  } else if (packet_length != Con::DATA_PACKET_SIZE) {
    throw runtime_error(string("Wrong decoded packet length (")
      + to_string(packet_length) + ")");
  } 

  unsigned short crc_before = ((t_packet[3] & 0xFF) << 8) | (t_packet[4] & 0xFF);
  unsigned short crc_after = crc16(t_packet, 3);
    
  if (crc_after != crc_before) {
    throw runtime_error(string("CRC checksum mismatch (") 
      + to_string(crc_before) + " " + to_string(crc_after) + ")");
  }

  if (m_debug) {
    cout << "Res: ";
    for (int i = 0; i < packet_length; ++i) {
      cout << hex << setfill('0') << setw(2)
        << (unsigned short) t_packet[i] << " ";
    }
    cout << "      : cobs ";
    for (int i = 0; i < bytes_received; ++i) {
      cout << hex << setfill('0') << setw(2)
        << (unsigned short) cobs_packet[i] << " ";
    }
    cout << endl;
  }
  
  return;
}

void Pulse::setRawMode(void)
{
  m_raw = true;

  unsigned char command[Con::COMMAND_PACKET_SIZE] = {0};

  command[0] = 0x10; // raw mode
  unsigned short int crc = crc16(command, 5);
  command[5] = crc >> 8; // crc, high byte
  command[6] = crc & 0xFF; // crc, low byte

  sendCommand(command, Con::COMMAND_PACKET_SIZE);

  unsigned char packet_buffer[Con::BUF_SIZE];
  memset(packet_buffer, '\0', Con::BUF_SIZE);
  
  receivePacket(packet_buffer, Con::BUF_SIZE);

  if (packet_buffer[0] != Con::RAW_MODE) {
    throw runtime_error("Invalid packet received. Setting raw mode failed");
  }
 
  if (m_debug) { 
    cout << "Sensor raw mode" << endl;
    m_clock = std::chrono::high_resolution_clock::now();
  }
}

void Pulse::setTriggerMode(short int const& t_low, 
  short int const& t_high) const
{
  if (t_low > t_high) {
    throw runtime_error(string("Trigger level low greater than high (")
     + to_string(t_low) + " <= " + to_string(t_high) + ")");
  }

  unsigned char command[Con::COMMAND_PACKET_SIZE] = {0};
  command[0] = 0x20; // trigger mode
  command[1] = t_low >> 8;    // low trigger threshold, high byte
  command[2] = t_low & 0xFF;  // low trigger threshold, low byte
  command[3] = t_high >> 8;   // high trigger threshold, high byte
  command[4] = t_high & 0xFF; // high trigger threshold, low byte

  unsigned short int crc = crc16(command, 5);
  command[5] = crc >> 8; // crc, high byte
  command[6] = crc & 0xFF; // crc, low byte

  sendCommand(command, Con::COMMAND_PACKET_SIZE);
  
  unsigned char packet_buffer[Con::BUF_SIZE];
  memset(packet_buffer, '\0', Con::BUF_SIZE);
  
  receivePacket(packet_buffer, Con::BUF_SIZE);

  if (packet_buffer[0] != Con::TRIGGER_MODE) {
    throw runtime_error("Invalid packet received. Setting trigger mode failed");
  }

  if (m_debug) {
    cout << "Sensor trigger mode, trigger level: " 
      << dec << t_low << " " << t_high << endl;
  }
}

int Pulse::getSensorValue(void) const
{
  unsigned char packet[Con::BUF_SIZE];
  memset(packet, '\0', Con::BUF_SIZE);

  receivePacket(packet, Con::BUF_SIZE);

  if (packet[0] != Con::SENSOR_VALUE) {
    throw runtime_error("Packet not a sensor reading");
  }

  int sensor_value = (short) ((packet[1] & 0xFF) << 8) | (packet[2] & 0xFF);

  if (m_raw) {
    cout << dec << sensor_value << endl;

    if (m_debug) {
      std::chrono::high_resolution_clock::time_point elapsed = std::chrono::high_resolution_clock::now();
      long long duration = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed - m_clock).count(); 
      ofstream log;
      log.open("raw.log", ios::app);
      log << duration << "," << sensor_value << endl;
      log.close();
    }
  }

  return sensor_value;
}
