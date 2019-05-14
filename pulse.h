#ifndef PULSE_H
#define PULSE_H
#include <string>

class Pulse
{
private:
	int SerialPort;
	uint8_t ReadBuf[256];
	uint8_t WriteBuf[256];

public:
	void OpenSerialPort(std::string device);
    void CloseSerialPort(void);
    void ReadSerialPort(void);
	Pulse(void);
	~Pulse(void);
};

#endif // PULSE_H
