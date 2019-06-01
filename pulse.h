#ifndef PULSE_H
#define PULSE_H

#include <rrd.h>
#include <rrd_client.h>

class Pulse
{
private:
	int SerialPort;
	bool Debug;
	int RevPerKiloWattHour;
	int SensorValue;
	unsigned long LastEnergyCounter;
	unsigned long EnergyCounter;
    rrd_client_t * RRDClient;
	const char * RRDAddress;
    const char * RRDFile;

	// sensor methods
	unsigned short Crc16(unsigned char * data_p, int length);
    void ConfigureSerialPort(unsigned char vmin, unsigned char vtime);
    void SendCommand(unsigned char * command, int command_length);
    bool SyncPacket(void);
    void ReceivePacket(unsigned char * packet, int buffer_size);

public:
	Pulse(const char * serial_device, const char * rrd_file, 
		double meter_reading, int rev_per_kWh);
	~Pulse(void);
	void SetDebug(void);

	// sensor methods
	void SyncSerial(void);
	void SetRawMode(void);
	void SetTriggerMode(short int trigger_level_low, short int trigger_level_high);
	int ReadSensorValue(void);

	// rrd methods
	void RRDConnect(const char * daemon_address);
	void RRDCreate(void);
	void RRDUpdateEnergyCounter(void);
	unsigned long RRDGetLastEnergyCounter(void);
	double RRDGetEnergy(void);
	double RRDGetPower(void);
};

#endif // PULSE_H
