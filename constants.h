#ifndef CONSTANTS_H
#define CONSTANTS_H

// transmission state:
enum transmission_status {SENSOR_VALUE, 
      SYNC_OK,
      RAW_MODE,
      TRIGGER_MODE,
      UNKNOWN_COMMAND,
      CRC_CHECKSUM_MISMATCH, 
      WRONG_PACKET_SIZE};

// serial buffer
const int BUF_SIZE = 16;

// packet size
const int COMMAND_PACKET_SIZE = 7;
const int DATA_PACKET_SIZE = 5;

// cobs encoded packet size
const int COBS_COMMAND_PACKET_SIZE = 9;
const int COBS_DATA_PACKET_SIZE = 7;

#endif // CONSTANTS_H
