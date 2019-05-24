/*
* Program to read the electrical meter using a reflective light sensor
* This is the data acquisition part running on an Arduino Nano.
* It controls the infrared light barrier, detects trigger
* and communicates with a master computer (Raspberry Pi)
* over USB serial.
*/

#include <LiquidCrystal.h>
#include <PacketSerial.h>

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// pin definitions
const int analogInPin = A0;  // Analog input pin that the photo transistor is attached to
const int irOutPin = 9; // Digital output pin that the LED is attached to
const int ledOutPin = 8; // Signal LED output pin

// sensor values
volatile int sensorValueOff = 0;  // value read from the photo transistor when ir LED is off
volatile int sensorValueOn = 0;  // value read from the photo transistor when ir LED is on

// trigger state and level
volatile int triggerLevelLow;
volatile int triggerLevelHigh;
volatile boolean triggerState = false;

// By default, PacketSerial uses COBS encoding and has a 256 byte receive
// buffer. This can be adjusted by the user by replacing `PacketSerial` with
// a variation of the `PacketSerial_<COBS, 0, BufferSize>` template found in
// PacketSerial.h.
PacketSerial myPacketSerial;

// size of a sensor value packet
const int PACKET_SIZE = 5;

// sensor mode, R: raw, T: trigger
volatile char mode = '\0';

// transmission state:
enum {SENSOR_VALUE, 
      ACK,
      UNKNOWN_COMMAND, 
      CRC_CHECKSUM_MISMATCH, 
      WRONG_PACKET_SIZE};

/*--------------------------------------------------------------------------
    crc16
                                         16   12   5
    this is the CCITT CRC 16 polynomial X  + X  + X  + 1.
    This is 0x1021 when x is 2, but the way the algorithm works
    we use 0x8408 (the reverse of the bit pattern).  The high
    bit is always assumed to be set, thus we only use 16 bits to
    represent the 17 bit value.
----------------------------------------------------------------------------*/
#define POLY 0x8408   /* 1021H bit reversed */

unsigned int crc16(byte *data_p, unsigned int length)
{
      byte i;
      unsigned int data;
      unsigned int crc = 0xffff;

      if (length == 0)
        return (~crc);
      do
      {
        for (i=0, data=(unsigned int)0xff & *data_p++;
         i < 8;
         i++, data >>= 1)
        {
          if ((crc & 0x0001) ^ (data & 0x0001))
            crc = (crc >> 1) ^ POLY;
          else  crc >>= 1;
        }
      } while (--length);

      crc = ~crc;

      return (crc);
}

/**
 * Detect and print a trigger event
 */
void detectTrigger(int val)
{
  boolean nextState = triggerState;
  if (val > triggerLevelHigh) {
    nextState = true;
  } else if (val < triggerLevelLow) {
    nextState = false;
  }
  if (nextState != triggerState) {
    triggerState = nextState;
    // control internal LED
    digitalWrite(ledOutPin, triggerState);

    // output trigger count on LCD
    if (triggerState == 0) {
      static unsigned long triggerCount;
      triggerCount++;
      lcd.clear();
      lcd.print("lo ");
      lcd.print(triggerLevelLow);
      lcd.print(" hi ");
      lcd.print(triggerLevelHigh);
      lcd.setCursor(0,1);
      lcd.print(triggerCount);
    }

    // send triggerState value via serial
    sendSensorValue(triggerState? 1 : 0, SENSOR_VALUE);
  }
}

void lcdPrintRaw(int val)
{
  unsigned long currentMillis = millis();
  static unsigned long previousMillis;

  if (currentMillis - previousMillis > 1000) {
    previousMillis = currentMillis;
    lcd.clear();
    lcd.print("Raw mode");
    lcd.setCursor(0,1);
    lcd.print(val);
  }
}

// send packet via serial line
void sendSensorValue(int val, int state) 
{
  byte buf[PACKET_SIZE] = {0};
  unsigned int crc = 0xFFFF;

  // transmission state
  buf[0] = state;

  // sensor value
  buf[1] = val >> 8; // sensor value high byte
  buf[2] = val & 0xFF; // sensor value low byte

  // cyclic redundancy check
  crc = crc16(buf, 3);
  buf[3] = crc >> 8; // crc high byte
  buf[4] = crc & 0xFF; // crc low byte

  // send packet (3x)
  for (int i = 0; i < 2; i++)
  {
    myPacketSerial.send(buf, PACKET_SIZE);
  }
}

// This is our handler callback function.
// When an encoded packet is received and decoded, it will be delivered here.
// The `buffer` is a pointer to the decoded byte array. `size` is the number of
// bytes in the `buffer`.
void onPacketReceived(const uint8_t* decoded_buffer, size_t decoded_length)
{
  unsigned int crc_before = 0, crc_after = 0;

  lcd.clear();
  lcd.print("packet received");

  // check received packet length
  if (decoded_length != PACKET_SIZE)
  {
    sendSensorValue(decoded_length, WRONG_PACKET_SIZE);
    lcd.print("packet size");
    return;
  }

  // check crc
  crc_before = ((decoded_buffer[5] & 0xFF) << 8) | (decoded_buffer[6] & 0xFF);
  crc_after = crc16(decoded_buffer, decoded_length);

  if (crc_before != crc_after)
  {
    sendSensorValue(crc_after, CRC_CHECKSUM_MISMATCH);
    lcd.print("CRC checksum");
    return;
  }
  
  // set mode
  switch (decoded_buffer[0]) {
  case 0x10:
    mode = 'R'; // raw mode
    break;
  case 0x20:
    mode = 'T'; // trigger mode
    triggerLevelLow = ((decoded_buffer[1] & 0xFF) << 8) | (decoded_buffer[2] & 0xFF);
    triggerLevelHigh = ((decoded_buffer[3] & 0xFF) << 8) | (decoded_buffer[4] & 0xFF);
    break;
  default:
    mode = '\0'; // unknown command received
    sendSensorValue(decoded_buffer[0], UNKNOWN_COMMAND);
    lcd.print("unknown cmd");
    return;
  }

  // send acknowledgement packet
  sendSensorValue(0, ACK);
}


/**
 * Setup.
 */
void setup() {
  // initialize the digital pins as an output.
  pinMode(irOutPin, OUTPUT);
  pinMode(ledOutPin, OUTPUT);
  
  // We begin communication with our PacketSerial object by setting the
  // communication speed in bits / second (baud).
  myPacketSerial.begin(9600);

  // If we want to receive packets, we must specify a packet handler function.
  // The packet handler is a custom function with a signature like the
  // onPacketReceived function below.
  myPacketSerial.setPacketHandler(&onPacketReceived);
  
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
}

/**
 * Main loop.
 */
void loop() {
  
  // perform measurement
  // turn IR LED off
  digitalWrite(irOutPin, LOW);
  delay(10);
  // read the analog in value:
  sensorValueOff = analogRead(analogInPin);           
  // turn IR LED on
  digitalWrite(irOutPin, HIGH);
  delay(10);
  // read the analog in value:
  sensorValueOn = analogRead(analogInPin);

  // raw mode
  if (mode == 'R')
  {
    // send raw sensor value over serial
    // sendSensorValue(sensorValueOn - sensorValueOff, 0);

    // output raw value on LCD
    lcdPrintRaw(sensorValueOn - sensorValueOff);  
  }

  // detect trigger
  else if (mode == 'T')
  {
    detectTrigger(sensorValueOn - sensorValueOff);
  }

  // The PacketSerial::update() method attempts to read in any incoming serial
  // data and emits received and decoded packets via the packet handler
  // function specified by the user in the void setup() function.
  //
  // The PacketSerial::update() method should be called once per loop(). Failure
  // to call the PacketSerial::update() frequently enough may result in buffer
  // serial overflows.
  myPacketSerial.update();
}
