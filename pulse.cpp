// c++ headers
#include <iostream>
#include <string>
#include <stdexcept>
#include <typeinfo>

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
    tty.c_cc[VMIN] = 0; // returning as soon as any data is received
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
	const int buf_size = 256;
	unsigned char serial_buffer[buf_size];
    unsigned char encoded_buffer[buf_size];
	unsigned char decoded_buffer[buf_size];
	int decoded_length = 0, encoded_length = 0;
	int length = 0;

    // print raw sensor values to console
    while (1)
    {		
    	// reset buffer
    	memset(encoded_buffer, '\0', buf_size);

    	// Read bytes
		do {
    		length = read(SerialPort, serial_buffer, buf_size);

    		// error handling
    		if (length == -1)
    		{
        		cerr << "Error reading serial port: " << strerror(errno)
             	 	 << " (" << errno << ")" << endl;
    		}
		} while ( length < 8);

		// decode packet data
		decoded_length = cobs_decode(serial_buffer, length, decoded_buffer);

		// write to console
        //cout.write((char *)decodedBuffer, length);
		cout << decoded_length << " packet size" << endl;
    }
}

void Pulse::CreateRRDFile(const char * file)
{

}
