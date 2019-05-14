#include <iostream>
#include <string>
#include "pulse.h"

using namespace std;

int main(int argc, char* argv[])
{
	Pulse meter;
	meter.OpenSerialPort("/dev/ttyACM0");
	meter.ReadSerialPort();
	
	return 0;
}
