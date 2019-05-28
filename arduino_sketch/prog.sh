#/bin/sh
avrdude -patmega328p -carduino -P/dev/ttyACM0 -b115200 -D -Uflash:w:pulsemeter.ino.hex
