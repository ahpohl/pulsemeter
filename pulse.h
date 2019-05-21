#ifndef PULSE_H
#define PULSE_H

class Pulse
{
private:
	int SerialPort;

public:
	~Pulse(void);

	// sensor methods
	int OpenSerialPort(const char * device);
	void ReadSerialRaw(void);
	unsigned short Crc16(unsigned char * data_p, unsigned short length);

	// RRD data methods
	void CreateRRDFile(const char * file);
};

#endif // PULSE_H
