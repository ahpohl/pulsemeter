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

unsigned short Pulse::Crc16(unsigned char * data_p, unsigned short length)
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

// synchronise with sensor data stream
int Pulse::SerialSyncPacket(unsigned char * encoded_buffer, unsigned short encoded_length)
{
    unsigned char data_byte;
	int bytes_received = 0;

	// reset buffer
    memset(encoded_buffer, '\0', BUF_SIZE);

    do
    {
        // flush input and output buffers
        tcflush(SerialPort, TCIOFLUSH);

        // Read one byte at a time
        encoded_length = read(SerialPort, &data_byte, 1);

        // error handling
        if (encoded_length == -1)
        {
            throw runtime_error(string("Error reading serial port: ")
                + strerror(errno) + " (" + to_string(errno) + ")");
        }

		// fill buffer
		*encoded_buffer = data_byte;
		encoded_buffer++;

		// count bytes received
		bytes_received++;
    }
    while (data_byte != 0x00); // test if read byte is a zero

	return bytes_received;
}

// send command to sensor
void Pulse::SendCommand(unsigned char * decoded_buffer, unsigned short decoded_length)
{
    unsigned char encoded_buffer[BUF_SIZE] = {0};
	int encoded_length = 0;

	// encode packet data
    cobs_encode(decoded_buffer, decoded_length, encoded_buffer);

	if (Debug)
	{
		cout << "Cmd: dec ";
        for (int i = 0; i < decoded_length; i++)
        {
            cout << setfill('0') << setw(2) << hex
                 << (unsigned short) decoded_buffer[i] << " ";
        }	
		cout << ", enc ";
		for (int i = 0; i < COMMAND_PACKET_SIZE; i++)
    	{
    		cout << setfill('0') << setw(2) << hex
             	 << (unsigned short) encoded_buffer[i] << " ";
    	}
    	cout << endl;
	}

	// flush input and output buffers
    tcflush(SerialPort, TCIOFLUSH);

    // send buffer via serial port (3x)
	for (int i = 0; i < 3; i++)
	{
    	encoded_length = write(SerialPort, encoded_buffer, COMMAND_PACKET_SIZE);
	}

    // error handling
    if (encoded_length == -1)
    {
		throw runtime_error(string("Error reading serial port: ") 
			+ strerror(errno) + " (" + to_string(errno) + ")");
	}
	else if (encoded_length != COMMAND_PACKET_SIZE)
	{
		throw runtime_error(string("Error sending encoded packet: ") 
			+ to_string(encoded_length) + " bytes transmitted ");
	}

	// reset buffer
	memset(&encoded_buffer, '\0', BUF_SIZE);

	// receive acknowlegement
	SerialSyncPacket(encoded_buffer, DATA_PACKET_SIZE);
}

// set raw mode, command 0x10
void Pulse::SetRawMode()
{
	unsigned char buf[7] = {0};
	unsigned short int crc = 0xFFFF;

	// command
	buf[0] = 0x10; // raw mode

	// calculate cyclic redundancy check value
	crc = Crc16(buf, 5);
	buf[5] = crc >> 8; // crc, high byte
	buf[6] = crc & 0xFF; // crc, low byte

	// send command to sensor
	SendCommand(buf, 7);
}

// set trigger mode, command 0x20
void Pulse::SetTriggerMode(short int trigger_level_low, short int trigger_level_high)
{
    unsigned char buf[7] = {0};
    unsigned short int crc = 0xFFFF;

    // command
    buf[0] = 0x20; // trigger mode

    // trigger threshold values
    buf[1] = trigger_level_low >> 8; // low trigger threshold, high byte
    buf[2] = trigger_level_low & 0xFF; // low trigger threshold, low byte
    buf[3] = trigger_level_high >> 8; // high trigger threshold, high byte
    buf[4] = trigger_level_high & 0xFF; // high trigger threshold, low byte

    // calculate cyclic redundancy check value
    crc = Crc16(buf, 5);
    buf[5] = crc >> 8; // crc, high byte
    buf[6] = crc & 0xFF; // crc, low byte

	// send command to sensor
	SendCommand(buf, 7);
}

// read raw sensor values
int Pulse::ReadSensorValue(void)
{
	unsigned char encoded_buffer[BUF_SIZE];
	unsigned char decoded_buffer[BUF_SIZE];
	int encoded_length = 0;
	int decoded_length = 0;
	unsigned short crc_before = 0;
	unsigned short crc_after = 0;
	short sensor_value = 0;

	// reset buffer
    memset(encoded_buffer, '\0', BUF_SIZE);

    // Read bytes
    encoded_length = read(SerialPort, encoded_buffer, DATA_PACKET_SIZE);

    // error handling
    if (encoded_length == -1)
    {
		throw runtime_error(string("Error reading serial port: ") 
			+ strerror(errno) + " (" + to_string(errno) + ")");
    }
	else if (encoded_length != DATA_PACKET_SIZE)
	{
		throw runtime_error(string("Error: wrong encoded packet length ( ") 
			+ to_string(encoded_length) + ")");
	}
	else if (encoded_buffer[DATA_PACKET_SIZE-1] != 0x00)
	{
		throw runtime_error("Packet framing error: out of sync");
	}

	// decode packet data
	decoded_length = cobs_decode(encoded_buffer, encoded_length, decoded_buffer);

	// error handling
	if (decoded_length == 0)
	{
		throw runtime_error("Error: decoding serial packet failed");
	}

	// get crc from decoded packet
	crc_before = ((decoded_buffer[3] & 0xFF) << 8) | (decoded_buffer[4] & 0xFF);
		
	// recalculate crc after receiving packet
    crc_after = Crc16(decoded_buffer, 3);
		
	// check crc
	if (crc_after != crc_before)
	{
		throw runtime_error("Error: CRC checksum mismatch");
	}
		
	// get sensor reading from decoded packet
    sensor_value = ((decoded_buffer[1] & 0xFF) << 8) | (decoded_buffer[2] & 0xFF);

	//
	// console output
	//
	
	if (Debug)
	{	
		// encoded packet
        cout << "Rec: enc ";
        for (int i = 0; i < encoded_length; i++)
        {
            cout << setfill('0') << setw(2) << hex
                 << (unsigned short) encoded_buffer[i] << " ";
        }

        // decoded packet
        cout << ", dec ";
        for (int i = 0; i < decoded_length-1; i++)
        {
            cout << setfill('0') << setw(2) << hex
                 << (unsigned short) decoded_buffer[i] << " ";
        }
        
		// crc
		cout << ", crc 0x" << setfill('0') << setw(4) << hex << crc_before 
			 << " 0x" << setfill('0') << setw(4) << hex << crc_after << " : ";

	}

	// sensor value
    cout << dec << sensor_value << endl;

	return sensor_value;
}

// create rrd database
void Pulse::CreateRRDFile(const char * file)
{

}
