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

// trigger level (set via serial command)
volatile int triggerLevelLow = 0;
volatile int triggerLevelHigh = 0;
volatile int sensorMax = 0;
volatile int sensorMin = 1000;

// By default, PacketSerial uses COBS encoding and has a 256 byte receive
// buffer. This can be adjusted by the user by replacing `PacketSerial` with
// a variation of the `PacketSerial_<COBS, 0, BufferSize>` template found in
// PacketSerial.h.
PacketSerial myPacketSerial;

// size of a sensor value packet
const int DATA_PACKET_SIZE = 5;
const int COMMAND_PACKET_SIZE = 7;

// sensor mode, R: raw, T: trigger
volatile char mode = '\0';

// transmission state:
enum transmissionStatus {SENSOR_VALUE, 
      SYNC_OK,
      RAW_MODE,
      TRIGGER_MODE,
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

// This is our handler callback function.
// When an encoded packet is received and decoded, it will be delivered here.
// The `buffer` is a pointer to the decoded byte array. `size` is the number of
// bytes in the `buffer`.
void onPacketReceived(const uint8_t* decoded_buffer, size_t decoded_length)
{
  unsigned int crc_before = 0, crc_after = 0;

  // reset mode (to silence sensor output)
  mode = '\0';

  // check received packet length
  if (decoded_length != COMMAND_PACKET_SIZE)
  { 
    sendSensorValue(decoded_length, WRONG_PACKET_SIZE);
    lcd.clear();
    lcd.print("packet size");
    lcd.setCursor(0,1);
    lcd.print(decoded_length);
    return;
  }

  // check crc
  crc_before = ((decoded_buffer[5] & 0xFF) << 8) | (decoded_buffer[6] & 0xFF);
  crc_after = crc16(decoded_buffer, 5);

  if (crc_before != crc_after)
  {
    sendSensorValue(crc_after, CRC_CHECKSUM_MISMATCH);
    lcd.clear();
    lcd.print("CRC checksum");
    lcd.setCursor(0,1);
    lcd.print(crc_before);
    lcd.print(" ");
    lcd.print(crc_after);
    return;
  }
  
  // set mode
  switch (decoded_buffer[0]) {
  case 0x10:
    mode = 'R'; // raw mode
    sendSensorValue(0, RAW_MODE);
    break;
  case 0x20:
    mode = 'T'; // trigger mode
    sendSensorValue(0, TRIGGER_MODE);
    triggerLevelLow = ((decoded_buffer[1] & 0xFF) << 8) | (decoded_buffer[2] & 0xFF);
    triggerLevelHigh = ((decoded_buffer[3] & 0xFF) << 8) | (decoded_buffer[4] & 0xFF);
    lcdPrintTrigger(0);
    break;
  case 0x30:
    sendSensorValue(0, SYNC_OK);
    lcd.clear();
    lcd.print("Sync OK");
    break;
  default: // unknown command received
    sendSensorValue(decoded_buffer[0], UNKNOWN_COMMAND);
    lcd.clear();
    lcd.print("unknown cmd");
    break;
  }
}

// send packet via serial line
void sendSensorValue(int val, int state) 
{
  byte buf[DATA_PACKET_SIZE] = {0};
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

  myPacketSerial.send(buf, DATA_PACKET_SIZE);
}

/**
 * Detect and print a trigger event
 */
void detectTrigger(int val)
{
  // adjust low and high marks
  if (val > sensorMax)
  {
    sensorMax = val;  
  } 
  else if (val < sensorMin)
  {
    sensorMin = val;
  }

  // detect trigger
  static boolean triggerState = false;
  
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
      lcdPrintTrigger(triggerCount);
      // reset min max sensor values
      sensorMax = 0;
      sensorMin = 1000;
    }

    // send triggerState value via serial (inverted)
    sendSensorValue(triggerState? 0 : 1, SENSOR_VALUE);
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

void lcdPrintTrigger(int val)
{
  lcd.clear();
  lcd.print(sensorMin);
  lcd.print(" ");
  lcd.print(sensorMax);
  lcd.print(" ");
  lcd.print(sensorMax - sensorMin);
  lcd.setCursor(0,1);
  lcd.print(val);
}

void getSensorValue(int sensor_delay)
{
  // sensor values
  int sensorValueOn = 0;
  int sensorValueOff = 0;

  // save ledstate
  static boolean ledState = LOW;

  static unsigned long previousMillis = 0;
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= sensor_delay)
  {
    // save the last time you blinked the LED
    previousMillis = currentMillis;

    // if the LED is off turn it on and vice-versa:
    if (ledState == LOW) {
      sensorValueOff = analogRead(analogInPin);
      ledState = HIGH;
    } else {
      sensorValueOn = analogRead(analogInPin);
      ledState = LOW;

      // raw mode
      if (mode == 'R')
      {
        // send raw sensor value over serial
        sendSensorValue(sensorValueOn - sensorValueOff, 0);

        // output raw value on LCD
        lcdPrintRaw(sensorValueOn - sensorValueOff);
      }

      // detect trigger
      else if (mode == 'T')
      {
        detectTrigger(sensorValueOn - sensorValueOff);
      }          
    }

    // set the LED with the ledState of the variable:
    digitalWrite(irOutPin, ledState);
  }
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

  if ((mode == 'R') || (mode == 'T'))
  {
    // get sensor value   
    getSensorValue(20);
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
