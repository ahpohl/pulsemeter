#ifndef PULSE_H
#define PULSE_H

class Pulse
{
private:
	int SerialPort;

public:
	~Pulse(void);
	int OpenSerialPort(const char * device);
	void ReadSerialPort(void);
	void SetTriggerLevel(int low, int high);
	void RawMode(void);
	void TriggerMode(void);
	void CreateRRDFile(const char * file);
};

#endif // PULSE_H
