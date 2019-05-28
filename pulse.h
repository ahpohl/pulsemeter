#ifndef PULSE_H
#define PULSE_H

class Pulse
{
private:
	int SerialPort;
	bool Debug;

public:
	Pulse(const char * device);
	~Pulse(void);
	void SetDebug(void);

	// sensor methods
	unsigned short Crc16(unsigned char * data_p, int length);
	void ConfigureSerialPort(unsigned char vmin, unsigned char vtime);
	void SendCommand(unsigned char * command, int command_length);
	void SyncSerial(void);
	bool SyncPacket(void);
	void ReceivePacket(unsigned char * packet, int buffer_size);
	void SetRawMode(void);
	void SetTriggerMode(short int trigger_level_low, short int trigger_level_high);
	int ReadSensorValue(void);

	// RRD data methods
	void CreateRRD(const char * filename);
};

#endif // PULSE_H
