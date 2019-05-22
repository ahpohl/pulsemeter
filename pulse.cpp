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
		cerr << "Error opening device " << device << ": "
             << strerror(errno) << " (" << errno << ")" << endl;
		return 1;
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
        cerr << "Error getting serial port attributes: " << strerror(errno) 
             << " (" << errno << ")" << endl;
        return 1;
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
		cerr << "Error setting serial port attributes: " << strerror(errno)
             << " (" << errno << ")" << endl;
		return 1;
    }

	return 0;
}

// read raw sensor values
void Pulse::ReadSerialRaw(void)
{
	const int BUF_SIZE = 16;
	const int ENCODED_PACKET_SIZE = 7; 
	unsigned char encoded_buffer[BUF_SIZE];
	unsigned char decoded_buffer[BUF_SIZE];
	int encoded_length = 0;
	int decoded_length = 0;
	unsigned char header;
	unsigned short crc_before = 0;
	unsigned short crc_after = 0;
	short sensor_value = 0;

    // synchronise with sensor data stream
    do
    {
		// flush input and output buffers
    	tcflush(SerialPort, TCIOFLUSH);			

    	// Read one byte at a time
        encoded_length = read(SerialPort, &header, 1);

        // error handling
        if (encoded_length == -1)
        {
        	cerr << "Error reading serial port: " << strerror(errno)
                 << " (" << errno << ")" << endl;
        }
    }
    while (header != 0x00); // test if read byte is a zero

    // read raw sensor values
    while (1)
    {	
    	// reset buffer
    	memset(encoded_buffer, '\0', BUF_SIZE);

    	// Read bytes
    	encoded_length = read(SerialPort, encoded_buffer, ENCODED_PACKET_SIZE);

    	// error handling
    	if (encoded_length == -1)
    	{
        	cerr << "Error reading serial port: " << strerror(errno)
             	 << " (" << errno << ")" << endl;
			break;
    	}
		else if (encoded_length != ENCODED_PACKET_SIZE)
		{
			cerr << "Wrong encoded packet length: " << encoded_length << endl;
			break;
		}
		else if (encoded_buffer[ENCODED_PACKET_SIZE-1] != 0x00)
		{
			cerr << "Encoded packet fragmetation: ";
		    for (int i = 0; i < ENCODED_PACKET_SIZE; i++)
        	{
            	cout << setfill('0') << setw(2) << hex
                     << (unsigned short) encoded_buffer[i] << " ";
            }
			cout << endl;
			break;
		}

		// decode packet data
		decoded_length = cobs_decode(encoded_buffer, encoded_length, decoded_buffer);

		// error handling
		if (decoded_length == 0)
		{
			cerr << "Error: decoding serial packet failed" << endl;
			break;
		}

		// get crc from decoded packet
		crc_before = ((decoded_buffer[3] & 0xFF) << 8) | (decoded_buffer[4] & 0xFF);
		
		// recalculate crc after receiving packet
        crc_after = Crc16(decoded_buffer, 3);
		
		// check crc
		if (crc_after != crc_before)
		{
			cerr << "Error: CRC checksum mismatch" << endl;
			break;
		}
		
		// get sensor reading from decoded packet
        sensor_value = ((decoded_buffer[1] & 0xFF) << 8) | (decoded_buffer[2] & 0xFF);

		//
		// console output
		//
	
		if (Debug)
		{	
			// encoded packet
        	cout << "enc: ";
        	for (int i = 0; i < encoded_length; i++)
        	{
            	cout << setfill('0') << setw(2) << hex
                 	 << (unsigned short) encoded_buffer[i] << " ";
        	}

        	// decoded packet
        	cout << "dec: ";
        	for (int i = 0; i < decoded_length-1; i++)
        	{
            	cout << setfill('0') << setw(2) << hex
                 	 << (unsigned short) decoded_buffer[i] << " ";
        	}
        
			// crc
			cout << "crc: 0x" << setfill('0') << setw(4) << hex << crc_before 
			 	 << " 0x" << setfill('0') << setw(4) << hex << crc_after << " : ";

		}

		// sensor value
        cout << dec << sensor_value << endl; ;
    }
}

void Pulse::ReadSerialTrigger(void)
{

}

void Pulse::CreateRRDFile(const char * file)
{

}
