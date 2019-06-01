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

public:
	Pulse(const char * serial_device, const char * rrd_file, 
		double meter_reading, int rev_per_kWh);
	~Pulse(void);
	void SetDebug(void);

	// sensor methods
	unsigned short Crc16(unsigned char * data_p, int length);
	void ConfigureSerialPort(unsigned char vmin, unsigned char vtime);
	void SendCommand(unsigned char * command, int command_length);
	void SyncSerial(void);
	bool SyncPacket(void);
	void ReceivePacket(unsigned char * packet, int buffer_size);
	void SetRawMode(void);
	void SetTriggerMode(short int trigger_level_low, short int trigger_level_high);
	int ReadSensorValue(void);

	// RRD data methods
	void RRDConnect(const char * daemon_address);
	void RRDCreate(void);
	void RRDUpdateEnergyCounter(void);
	unsigned long RRDGetLastEnergyCounter(void);
	void RRDGetEnergyMeterN(char * energy_string, int length);
	void RRDGetPowerMeterN(char * power_string, int length);
};

#endif // PULSE_H
