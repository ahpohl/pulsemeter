// c++ headers
#include <iostream>
#include <string>
#include <stdexcept>
#include <typeinfo>
#include <iomanip>

// c headers
#include <cstring>
#include <termios.h> // contains POSIX terminal control definition
#include <fcntl.h> // contains file controls like 0_RDWR
#include <unistd.h> // write(), read(), close()
#include <errno.h> // error integer and strerror() function

// program headers
#include "pulse.h"
#include "cobs.h"
#include "constants.h"

using namespace std;

// destructor
Pulse::~Pulse(void)
{
	// close serial port
	if (SerialPort > 0)
    {
        close(SerialPort);
    }
}

void Pulse::SetDebug(void)
{
	this -> Debug = true;
}

/*--------------------------------------------------------------------------
    crc16
                                         16   12   5
    this is the CCITT CRC 16 polynomial X  + X  + X  + 1.
    This is 0x1021 when x is 2, but the way the algorithm works
    we use 0x8408 (the reverse of the bit pattern).  The high
    bit is always assumed to be set, thus we only use 16 bits to
    represent the 17 bit value.
----------------------------------------------------------------------------*/

#define POLY 0x8408   /* 1021H bit reversed */

unsigned short Pulse::Crc16(unsigned char * data_p, int length)
{
    unsigned char i;
    unsigned int data;
    unsigned int crc = 0xffff;

    if (length == 0)
      return (~crc);
    do
    {
        for (i=0, data=(unsigned int)0xff & *data_p++; i < 8; i++, data >>= 1)
        {
            if ((crc & 0x0001) ^ (data & 0x0001))
                crc = (crc >> 1) ^ POLY;
            else  crc >>= 1;
        }
    } while (--length);

    crc = ~crc;

    return (crc);
}

int Pulse::OpenSerialPort(const char * device)
{
	int ret = 0;
	struct termios tty;

	// open serial port	
	SerialPort = open(device, O_RDWR | O_NOCTTY);
	if (SerialPort < 0)
	{
		throw runtime_error(string("Error opening device ") + device + ": "
             + strerror(errno) + " (" + to_string(errno) + ")");
	}
	else
	{
		cout << "Opened serial device " << device << endl;
	}

	// init termios struct
    memset(&tty, 0, sizeof(tty));

    // read in existing settings
    ret = tcgetattr(SerialPort, &tty);
	if (ret != 0)
    {
        throw runtime_error(string("Error getting serial port attributes: ") 
			+ strerror(errno) + " (" + to_string(errno) + ")");
    }

	// set raw mode
    cfmakeraw(&tty);

    // set vmin and vtime
    tty.c_cc[VMIN] = 16; // returning when 16 bytes are available
    tty.c_cc[VTIME] = 10; // wait for up to 1 second

    // set in/out baud rate to be 19200 baud
    cfsetispeed(&tty, B9600);
    cfsetospeed(&tty, B9600);

    // save tty settings
    ret = tcsetattr(SerialPort, TCSANOW, &tty);
	if (ret != 0)
    {
		throw runtime_error(string("Error setting serial port attributes: ") 
			+ strerror(errno) + " (" + to_string(errno) + ")");
    }

	return 0;
}

// send command to sensor
void Pulse::SendCommand(unsigned char * command, int command_length)
{
	// command
    unsigned char cobs_command[BUF_SIZE] = {0};
	int cobs_command_length = 0;
	int bytes_sent = 0;
	
	// response
	unsigned char packet[BUF_SIZE] = {0};
	int packet_length = 0;
	int bytes_received = 0;

	// encode packet data
    cobs_command_length = cobs_encode(command, command_length, cobs_command);

	if (Debug)
	{
		cout << "Command: ";
        for (int i = 0; i < command_length; i++)
        {
            cout << hex << setfill('0') << setw(2) 
				 << (unsigned short) command[i] << " ";
        }	
		cout << ", cobs ";
		for (int i = 0; i < cobs_command_length; i++)
    	{
    		cout << hex << setfill('0') << setw(2) 
			     << (unsigned short) cobs_command[i] << " ";
    	}
    	cout << endl;
	}

    // send buffer via serial port (100x)
	for (int i = 0; i < 1; i++)
	{
    	bytes_sent = write(SerialPort, cobs_command, cobs_command_length);
	}

    // error handling
    if (bytes_sent == -1)
    {
		throw runtime_error(string("Error reading serial port: ") 
			+ strerror(errno) + " (" + to_string(errno) + ")");
	}
	else if (bytes_sent != cobs_command_length)
	{
		throw runtime_error(string("Error sending encoded packet: ") 
			+ to_string(bytes_sent) + " bytes transmitted ");
	}

	//
	// get response
	// 
	
	packet_length = ReceivePacket(packet, BUF_SIZE);
}

// receive response data packet
int Pulse::ReceivePacket(unsigned char * packet, int buffer_size)
{
	unsigned char cobs_packet[BUF_SIZE];
	int cobs_packet_length = 0;
	int bytes_received = 0;
	unsigned short crc_before = 0, crc_after = 0;
	int packet_length = 0;
	
	// reset buffer
    memset(cobs_packet, '\0', BUF_SIZE);

    // Read bytes
    bytes_received = read(SerialPort, cobs_packet, COBS_DATA_PACKET_SIZE);

    // error handling
    if (bytes_received == -1)
    {
		throw runtime_error(string("Error reading serial port: ") 
			+ strerror(errno) + " (" + to_string(errno) + ")");
    }
	else if (bytes_received != COBS_DATA_PACKET_SIZE)
	{
		throw runtime_error(string("Error: wrong encoded packet length (") 
			+ to_string(bytes_received) + ")");
	}
	else if (cobs_packet[bytes_received-1] != 0x00)
	{
		throw runtime_error("Packet framing error: out of sync");
	}

	// reset packet buffer
	memset(packet, '\0', buffer_size);

	// decode packet data
	packet_length = cobs_decode(cobs_packet, bytes_received, packet);

    if (Debug)
    {
        // decoded packet
        cout << "Response: ";
        for (int i = 0; i < packet_length; i++)
        {
            cout << hex << setfill('0') << setw(2)
                 << (unsigned short) packet[i] << " ";
        }

        // encoded packet
        cout << ", cobs ";
        for (int i = 0; i < bytes_received; i++)
        {
            cout << hex << setfill('0') << setw(2)
                 << (unsigned short) cobs_packet[i] << " ";
        }
    }

	// error handling
	if (packet_length == 0)
	{
		throw runtime_error("Error: decoding serial packet failed");
	}
    else if (packet_length != DATA_PACKET_SIZE)
    {
        throw runtime_error(string("Error: wrong decoded packet length (")
            + to_string(packet_length) + ")");
    }	

	// get crc from decoded packet
	crc_before = ((packet[3] & 0xFF) << 8) | (packet[4] & 0xFF);
		
	// recalculate crc after receiving packet
    crc_after = Crc16(packet, 3);
		
	// check crc
	if (crc_after != crc_before)
	{
		cout << "crc: 0x" << setfill('0') << setw(4) << hex << crc_before
             << " 0x" << setfill('0') << setw(4) << hex << crc_after << endl;;
		throw runtime_error("Error: CRC checksum mismatch");
	}
		
	return packet_length;
}

// set raw mode, command 0x10
void Pulse::SetRawMode()
{
    unsigned char command[COMMAND_PACKET_SIZE] = {0};
    unsigned short int crc = 0xFFFF;

    // command
    command[0] = 0x10; // raw mode

    // calculate cyclic redundancy check value
    crc = Crc16(command, 5);
    command[5] = crc >> 8; // crc, high byte
    command[6] = crc & 0xFF; // crc, low byte

    // send command to sensor
    SendCommand(command, COMMAND_PACKET_SIZE);
}

// set trigger mode, command 0x20
void Pulse::SetTriggerMode(short int trigger_level_low, short int trigger_level_high)
{
    unsigned char command[COMMAND_PACKET_SIZE] = {0};
    unsigned short int crc = 0xFFFF;

    // command
    command[0] = 0x20; // trigger mode

    // trigger threshold values
    command[1] = trigger_level_low >> 8; // low trigger threshold, high byte
    command[2] = trigger_level_low & 0xFF; // low trigger threshold, low byte
    command[3] = trigger_level_high >> 8; // high trigger threshold, high byte
    command[4] = trigger_level_high & 0xFF; // high trigger threshold, low byte

    // calculate cyclic redundancy check value
    crc = Crc16(command, 5);
    command[5] = crc >> 8; // crc, high byte
    command[6] = crc & 0xFF; // crc, low byte

    // send command to sensor
    SendCommand(command, COMMAND_PACKET_SIZE);
}

int Pulse::ReadSensorValue(void)
{
	int sensor_value = 0;
	unsigned char packet[BUF_SIZE];
	unsigned short packet_length = 0;

	// reset buffer
	memset(packet, '\0', BUF_SIZE);

	// get response
	packet_length = ReceivePacket(packet, BUF_SIZE);

	if (packet_length != DATA_PACKET_SIZE)
    {
        throw runtime_error(string("Error: wrong length of data packet (")
            + to_string(packet_length) + ")");
    }
   
	// get sensor reading from decoded packet
    sensor_value = ((packet[1] & 0xFF) << 8) | (packet[2] & 0xFF);

	return sensor_value;
}

// create rrd database
void Pulse::CreateRRDFile(const char * file)
{

}
