#ifndef PULSE_H
#define PULSE_H

#include <rrd.h>
#include <rrd_client.h>
#include <ctime>

class Pulse
{
private:
	bool Debug;
	int SerialPort;
	int RevPerKiloWattHour;
	int SensorValue;
	unsigned long LastEnergyCounter;
	const char * RRDCachedAddress;
    const char * RRDFile;
	time_t RRDTime;
	double Energy;
	double Power;
	const char * PVOutputApiKey;
	const char * PVOutputSysId;

	// sensor methods
	unsigned short Crc16(unsigned char * data_p, int length);
    void ConfigureSerialPort(unsigned char vmin, unsigned char vtime);
    void SendCommand(unsigned char * command, int command_length);
    bool SyncPacket(void);
    void ReceivePacket(unsigned char * packet, int buffer_size);

public:
	Pulse(const char * rrd_file, const char * rrdcached_address,
		double meter_reading, int rev_per_kWh,
		const char * pvoutput_api_key, const char * pvoutput_system_id);
	~Pulse(void);
	void SetDebug(void);

	// sensor methods
	void OpenSyncSerialPort(const char * serial_device);
	void SetRawMode(void);
	void SetTriggerMode(short int trigger_level_low, 
		short int trigger_level_high);
	int ReadSensorValue(void);

	// rrd methods
    void RRDCreate(void);
    void RRDUpdateEnergyCounter(void);
	unsigned long RRDGetLastEnergyCounter(void);
    double RRDGetEnergyMeterN(void);
	void RRDGetEnergyAndPower(time_t end_time);
	void RRDUploadToPVOutput(time_t end_time);
};

#endif // PULSE_H
