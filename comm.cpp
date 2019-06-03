// c++ headers
#include <iostream>
#include <string>
#include <stdexcept>
#include <typeinfo>
#include <iomanip>

// c headers
#include <cstring>
#include <csignal>
#include <termios.h> // contains POSIX terminal control definition
#include <fcntl.h> // contains file controls like 0_RDWR
#include <unistd.h> // write(), read(), close()
#include <errno.h> // error integer and strerror() function
#include <math.h> 
#include <sys/ioctl.h> // contains ioctl_tty

// program headers
#include "pulse.h"
#include "cobs.h"
#include "constants.h"

using namespace std;

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

void Pulse::OpenSyncSerialPort(const char * serial_device)
{
	int ret = 0;
	unsigned char sync_command[COMMAND_PACKET_SIZE] = {0};
	unsigned short crc = 0xFFFF;
	unsigned char sync_buffer[BUF_SIZE];
	char header = 0xFF;
	int byte_received = 0;
	int count = 0;
	int garbage = 0;

    // open serial port 
    SerialPort = open(serial_device, O_RDWR | O_NOCTTY);
    if (SerialPort < 0)
    {
        throw runtime_error(string("Error opening device ") + serial_device + ": "
             + strerror(errno) + " (" + to_string(errno) + ")");
    }

    // lock serial port
    ret = ioctl(SerialPort, TIOCEXCL);
	if (ret < 0)
    {
        throw runtime_error(string("Error getting device lock on") + serial_device + ": "
             + strerror(errno) + " (" + to_string(errno) + ")");
    }

    if (Debug)
		cout << "Opened serial device " << serial_device << endl;

	// configure serial port for non-blocking read
	// vmin: return immediately if characters are available
	// vtime: or wait for max 0.1 sec
	ConfigureSerialPort(0, 1);

	// sync packet
	sync_command[0] = 0x30; // sync mode
    crc = Crc16(sync_command, 5);
    sync_command[5] = crc >> 8; // crc, high byte
    sync_command[6] = crc & 0xFF; // crc, low byte 

	// send sync packet until sensor responds with the first byte
	do
	{
    	// send command to sensor
    	SendCommand(sync_command, COMMAND_PACKET_SIZE);
		
		byte_received = read(SerialPort, &header, 1);
        
		// error handling
        if (byte_received == -1)
        {
            throw runtime_error(string("Error reading serial port: ")
                + strerror(errno) + " (" + to_string(errno) + ")");
        }
		
		if (count == 100)
        {
            throw runtime_error("Unable to sync serial port");
        }
		count++;
	}
	while (byte_received == 0);

	// output number of sync packets sent
	if (Debug)
		cout << "Sync packets sent (" << dec << count << ")" << endl;

	// empty serial input buffer
	do
	{
		byte_received = read(SerialPort, &header, 1);

        // error handling
        if (byte_received == -1)
        {
            throw runtime_error(string("Error reading serial port: ")
                + strerror(errno) + " (" + to_string(errno) + ")");
        }
		garbage++;
    }
    while (byte_received > 0);

	if (Debug)
		cout << "Serial input buffer empty (" << dec << garbage << ")" << endl;

	// wait until all remaining bytes are transmitted
	sleep(2);

    // set vmin and vtime for blocking read
	// vmin: returning when max 16 bytes are available
	// vtime: wait for up to 1 second
	ConfigureSerialPort(BUF_SIZE, 10);

	// send sync packet
    SendCommand(sync_command, COMMAND_PACKET_SIZE);

    // reset buffer
    memset(sync_buffer, '\0', BUF_SIZE);

    // get response
    ReceivePacket(sync_buffer, BUF_SIZE);

	if (sync_buffer[0] != SYNC_OK)
	{
		throw runtime_error("Communication failed: packets not in sync");
		//cerr << "Communication failed: packets not in sync" << endl;
		//return;
	}
	
	if (Debug)
		cout << "Communication ok" << endl;
}

void Pulse::ConfigureSerialPort(unsigned char vmin, unsigned char vtime)
{
	struct termios tty;
	int ret = 0;

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
    tty.c_cc[VMIN] = vmin;
    tty.c_cc[VTIME] = vtime;

    // set in/out baud rate
    cfsetispeed(&tty, B9600);
    cfsetospeed(&tty, B9600);

    // save tty settings
    ret = tcsetattr(SerialPort, TCSANOW, &tty);
	if (ret != 0)
    {
		throw runtime_error(string("Error setting serial port attributes: ") 
			+ strerror(errno) + " (" + to_string(errno) + ")");
    }
}

bool Pulse::SyncPacket(void)
{
	unsigned char header = 0xFF;
	int count = 0;
	int byte_received = 0;

	do
	{
    	// Read byte
    	byte_received = read(SerialPort, &header, 1);

    	// error handling
    	if (byte_received == -1)
    	{
        	throw runtime_error(string("Error reading serial port: ")
            	+ strerror(errno) + " (" + to_string(errno) + ")");
    	}

		count++;

		if (count == 100)
		{
			cerr << "Unable to sync packets" << endl;
			return false;
		}
		
	} while (header != 0x00);

	if (Debug)
		cout << "Packet re-sync successful (" << count << ")" << endl;

	return true;
}

// send command to sensor
void Pulse::SendCommand(unsigned char * command, int command_length)
{
	// command
    unsigned char cobs_command[BUF_SIZE] = {0};
	int cobs_command_length = 0;
	int bytes_sent = 0;
	
	// encode packet data
    cobs_command_length = cobs_encode(command, command_length, cobs_command);

	if (Debug)
	{
		cout << "Cmd: ";
        for (int i = 0; i < command_length; i++)
        {
            cout << hex << setfill('0') << setw(2) 
				 << (unsigned short) command[i] << " ";
        }	
		cout << ": cobs ";
		for (int i = 0; i < cobs_command_length; i++)
    	{
    		cout << hex << setfill('0') << setw(2) 
			     << (unsigned short) cobs_command[i] << " ";
    	}
    	cout << endl;
	}

    // send buffer via serial port
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
}

// receive response data packet
void Pulse::ReceivePacket(unsigned char * packet, int buffer_size)
{
	unsigned char cobs_packet[BUF_SIZE];
	int bytes_received = 0;
	unsigned short crc_before = 0, crc_after = 0;
	int packet_length = 0;
	bool is_synced = true;
	
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
		throw runtime_error(string("Wrong encoded packet length (") 
			+ to_string(bytes_received) + ")");
	}
	else if (cobs_packet[bytes_received-1] != 0x00)
	{
		if (is_synced != SyncPacket())
		{
			throw runtime_error("Packet framing error: out of sync");
		}
	}

	// reset packet buffer
	memset(packet, '\0', buffer_size);

	// decode packet data
	packet_length = cobs_decode(cobs_packet, bytes_received, packet);

	// error handling
	if (packet_length == 0)
	{
		throw runtime_error("Error decoding serial packet");
	}
    else if (packet_length != DATA_PACKET_SIZE)
    {
        throw runtime_error(string("Wrong decoded packet length (")
            + to_string(packet_length) + ")");
    }	

	// get crc from decoded packet
	crc_before = ((packet[3] & 0xFF) << 8) | (packet[4] & 0xFF);
		
	// recalculate crc after receiving packet
    crc_after = Crc16(packet, 3);
		
	// check crc
	if (crc_after != crc_before)
	{
		throw runtime_error(string("CRC checksum mismatch (") 
			+ to_string(crc_before) + " " + to_string(crc_after) + ")");
	}

	if (Debug)
    {
        // decoded packet
        cout << "Res: ";
        for (int i = 0; i < packet_length; i++)
        {
            cout << hex << setfill('0') << setw(2)
                 << (unsigned short) packet[i] << " ";
        }

        // encoded packet
        cout << "      : cobs ";
        for (int i = 0; i < bytes_received; i++)
        {
            cout << hex << setfill('0') << setw(2)
                 << (unsigned short) cobs_packet[i] << " ";
        }
		cout << endl;
    }
	
	return;
}

// set raw mode, command 0x10
void Pulse::SetRawMode()
{
    unsigned char command[COMMAND_PACKET_SIZE] = {0};
    unsigned short int crc = 0xFFFF;
	unsigned char packet_buffer[BUF_SIZE];

    // command
    command[0] = 0x10; // raw mode

    // calculate cyclic redundancy check value
    crc = Crc16(command, 5);
    command[5] = crc >> 8; // crc, high byte
    command[6] = crc & 0xFF; // crc, low byte

    // send command to sensor
    SendCommand(command, COMMAND_PACKET_SIZE);

    // reset buffer
    memset(packet_buffer, '\0', BUF_SIZE);

    // get response
    ReceivePacket(packet_buffer, BUF_SIZE);

    if (packet_buffer[0] != RAW_MODE)
    {
        throw runtime_error("Error: setting raw mode failed");
    }
	
	if (Debug)
		cout << "Sensor raw mode" << endl;
}

// set trigger mode, command 0x20
void Pulse::SetTriggerMode(short int trigger_level_low, short int trigger_level_high)
{
    unsigned char command[COMMAND_PACKET_SIZE] = {0};
    unsigned short int crc = 0xFFFF;
	unsigned char packet_buffer[BUF_SIZE];

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

    // reset buffer
    memset(packet_buffer, '\0', BUF_SIZE);

    // get response
    ReceivePacket(packet_buffer, BUF_SIZE);

    if (packet_buffer[0] != TRIGGER_MODE)
    {
        throw runtime_error("Error: setting trigger mode failed");
    }

	// output
	if (Debug)
	{
        cout << "Sensor trigger mode" << endl;
		cout << "Trigger level: " << dec << trigger_level_low 
			 << " " << trigger_level_high << endl;
	}
}

int Pulse::ReadSensorValue(void)
{
	unsigned char packet[BUF_SIZE];

	// reset buffer
	memset(packet, '\0', BUF_SIZE);

	// get response
	ReceivePacket(packet, BUF_SIZE);

	if (packet[0] != SENSOR_VALUE)
	{
		throw runtime_error("Error: packet not a sensor reading");
	}

	// get sensor reading from decoded packet
    SensorValue = (short) ((packet[1] & 0xFF) << 8) | (packet[2] & 0xFF);

	// screen output
	cout << dec << SensorValue << endl;

	return SensorValue;
}
