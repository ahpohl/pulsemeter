#ifndef PULSE_H
#define PULSE_H

class Pulse
{
private:
	int SerialPort;
	bool Debug;

public:
	~Pulse(void);
	void SetDebug(void);

	// sensor methods
	unsigned short Crc16(unsigned char * data_p, int length);
	int OpenSerialPort(const char * device);
	void SendCommand(unsigned char * command, int command_length);
	bool SyncPacket(unsigned char * cobs_packet, int cobs_packet_size);
	void ReceivePacket(unsigned char * packet, int buffer_size);
	void SetRawMode(void);
	void SetTriggerMode(short int trigger_level_low, short int trigger_level_high);
	int ReadSensorValue(void);

	// RRD data methods
	void CreateRRDFile(const char * file);
};

#endif // PULSE_H
