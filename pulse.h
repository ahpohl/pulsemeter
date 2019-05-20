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

	// RRD data methods
	void CreateRRDFile(const char * file);
};

#endif // PULSE_H
