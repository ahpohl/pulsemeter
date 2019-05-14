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
	void SetTriggerLevel(int low, int high);
	void RawMode(void);
	void TriggerMode(void);

	// RRD data methods
	void CreateRRDFile(const char * file);
};

#endif // PULSE_H
