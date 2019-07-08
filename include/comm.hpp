#ifndef COMM_HPP
#define COMM_HPP

namespace Con 
{
  // crc16 polynomial, 1021H bit reversed
  const int POLY = 0x8408;

  enum TRANSMISSION_STATUS {SENSOR_VALUE, 
    SYNC_OK,
    RAW_MODE,
    TRIGGER_MODE,
    UNKNOWN_COMMAND,
    CRC_CHECKSUM_MISMATCH, 
    WRONG_PACKET_SIZE};

  const int BUF_SIZE = 16;
  const int COMMAND_PACKET_SIZE = 7;
  const int DATA_PACKET_SIZE = 5;
  const int COBS_COMMAND_PACKET_SIZE = 9;
  const int COBS_DATA_PACKET_SIZE = 7;
}

#endif // COMM_HPP
