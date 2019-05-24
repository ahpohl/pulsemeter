#ifndef CONSTANTS_H
#define CONSTANTS_H

// transmission state:
enum {SENSOR_VALUE, 
      ACK,
      UNKNOWN_COMMAND, 
      CRC_CHECKSUM_MISMATCH, 
      WRONG_PACKET_SIZE};

// serial buffer
const int BUF_SIZE = 16;

// cobs encoded packet size
const int COMMAND_PACKET_SIZE = 9;
const int DATA_PACKET_SIZE = 7;

#endif // CONSTANTS_H
