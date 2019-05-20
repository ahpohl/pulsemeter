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

// read serial device into buffer
int Pulse::ReadSerialPort(char * buffer, int size)
{
	int length = 0;

	// reset buffer
    memset(buffer, '\0', size);

   	// Read bytes
	length = read(SerialPort, buffer, size);

	// error handling
    if (length == -1)
    {
		cerr << "Error reading serial port: " << strerror(errno)
             << " (" << errno << ")" << endl;
    }

	return length;
}

// set sensor mode, R: raw, T: trigger
void Pulse::SetSensorMode(char mode)
{
	const int BufSize = 256;
    char ReadBuf[BufSize];
	int length;
	char msg[] = {'C','\n',mode,'\n'};

	// put sensor in mode
	length = write(SerialPort, msg, sizeof(msg)/sizeof(msg[0]));
    if (length == -1)
    {
        cerr << "Error sending command: " << strerror(errno)
             << " (" << errno << ")" << endl;
    }
	else if (mode == 'R')
	{
		cout << "Raw mode: " << length << " bytes written" << endl;
	}
	else if (mode == 'T')
	{
		cout << "Trigger mode: " << length << " bytes written" << endl;
	}
	else
	{
		cerr << "unknown mode" << endl;
	}

    // print raw sensor values to console
    while (1)
    {		
        // Read bytes
        length = ReadSerialPort(ReadBuf, BufSize);

		// write to console
        cout.write(ReadBuf, length);
    }
}

// set trigger level
void Pulse::SetTriggerLevel(int low, int high)
{

}

void Pulse::CreateRRDFile(const char * file)
{

}
