/*
  DoinoCoinI2C_Tiny_Slave.ino
  adapted from Luiz H. Cassettari
  by JK Rolling
*/
#pragma GCC optimize ("-Ofast")
#include <ArduinoUniqueID.h>  // https://github.com/ricaun/ArduinoUniqueID
#include <Wire.h>
#include "duco_hash.h"

/****************** USER MODIFICATION START ****************/
#define DEV_INDEX                   0
#define I2CS_START_ADDRESS          1
#define I2CS_FIND_ADDR              false
#define WDT_EN                      true        // recommended 'true', but do not turn on if using old bootloader
#define CRC8_EN                     true
#define LED_EN                      true
#define SENSOR_EN                   false
/****************** USER MODIFICATION END ******************/
/*---------------------------------------------------------*/
/****************** FINE TUNING START **********************/
#define LED_PIN                     LED_BUILTIN  // Uno / Nano onboard LED (13) does not support brightness control. use pin (3, 5, 6, 9, 10, or 11) instead
#define LED_BRIGHTNESS              255          // 1-255
#define LED_PROTECT                 true         // protect LED overvoltage. only set to false for LED with current limiting resistor or matching specs LED
#define BLINK_SHARE_FOUND           1
#define BLINK_SETUP_COMPLETE        2
#define BLINK_BAD_CRC               3
//#define SERIAL_LOGGER               Serial
#define I2CS_MAX                    32
#define WORKER_NAME                 "atmega328p"
#define WIRE_CLOCK                  100000
#define TEMPERATURE_OFFSET          338
#define FILTER_LP                   0.1
/****************** FINE TUNING END ************************/
uint8_t led_brightness = LED_BRIGHTNESS;
#if WDT_EN
#include <avr/wdt.h>
#endif

// ATtiny85 - http://drazzy.com/package_drazzy.com_index.json
// SCL - PB2 - 2
// SDA - PB0 - 0

// Nano/UNO
// SCL - A5
// SDA - A4

#ifdef SERIAL_LOGGER
#define SerialBegin()              SERIAL_LOGGER.begin(115200);
#define SerialPrint(x)             SERIAL_LOGGER.print(x);
#define SerialPrintln(x)           SERIAL_LOGGER.println(x);
#else
#define SerialBegin()
#define SerialPrint(x)
#define SerialPrintln(x)
#endif

#define BUFFER_MAX 90
#define HASH_BUFFER_SIZE 20
#define CHAR_END '\n'
#define CHAR_DOT ','

static const char DUCOID[] PROGMEM = "DUCOID";
static const char ZEROS[] PROGMEM = "000";
static const char WK_NAME[] PROGMEM = WORKER_NAME;
static const char UNKN[] PROGMEM = "unkn";
static const char ONE[] PROGMEM = "1";
static const char ZERO[] PROGMEM = "0";

static byte address;
static char buffer[BUFFER_MAX];
static uint8_t buffer_position;
static uint8_t buffer_length;
static bool working;
static bool jobdone;
static double temperature_filter;

void(* resetFunc) (void) = 0;//declare reset function at address 0
void Blink(uint8_t count, uint8_t pin, uint8_t dly);

// --------------------------------------------------------------------- //
// setup
// --------------------------------------------------------------------- //
void setup() {
  SerialBegin();

  if (SENSOR_EN) {
    analogReference (INTERNAL);
    temperature_filter = readTemperature();
  }

  if (LED_EN) {
    pinMode(LED_PIN, OUTPUT);
  }
  
  #if WDT_EN
    wdt_disable();
    wdt_enable(WDTO_4S);
  #endif

  initialize_i2c();

  Blink(BLINK_SETUP_COMPLETE, LED_PIN, 50);
  SerialPrintln("Startup Done!");
}

// --------------------------------------------------------------------- //
// loop
// --------------------------------------------------------------------- //

void loop() {
  do_work();
}

// --------------------------------------------------------------------- //
// run
// --------------------------------------------------------------------- //
boolean runEvery(unsigned long interval)
{
  static unsigned long previousMillis = 0;
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval)
  {
    previousMillis = currentMillis;
    return true;
  }
  return false;
}

// --------------------------------------------------------------------- //
// work
// --------------------------------------------------------------------- //
void do_work()
{
  if (working)
  {
    led_on();
    SerialPrintln(buffer);

    if (buffer[0] == '&') {
      resetFunc();
    }

    // worker related command
    if (buffer[0] == 'g') {
      // i2c_cmd
      //pos 0123[4]5678
      //    get,[c]rc8$
      //    get,[b]aton$
      //    get,[s]inglecore$
      //    get,[f]req$
      //    get,[i]d$
      char f = buffer[4];
      switch (tolower(f)) {
        case 't': // temperature
          if (SENSOR_EN) {
            temperature_filter = ( FILTER_LP * readTemperature()) + (( 1.0 - FILTER_LP ) * temperature_filter);
            ltoa(temperature_filter, buffer, 8);
          }
          else strcpy_P(buffer, ZERO);
          SerialPrint("SENSOR_EN: ");
          break;
        case 'f': // i2c clock frequency
          ltoa(WIRE_CLOCK, buffer, 10);
          SerialPrint("WIRE_CLOCK: ");
          break;
        case 'c': // crc8 status
          if (CRC8_EN) strcpy_P(buffer, ONE);
          else strcpy_P(buffer, ZERO);
          SerialPrint("CRC8_EN: ");
          break;
        case 'n': // worker name
          strcpy_P(buffer, WK_NAME);
          SerialPrint("WORKER: ");
          break;
        case 'i': // ducoid
          strcpy_P(buffer, DUCOID);
          for (uint8_t i = 0; i < 8; i++)
          {
            if (UniqueID8[i] < 16) strcpy(buffer + strlen(buffer), ZERO);
            itoa(UniqueID8[i], buffer+strlen(buffer), 16);
          }
          SerialPrint("DUCOID ");
          break;
        default:
          strcpy_P(buffer, UNKN);
          SerialPrint("command: ");
          SerialPrintln(f);
          SerialPrint("response: ");
      }
      SerialPrintln(buffer);
      buffer_position = 0;
      buffer_length = strlen(buffer);
      working = false;
      jobdone = true;
      #if WDT_EN
      wdt_reset();
      #endif
      return;
    }
    else if (buffer[0] == 's') {
      char f = buffer[4];
      // i2c_cmd
      //pos 0123[4]5678
      //    set,[d]im,123$
      switch (f) {
        case 'd': // led dim
          // Tokenize the buffer string using commas as delimiters
          char* token = strtok(buffer, ","); // "set"
          token = strtok(NULL, ",");         // "dim"
          token = strtok(NULL, "$");         // "123" (or "1")
        
          // Convert the extracted token to a number
          if (token != NULL) {
            uint8_t brightness = atoi(token);
            led_brightness = (LED_PROTECT && brightness > 127) ? 128 : brightness;
          }
          break;
      }
      buffer_position = 0;
      buffer_length = 0;
      working = false;
      return;
    }

    #if I2CS_FIND_ADDR
    if (buffer[0] == '@') {
      address = find_i2c();
      memset(buffer, 0, sizeof(buffer));
      buffer_position = 0;
      buffer_length = 0;
      working = false;
      jobdone = false;
      return;
    }
    #endif

    do_job();
  }
  led_off();
}

void do_job()
{
  uint16_t job = 1;
  #if CRC8_EN
  // calculate crc8
  char *delim_ptr = strchr(buffer, CHAR_DOT);
  delim_ptr = strchr(delim_ptr+1, CHAR_DOT);
  uint8_t job_length = strchr(delim_ptr+1, CHAR_DOT) - buffer + 1;
  uint8_t calc_crc8 = crc8((uint8_t *)buffer, job_length);

  // validate integrity
  char *rx_crc8 = strchr(delim_ptr+1, CHAR_DOT) + 1;
  if (calc_crc8 != atoi(rx_crc8)) {
    // data corrupted
    SerialPrintln("CRC8 mismatched. Abort..");
    job = 0;
  }
  #endif
  
  uint32_t startTime = millis();
  if (job)
    job = work();
  uint32_t elapsedTime = millis() - startTime;
  if (job<5) elapsedTime = job*(1<<2);
  
  memset(buffer, 0, sizeof(buffer));
  char cstr[6];
  
  // Job
  if (job == 0)
    strcpy(cstr,"#"); // re-request job
  else
    itoa(job, cstr, 10);
  strcpy(buffer, cstr);
  buffer[strlen(buffer)] = CHAR_DOT;

  // Time
  itoa(elapsedTime, cstr, 10);
  strcpy(buffer + strlen(buffer), cstr);
  
  #if CRC8_EN
  char gen_crc8[3];
  buffer[strlen(buffer)] = CHAR_DOT;

  // CRC8
  itoa(crc8((uint8_t *)buffer, strlen(buffer)), gen_crc8, 10);
  strcpy(buffer + strlen(buffer), gen_crc8);
  #endif

  SerialPrintln(buffer);

  buffer_position = 0;
  buffer_length = strlen(buffer);
  working = false;
  jobdone = true;
  
  #if WDT_EN
  wdt_reset();
  #endif
}

int work()
{
  // tokenize
  const char *delim = ",";
  char *lastHash = strtok(buffer, delim);
  char *newHash = strtok(NULL, delim);
  char *diff = strtok(NULL, delim);
  
  buffer_length = 0;
  buffer_position = 0;
  return work(lastHash, newHash, atoi(diff));
}

//#define HTOI(c) ((c<='9')?(c-'0'):((c<='F')?(c-'A'+10):((c<='f')?(c-'a'+10):(0))))
#define HTOI(c) ((c<='9')?(c-'0'):((c<='f')?(c-'a'+10):(0)))
#define TWO_HTOI(h, l) ((HTOI(h) << 4) + HTOI(l))
//byte HTOI(char c) {return ((c<='9')?(c-'0'):((c<='f')?(c-'a'+10):(0)));}
//byte TWO_HTOI(char h, char l) {return ((HTOI(h) << 4) + HTOI(l));}

void HEX_TO_BYTE(char * address, char * hex, int len)
{
  for (uint8_t i = 0; i < len; i++) address[i] = TWO_HTOI(hex[2 * i], hex[2 * i + 1]);
}

// DUCO-S1A hasher
uint32_t work(char * lastblockhash, char * newblockhash, int difficulty)
{
  if (difficulty > 655) return 0;
  HEX_TO_BYTE(newblockhash, newblockhash, HASH_BUFFER_SIZE);
  static duco_hash_state_t hash;
  duco_hash_init(&hash, lastblockhash);

  char nonceStr[10 + 1];
  int nonce_max = difficulty*100+1;
  for (int nonce = 0; nonce < nonce_max; nonce++) {
    ultoa(nonce, nonceStr, 10);

    uint8_t const * hash_bytes = duco_hash_try_nonce(&hash, nonceStr);
    if (memcmp(hash_bytes, newblockhash, HASH_BUFFER_SIZE) == 0) {
      return nonce;
    }
    #if WDT_EN
    if (runEvery(2000))
      wdt_reset();
    #endif
  }

  return 0;
}

// --------------------------------------------------------------------- //
// i2c
// --------------------------------------------------------------------- //

void initialize_i2c(void) {
  address = DEV_INDEX + I2CS_START_ADDRESS;
  
  #if I2CS_FIND_ADDR
  address = find_i2c();
  #endif

  Wire.setClock(WIRE_CLOCK);
  Wire.begin(address);
  Wire.onReceive(onReceiveJob);
  Wire.onRequest(onRequestResult);

  SerialPrint("Wire begin ");
  SerialPrintln(address);
}

void onReceiveJob(int howMany) {
  if (howMany == 0) return;
  if (working) return;
  if (jobdone) return;

  for (int i=0; i < howMany; i++) {
    if (i == 0) {
      char c = Wire.read();
      buffer[buffer_length++] = c;
      if (buffer_length == BUFFER_MAX) buffer_length--;
      if ((c == CHAR_END) || (c == '$'))
      {
        working = true;
      }
    }
    else {
      // dump the rest
      Wire.read();
    }
  }
}

void onRequestResult() {
  char c = CHAR_END;
  if (jobdone) {
    c = buffer[buffer_position++];
    if ( buffer_position == buffer_length) {
      jobdone = false;
      buffer_position = 0;
      buffer_length = 0;
      memset(buffer, 0, sizeof(buffer));
    }
  }
  Wire.write(c);
}

// --------------------------------------------------------------------- //
// I2CS_FIND_ADDR
// --------------------------------------------------------------------- //
#if I2CS_FIND_ADDR
byte find_i2c()
{
  address = I2CS_START_ADDRESS;
  unsigned long time = (unsigned long) getTrueRotateRandomByte() * 1000 + (unsigned long) getTrueRotateRandomByte();
  delayMicroseconds(time);
  Wire.setClock(WIRE_CLOCK);
  Wire.begin();
  int address;
  for (address = I2CS_START_ADDRESS; address < I2CS_MAX; address++)
  {
    Wire.beginTransmission(address);
    int error = Wire.endTransmission();
    if (error != 0)
    {
      break;
    }
  }
  Wire.end();
  return address;
}

// ---------------------------------------------------------------
// True Random Numbers
// https://gist.github.com/bloc97/b55f684d17edd8f50df8e918cbc00f94
// ---------------------------------------------------------------
#if defined(ARDUINO_AVR_PRO)
#define ANALOG_RANDOM A6
#else
#define ANALOG_RANDOM A1
#endif

const int waitTime = 16;

byte lastByte = 0;
byte leftStack = 0;
byte rightStack = 0;

byte rotate(byte b, int r) {
  return (b << r) | (b >> (8 - r));
}

void pushLeftStack(byte bitToPush) {
  leftStack = (leftStack << 1) ^ bitToPush ^ leftStack;
}

void pushRightStackRight(byte bitToPush) {
  rightStack = (rightStack >> 1) ^ (bitToPush << 7) ^ rightStack;
}

byte getTrueRotateRandomByte() {
  byte finalByte = 0;

  byte lastStack = leftStack ^ rightStack;

  for (int i = 0; i < 4; i++) {
    delayMicroseconds(waitTime);
    int leftBits = analogRead(ANALOG_RANDOM);

    delayMicroseconds(waitTime);
    int rightBits = analogRead(ANALOG_RANDOM);

    finalByte ^= rotate(leftBits, i);
    finalByte ^= rotate(rightBits, 7 - i);

    for (int j = 0; j < 8; j++) {
      byte leftBit = (leftBits >> j) & 1;
      byte rightBit = (rightBits >> j) & 1;

      if (leftBit != rightBit) {
        if (lastStack % 2 == 0) {
          pushLeftStack(leftBit);
        } else {
          pushRightStackRight(leftBit);
        }
      }
    }

  }
  lastByte ^= (lastByte >> 3) ^ (lastByte << 5) ^ (lastByte >> 4);
  lastByte ^= finalByte;

  return lastByte ^ leftStack ^ rightStack;
}
#endif

#if CRC8_EN
byte _table [] = {0x0, 0x7, 0xe, 0x9, 0x1c, 0x1b, 0x12, 0x15, 0x38, 0x3f, 0x36, 0x31, 0x24, 0x23, 0x2a, 0x2d, 0x70, 0x77, 0x7e, 0x79, 0x6c, 0x6b, 0x62, 0x65, 0x48, 0x4f, 0x46, 0x41, 0x54, 0x53, 0x5a, 0x5d, 0xe0, 0xe7, 0xee, 0xe9, 0xfc, 0xfb, 0xf2, 0xf5, 0xd8, 0xdf, 0xd6, 0xd1, 0xc4, 0xc3, 0xca, 0xcd, 0x90, 0x97, 0x9e, 0x99, 0x8c, 0x8b, 0x82, 0x85, 0xa8, 0xaf, 0xa6, 0xa1, 0xb4, 0xb3, 0xba, 0xbd, 0xc7, 0xc0, 0xc9, 0xce, 0xdb, 0xdc, 0xd5, 0xd2, 0xff, 0xf8, 0xf1, 0xf6, 0xe3, 0xe4, 0xed, 0xea, 0xb7, 0xb0, 0xb9, 0xbe, 0xab, 0xac, 0xa5, 0xa2, 0x8f, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9d, 0x9a, 0x27, 0x20, 0x29, 0x2e, 0x3b, 0x3c, 0x35, 0x32, 0x1f, 0x18, 0x11, 0x16, 0x3, 0x4, 0xd, 0xa, 0x57, 0x50, 0x59, 0x5e, 0x4b, 0x4c, 0x45, 0x42, 0x6f, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7d, 0x7a, 0x89, 0x8e, 0x87, 0x80, 0x95, 0x92, 0x9b, 0x9c, 0xb1, 0xb6, 0xbf, 0xb8, 0xad, 0xaa, 0xa3, 0xa4, 0xf9, 0xfe, 0xf7, 0xf0, 0xe5, 0xe2, 0xeb, 0xec, 0xc1, 0xc6, 0xcf, 0xc8, 0xdd, 0xda, 0xd3, 0xd4, 0x69, 0x6e, 0x67, 0x60, 0x75, 0x72, 0x7b, 0x7c, 0x51, 0x56, 0x5f, 0x58, 0x4d, 0x4a, 0x43, 0x44, 0x19, 0x1e, 0x17, 0x10, 0x5, 0x2, 0xb, 0xc, 0x21, 0x26, 0x2f, 0x28, 0x3d, 0x3a, 0x33, 0x34, 0x4e, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5c, 0x5b, 0x76, 0x71, 0x78, 0x7f, 0x6a, 0x6d, 0x64, 0x63, 0x3e, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2c, 0x2b, 0x6, 0x1, 0x8, 0xf, 0x1a, 0x1d, 0x14, 0x13, 0xae, 0xa9, 0xa0, 0xa7, 0xb2, 0xb5, 0xbc, 0xbb, 0x96, 0x91, 0x98, 0x9f, 0x8a, 0x8d, 0x84, 0x83, 0xde, 0xd9, 0xd0, 0xd7, 0xc2, 0xc5, 0xcc, 0xcb, 0xe6, 0xe1, 0xe8, 0xef, 0xfa, 0xfd, 0xf4, 0xf3};
uint8_t crc8(uint8_t *args, uint8_t len) {
  uint8_t _sum = 0;
  for (uint8_t i=0; i<len; i++) {
    _sum = _table[_sum ^ args[i]];
  }
  return _sum;
}
#endif

void led_on() {
  if (!LED_EN) return;
  if (led_brightness == 255) digitalWrite(LED_PIN, HIGH);
  else analogWrite(LED_PIN, led_brightness);
}

void led_off() {
  if (!LED_EN) return;
  digitalWrite(LED_PIN, LOW);
}

void Blink(uint8_t count, uint8_t pin = LED_BUILTIN, uint8_t dly = 50) {
  if (!LED_EN) return;
  uint8_t state = LOW;

  for (int x=0; x<(count << 1); ++x) {
    analogWrite(pin, state ^= led_brightness);
    delay(dly);
  }
}

static int readTemperature() {
   ADMUX = 0xC8;                          // activate interal temperature sensor, 
                                          // using 1.1V ref. voltage
   ADCSRA |= _BV(ADSC);                   // start the conversion
   while (bit_is_set(ADCSRA, ADSC));      // ADSC is cleared when the conversion 
                                          // finishes
                                          
   // combine bytes & correct for temperature offset (approximate)
   return ( (ADCL | (ADCH << 8)) - TEMPERATURE_OFFSET);  
}
