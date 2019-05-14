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
#include "aurora.h"
#include "constants.h"
#include "strings.h"

using namespace std;

Aurora::Aurora(string device, int address)
	:SerialDevice(device), InverterAddress(address)
{
	OpenSerialConnection();
	SetupSerialConnection();
	//ReadSerialConnection();
}

Aurora::~Aurora(void)
{
	CloseSerialConnection();
}

int Aurora::OpenSerialConnection(void)
{	
	SerialPort = open(SerialDevice.c_str(), O_RDWR | O_NOCTTY);
	
	// error handling
	if (SerialPort < 0)
	{
		cerr << "Error opening device " << SerialDevice << ": "
             << strerror(errno) << " (" << errno << ")" << endl;
		return SerialPort;
	}
	// success
	else
	{
		cout << "Opened serial device " << SerialDevice << endl;
	}

	return SerialPort;
}

void Aurora::CloseSerialConnection(void)
{
	if (SerialPort > 0)
	{
		close(SerialPort);
		cout << "Closed serial device " << SerialDevice << endl;
	}	
}

int Aurora::SetupSerialConnection(void)
{
	int ret = 0; 

	// create new termios struc, we call it 'tty' for convention
    struct termios tty;
    memset(&tty, 0, sizeof(tty));

    // read in existing settings
    ret = tcgetattr(SerialPort, &tty);
	if (ret != 0)
    {
        cerr << "Error getting serial port attributes: " << strerror(errno) 
             << " (" << errno << ")" << endl;
        return ret;
    }

	// set raw mode
    cfmakeraw(&tty);

    // set vmin and vtime
    tty.c_cc[VMIN] = 0; // returning as soon as any data is received
    tty.c_cc[VTIME] = 10; // wait for up to 1 second

    // set in/out baud rate to be 19200 baud
    cfsetispeed(&tty, B19200);
    cfsetospeed(&tty, B19200);

    // save tty settings
    ret = tcsetattr(SerialPort, TCSANOW, &tty);
	if (ret != 0)
    {
		cerr << "Error setting serial port attributes: " << strerror(errno)
             << " (" << errno << ")" << endl;
		return ret;
    }

	return ret;
}

int Aurora::ReadSerialConnection(void)
{
	while (1)
    {
		// reset buffer
        memset(&ReadBuf, '\0', sizeof(ReadBuf));

        // Read bytes. The behaviour of read() (e.g. does it block?,
        // how long does it block for?) depends on the configuration
        // settings above, specifically VMIN and VTIME
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

	return 0;
}

uint16_t Aurora::CRC(uint8_t *data, int count)
{
    uint8_t BccLo = 0xFF, BccHi = 0xFF;
    uint8_t New = 0, Tmp = 0;
	uint16_t crc = 0;

    for (int i = 0; i < count; i++)
    {
        New = data[i] ^ BccLo;
        Tmp = New << 4;
        New = Tmp ^ New;
        Tmp = New >> 5;
        BccLo = BccHi;
        BccHi = New ^ Tmp;
        Tmp = New << 3;
        BccLo = BccLo ^ Tmp;
        Tmp = New >> 4;
        BccLo = BccLo ^ Tmp;
        crc = ((BccHi & 0xFF) << 8) | (BccLo & 0xFF);
        crc = ~crc;
    }

    return crc;
}
