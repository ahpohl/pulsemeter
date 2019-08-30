# Pulse

Pulse energy meter with Arduino and simple LED sensor

## Introduction

There are many energy meters out there either with DIY hardware or commercial meters, which need to be permanently installed in your electrical switchboard. Before starting this project, I was inclined to buy one of these meters (modbus version of SDM...). I then decided to build one myself because of the cost involved to buy the commercial hardware and to get it professionally installed (I didn't want to tamper with the switchboard myself!). After searching around, I came across this project [1]. The hardware is really simple and the explanations of how the sensor and the Arduino code works are really easy to follow.

## Hardware

The hardware is built around an Arduino One, a character LCD and a sensor made from a blue/white LED and a simple photo transitor (I do not have a datasheet but it seems to work well). Because of the algorithm of how the sensor is read, it is still fairly tolerant to the ambient light and I found an IR diode to block the light is not necessary. The Arduino (any cheap clone will also do, as long the µC has at least one ADC and a couple of GPIOs to connect the LEDs and the LCD) is connected via a 5 meter USB cable to an Odroid C2, which is housed in my network cabinet. The whole installation in the switchboard looks like this:

![Fig. 2: photo of electrical switchbox](https://github.com/ahpohl/pulse/blob/master/resources/ferraris_meter.jpg)

The components are wired according to the following scheme:

![Fig. 1: schematic diagram](https://github.com/ahpohl/pulse/blob/master/resources/schematic.png)

## Software

The software constists of two components. First, an Arduino sketch to process the raw sensor values and to detect a revolution of the ferraris disc, and second a daemon which receives the energy counts from the Arduino and saves it into a Round Robin Database [2]. The daemon has the option to submit the consumed energy and power output to PVOutput [3]. Additionally, my house is equipped with a photo voltaic system [4] (production data are not yet gathered by Pulse, but using 123Solar [5] connected to the PowerOne Aurora inverter instead). The net electricity data from Pulse and 123Solar can be conveniently montitored using the PVOutput mobile app [6].

### Arduino sketch

The original command line interface [1] was removed from the Arduino sketch. The Arduino communicates with the daemon using a packet serial protocol with COBS encoding instead [7, 8]. The daemon sends two different commands to put the sensor either into raw or trigger mode. In raw mode, the Arduino responds with the raw sensor values which can be logged to a file to produce a plot like this: ![Fig. 3: diagram of raw sensor values with intital trigger levels](https://github.com/ahpohl/pulse/blob/master/resources/sensor.png)

The raw sensor plot gives a good estimation for the intial low and high trigger threshold levels, although the trigger detection algorithm has been modified so that the low and high trigger levels are automatically adjusted after each revolution of the disc (currently, the low level will be 15 units above the minimum sensor value and the high level 30 units above the minimum). In trigger mode, the Arduino sends a "1" if the sensor value falls below the low threshold, and a "0" if the sensor value has raised above the high level after being low to indicate that one complete revolution of the disc has passed (these trigger events will be counted by the daemon to calculate the energy consumed). The raw sensor value and the revolution counter are displayed on a 16x2 character LCD. 

![Fig. 4: LCD with annotation of fields](https://github.com/ahpohl/pulse/blob/master/resources/lcd.png)

### Pulse daemon program

The Pulse daemon counts the events received by the Arduino sensor and writes them into a Round Robin Database. The RRD database is accessed through the rrdcached daemon located at a local unix socket (remote access by IP address would be possible but is currently not implemented). The initial RRD database is created with the current meter reading and can be later updated if necessary, for example when the sensor was offline for some time without Pulse registering all trigger events. Both Pulse and RRDCached instances are started and stopped via the provided systemd startup scripts, and all parameters such as the intial meter reading are set in a separate systemd environment file called [pulse_defs.conf](https://github.com/ahpohl/pulse/blob/master/resources/pulse_defs.conf). The energy and power values recorded in the RRD database are optionally uploaded to PVOutput every 5, 10 or 15 minutes (at the intervals currently supported by the PVOutput api). See the 24-hour plot below. ![Fig. 5: PVOutput energy consumption plot](https://github.com/ahpohl/pulse/blob/master/resources/pulsemeter.png)

### Serial protocol

The serial protocol used to communiate between the Arduino sensor and the Pulse daemon program consists of fixed length packets encoded by the consistent overhead byte stuffing algorithm [9]. The command packets are seven bytes long, whereas the data packets are five bytes long.

```
       Command packet
       -----------------------------------------------------------------------
 byte  |    0    |    1    |    2    |    3    |    4    |    5    |    6    |
       |   cmd   |   low   |   low   |   high  |   high  |   crc   |   crc   |
       -----------------------------------------------------------------------
                     MSB       LSB

       Data packet
       ---------------------------------------------------
 byte  |    0    |    1    |    2    |    3    |    4    |
       | Status  |   val   |   val   |   crc   |   crc   |
       ---------------------------------------------------

       Explanation
       -----------
       a) cmd: 0x10 raw mode
               0x20 trigger mode
               0x30 sync packet
       b) low trigger level
       c) high trigger level
       d) CCITT CRC 16 checksum
       e) status: 0 sensor value
                  1 sync ok
                  2 ack raw mode 
                  3 ack trigger mode
                  4 unknown command
                  5 crc checksum mismatch
                  6 wrong packet size
       f) sensor value
```

## Installation

The Arduino sketch can be either uploaded using the standard Arduino IDE or via the provided programmer script if the sensor is alreay connected to the target embedded PC (such as Raspberry Pi, Odroid etc.; anything really which has at least one free USB port and a C++ compiler is available). The daemon is compiled and installed using the GNU Makefile. Note the program version string is automatically pulled in from the git tag (thanks to these instructions [10]). There is also an Arch Linux package available in the AUR called "pulse" [11].

## Changelog

All notable changes and releases are documented in the [CHANGELOG](https://github.com/ahpohl/pulse/blob/master/CHANGELOG.md).

## Acknowledgements

* Martin Kompf for the inspiration and the initial idea of the project
* Tobias Oetiker for the Round Robin Database library
* The PVOutput Team for providing an excellent website with a simple to use API
* Christopher Baker for the Arduino packet serial library and 
* Jacques Fortier for the C implementation of the COBS encoding
* Phillip Johnston for git versioning in a Makefile

## License

This project is licensed under the MIT license - see the [LICENSE](https://github.com/ahpohl/pulse/blob/master/LICENSE) file for details

## References

1. [Infrared light switch with Arduino to read your energy meter](https://www.kompf.de/tech/emeir.html)
2. [Round Robin Database](https://oss.oetiker.ch/rrdtool/)
3. [PVOutput is a free service for sharing and comparing PV output data.](https://pvoutput.org/)
4. [Ilvesheim system on PVOutput](https://pvoutput.org/intraday.jsp?id=74913&sid=66419)
5. [123Solar Web Solar logger](https://123solar.org/)
6. [PVOutput Pro mobile app](https://apps.apple.com/au/app/pvoutput-pro/id994297624)
7. [Packet serial library for Arduino](https://github.com/bakercp/PacketSerial)
8. [Consistent Overhead Byte Stuffing (C implementation)](https://github.com/jacquesf/COBS-Consistent-Overhead-Byte-Stuffing)
9. [Consistent Overhead Byte Stuffing (background)](https://en.wikipedia.org/wiki/Consistent_Overhead_Byte_Stuffing)
10. [Giving Your Firmware Build a Version](https://embeddedartistry.com/blog/2016/10/27/giving-you-build-a-version)
11. [Pulse Arch Linux package](https://aur.archlinux.org)
