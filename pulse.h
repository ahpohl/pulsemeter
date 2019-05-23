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
	unsigned short Crc16(unsigned char * data_p, unsigned short length);
	int OpenSerialPort(const char * device);
	int SendCommand(unsigned char * decoded_buffer, unsigned short decoded_length);
	void SetRawMode(void);
	void SetTriggerMode(short int trigger_level_low, short int trigger_level_high);
	void ReadSensorValue(void);

	// RRD data methods
	void CreateRRDFile(const char * file);
};

#endif // PULSE_H
