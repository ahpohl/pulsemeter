#ifndef PULSE_H
#define PULSE_H

#include <rrd.h>
#include <rrd_client.h>

class Pulse
{
private:
	bool Debug;
	int SerialPort;
	int RevPerKiloWattHour;
	int SensorValue;
	unsigned long LastEnergyCounter;
	unsigned long EnergyCounter;
	const char * RRDCachedAddress;
    const char * RRDFile;

	// sensor methods
	unsigned short Crc16(unsigned char * data_p, int length);
    void ConfigureSerialPort(unsigned char vmin, unsigned char vtime);
    void SendCommand(unsigned char * command, int command_length);
    bool SyncPacket(void);
    void ReceivePacket(unsigned char * packet, int buffer_size);

public:
	Pulse(const char * rrd_file, const char * rrdcached_address,
		double meter_reading, int rev_per_kWh);
	Pulse(const Pulse & obj);
	~Pulse(void);
	void SetDebug(void);

	// sensor methods
	void OpenSyncSerialPort(const char * serial_device);
	void SetRawMode(void);
	void SetTriggerMode(short int trigger_level_low, 
		short int trigger_level_high);
	int ReadSensorValue(void);

	// rrd methods
    void RRDClientCreate(void);
    void RRDClientUpdateEnergyCounter(void);
	unsigned long RRDGetLastEnergyCounter(void);
    double RRDGetEnergy(void);

	// thread functions
	void RunTriggerThread(void);
};

#endif // PULSE_H
