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
	int OpenSerialPort(const char * device);
	void ReadSerialRaw(void);
	void ReadSerialTrigger(void);
	unsigned short Crc16(unsigned char * data_p, unsigned short length);

	// RRD data methods
	void CreateRRDFile(const char * file);
};

#endif // PULSE_H
