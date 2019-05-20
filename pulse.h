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
	int ReadSerialPort(char * buffer, int size);
	void SetTriggerLevel(int low, int high);
	void SetSensorMode(char mode);

	// RRD data methods
	void CreateRRDFile(const char * file);
};

#endif // PULSE_H
