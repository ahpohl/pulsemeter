Pulse energy meter with Arduino and simple LED sensor

References
1. Infrared light switch with Arduino to read your energy meter
   https://www.kompf.de/tech/emeir.html
2. Round Robin Database
   https://oss.oetiker.ch/rrdtool/
3. PVOutput is a free service for sharing and comparing PV output data.
   https://pvoutput.org/
4. Ilvesheim system on PVOutput
   https://pvoutput.org/intraday.jsp?id=74913&sid=66419
5. 123Solar Web Solar logger
   https://123solar.org/
6. PVOutput Pro mobile app
   https://apps.apple.com/au/app/pvoutput-pro/id994297624
7. Packet serial library for Arduino
   https://github.com/bakercp/PacketSerial
8. Consistent Overhead Byte Stuffing (C implementation)
   https://github.com/jacquesf/COBS-Consistent-Overhead-Byte-Stuffing

1 Introduction


There are many energy meters out there either with DIY hardware or commercial meters, which need to be permanently installed in your electrical switchboard. Before starting this project, I was inclined to buy one of these meters (modbus version of SDM...). I then decided to build one myself because of the cost involved to buy the commercial hardware and to get it professionally installed (I didn't want to tamper with the switchboard myself!). After searching around, I came across this project [1]. The hardware is really simple and the explanations of how the sensor and the Arduino code works are really easy to follow.

2 Hardware

I used a spare blue/white LED I had lying around from an old ceiling lamp instead of the IR LED. Because of the algorithm of how the sensor is read, it is still fairly tolerant to the ambient light and I found the IR diode is not necessary. The receiving side is a simple phototransistor I had in my electronics toys box. I do not have any data sheet, but it seems to work well. I soldered the sensor together according to the following scheme: [Fig. 1: schematic diagram]. The sensor is connected to the Arduino One (any cheap clone will also do, as long the µC has at least one ADC and a couple of GPIOs to connect the LEDs and the LCD) and the whole installation in the switchboard looks like this: [Fig. 2: photo of electrical switchbox]. The Arduino is connected via a 5 meter USB cable to an Odroid C2, which is housed in my network cabinet.

3 Software

The software constists of two components. First, an Arduino sketch to process the raw sensor values and to detect a revolution of the ferraris disc, and second a daemon which receives the energy counts from the Arduino and saves it into a Round Robin Database [2]. The daemon has the option to submit the consumed energy and power output to PVOutput [3]. Additionally, my house is equipped with a photo voltaic system [4] (production data are not yet gathered by Pulse, but using 123Solar [5] connected to the PowerOne Aurora inverter instead). The net electricity data from Pulse and 123Solar can be conveniently montitored using the PVOutput mobile app [6].

3.1 Arduino sketch

The original command line interface [1] was removed from the Arduino sketch. The Arduino communicates with the daemon using a packet serial protocol with COBS encoding instead [7, 8]. The daemon sends two different commands to put the sensor either into raw or trigger mode. In raw mode, the Arduino responds with the raw sensor values which can be logged to a file to produce a plot like this: [Fig. 3: diagram of raw sensor values with intital trigger levels]. The raw sensor plot gives a good estimation for the intial low and high trigger threshold levels, although the trigger detection algorithm has been modified so that the low and high trigger levels are automatically adjusted after each revolution of the disc (currently, the low level will be 15 units above the minimum sensor value and the high level 30 units above the minimum). In trigger mode, the Arduino sends a "1" if the sensor value falls below the low threshold, and a "0" if the sensor value has raised above the high level after being low to indicate that one complete revolution of the disc has passed (these trigger events will be counted by the daemon to calculate the energy consumed). The raw sensor value and the revolution counter are displayed on a 16x2 character LCD [Fig. 4: LCD with annotation of fields].

3.2 Pulse daemon

The daemon counts the 


3.3 Serial protocol


3.4 Build instructions


4 Acknowledgements


5 References
