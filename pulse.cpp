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

// set trigger level
void SetTriggerLevel(int low, int high)
{

}

// put sensor into raw mode
void Pulse::RawMode(void)
{

}

// put sensor into trigger mode
void Pulse::TriggerMode(void)
{
	char ReadBuf[256];

	while (1)
    {
		// reset buffer
        memset(&ReadBuf, '\0', sizeof(ReadBuf));

        // Read bytes
        ssize_t length = read(SerialPort, &ReadBuf, sizeof(ReadBuf));

		if (length == -1)
        {
			cerr << "Error reading serial port: " << strerror(errno)
				 << " (" << errno << ")" << endl;
            break;
        }
        else
        {
            cout.write((char *)ReadBuf, length);
        }
    }
}

void Pulse::CreateRRDFile(const char * file)
{

]
