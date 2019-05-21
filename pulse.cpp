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
	const int buf_size = 16;
    unsigned char encoded_buffer[buf_size];
	unsigned char decoded_buffer[buf_size];
	int length = 0;
	int packet_length = 0;
	unsigned char header;
	unsigned short crc = 0;

	// synchronise with sensor data stream
	do
	{
        // Read one byte at a time
        length = read(SerialPort, &header, 1);

        // error handling
        if (length == -1)
        {
            cerr << "Error reading serial port: " << strerror(errno)
                 << " (" << errno << ")" << endl;
        }
	}
	while (header != 0x00); // test if read byte is a zero

    // print raw sensor values to console
    while (1)
    {		
    	// reset buffer
    	memset(encoded_buffer, '\0', buf_size);

    	// Read bytes
    	length = read(SerialPort, encoded_buffer, 7);

    	// error handling
    	if (length == -1)
    	{
        	cerr << "Error reading serial port: " << strerror(errno)
             	 << " (" << errno << ")" << endl;
    	}

		// decode packet data
		packet_length = cobs_decode(encoded_buffer, length, decoded_buffer);

		// write to console
		for (int i = 0; i < packet_length-1; i++)
		{
        	//cout.write((char *)decoded_buffer, packet_length);
			cout << setfill('0') << setw(2) << hex 
				 << (unsigned short) decoded_buffer[i] << " ";
		}

		// calculate crc16 of data inside packet
		crc = Crc16(decoded_buffer, 3);
		
		cout << " crc: 0x" << hex << setfill('0') 
			 << setw(4) << crc << endl;
    }
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

void Pulse::CreateRRDFile(const char * file)
{

}
