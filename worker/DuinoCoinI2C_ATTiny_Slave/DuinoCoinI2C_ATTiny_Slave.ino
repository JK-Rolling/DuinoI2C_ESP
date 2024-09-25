/*
  DoinoCoinI2C_ATTiny_Slave.ino
  adapted from Luiz H. Cassettari
  by JK Rolling
*/
//#pragma GCC optimize ("-Ofast")
#include <ArduinoUniqueID.h>  // https://github.com/ricaun/ArduinoUniqueID
#include <EEPROM.h>
#include <Wire.h>
#include <avr/wdt.h>
#include "duco_hash.h"

/****************** USER MODIFICATION START ****************/
#define ADDRESS_I2C                 1             // manual I2C address assignment
#define CRC8_EN                     true
#define WDT_EN                      true
#define SENSOR_EN                   false         // use ATTiny85 internal temperature sensor
#define LED_EN                      true          // brightness controllable on pin 1
/****************** USER MODIFICATION END ******************/
/*---------------------------------------------------------*/
/****************** FINE TUNING START **********************/
#define WORKER_NAME                 "t"
#define WIRE_MAX                    32
#define WIRE_CLOCK                  100000
#define TEMPERATURE_OFFSET          287           // calibrate ADC here
#define TEMPERATURE_COEFF           1             // calibrate ADC further
#define LED_PIN                     1
#define LED_BRIGHTNESS              1             // 1-255
/****************** FINE TUNING END ************************/
//#define EEPROM_ADDRESS              0
#if defined(ARDUINO_AVR_UNO) | defined(ARDUINO_AVR_PRO)
#define SERIAL_LOGGER Serial
#endif

// ATtiny85 - http://drazzy.com/package_drazzy.com_index.json
// SCL - PB2 - 2
// SDA - PB0 - 0

#ifdef SERIAL_LOGGER
#define SerialBegin()              SERIAL_LOGGER.begin(115200);
#define SerialPrint(x)             SERIAL_LOGGER.print(x);
#define SerialPrintln(x)           SERIAL_LOGGER.println(x);
#else
#define SerialBegin()
#define SerialPrint(x)
#define SerialPrintln(x)
#endif

#if LED_EN
#define LedBegin()                DDRB |= (1 << LED_PIN);
#if LED_BRIGHTNESS == 255
#define LedHigh()                 PORTB |= (1 << LED_PIN);
#define LedLow()                  PORTB &= ~(1 << LED_PIN);
#else
#define LedHigh()                 analogWrite(LED_PIN, LED_BRIGHTNESS);
#define LedLow()                  analogWrite(LED_PIN, 0);
#endif
#define LedBlink()                LedHigh(); delay(100); LedLow(); delay(100);
#else
#define LedBegin()
#define LedHigh()
#define LedLow()
#define LedBlink()
#endif

#if CRC8_EN
#define BUFFER_MAX 89
#else
#define BUFFER_MAX 86
#endif
#define HASH_BUFFER_SIZE 20
#define CHAR_END '\n'
#define CHAR_DOT ','

static const char DUCOID[] PROGMEM = "DUCOID";
//static const char ZEROS[] PROGMEM = "000";
static const char WK_NAME[] PROGMEM = WORKER_NAME;
static const char UNKN[] PROGMEM = "un";
static const char ONE[] PROGMEM = "1";
static const char ZERO[] PROGMEM = "0";

static byte address;
static char buffer[BUFFER_MAX];
static uint8_t buffer_position;
static uint8_t buffer_length;
static bool working;
static bool jobdone;

void(* resetFunc) (void) = 0;//declare reset function at address 0

// --------------------------------------------------------------------- //
// setup
// --------------------------------------------------------------------- //

void setup() {
  if (SENSOR_EN) {
    // Setup the Analog to Digital Converter -  one ADC conversion
    //   is read and discarded
    ADCSRA &= ~(_BV(ADATE) |_BV(ADIE)); // Clear auto trigger and interrupt enable
    ADCSRA |= _BV(ADEN);                // Enable AD and start conversion
    ADMUX = 0xF | _BV( REFS1 );         // ADC4 (Temp Sensor) and Ref voltage = 1.1V;
    delay(3);                           // Settling time min 1ms
    getADC();                           // discard first read
  }
  SerialBegin();
  if (WDT_EN) {
    wdt_disable();
    wdt_enable(WDTO_4S);
  }
  initialize_i2c();
  LedBegin();
  LedBlink();
  LedBlink();
  LedBlink();
}

// --------------------------------------------------------------------- //
// loop
// --------------------------------------------------------------------- //

void loop() {
  do_work();
  millis(); // ????? For some reason need this to work the i2c
#ifdef SERIAL_LOGGER
  if (SERIAL_LOGGER.available())
  {
#ifdef EEPROM_ADDRESS
    EEPROM.write(EEPROM_ADDRESS, SERIAL_LOGGER.parseInt());
#endif
    resetFunc();
  }
#endif
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
    LedHigh();
    SerialPrintln(buffer);

    if (buffer[0] == '&')
    {
      resetFunc();
    }

    if (buffer[0] == 'g') {
      // i2c_cmd
      //pos 0123[4]5678
      //    get,[c]rc8$
      //    get,[b]aton$
      //    get,[s]inglecore$
      //    get,[f]req$
      char f = buffer[4];
      switch (tolower(f)) {
        case 't': // temperature
          if (SENSOR_EN) ltoa(getTemperature(), buffer, 8);
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
          strcpy_P(buffer, WORKER_NAME);
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
      if (WDT_EN) wdt_reset();
      return;
    }
    do_job();
  }
  LedLow();
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
  
  uint16_t startTime = millis();
  if (job)
    job = work();
  uint16_t elapsedTime = millis() - startTime;
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
  
  if (WDT_EN) wdt_reset();
}

uint16_t work()
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

void HEX_TO_BYTE(char * address, char * hex, uint8_t len)
{
  for (uint8_t i = 0; i < len; i++) address[i] = TWO_HTOI(hex[2 * i], hex[2 * i + 1]);
}

// DUCO-S1A hasher
uint16_t work(char * lastblockhash, char * newblockhash, uint8_t difficulty)
{
  HEX_TO_BYTE(newblockhash, newblockhash, HASH_BUFFER_SIZE);
  static duco_hash_state_t hash;
  duco_hash_init(&hash, lastblockhash);
  char nonceStr[10 + 1];
  for (uint16_t nonce = 0; nonce < difficulty*100+1; nonce++) {
    ultoa(nonce, nonceStr, 10);
    uint8_t const * hash_bytes = duco_hash_try_nonce(&hash, nonceStr);
    if (memcmp(hash_bytes, newblockhash, HASH_BUFFER_SIZE) == 0) {
      return nonce;
    }
    if (WDT_EN) {
      if (runEvery(2000)) wdt_reset();
    }
  }

  return 0;
}

// --------------------------------------------------------------------- //
// i2c
// --------------------------------------------------------------------- //

void initialize_i2c(void) {
  address = ADDRESS_I2C;

#ifdef EEPROM_ADDRESS
  address = EEPROM.read(EEPROM_ADDRESS);
  if (address == 0 || address > 127) {
    address = ADDRESS_I2C;
    EEPROM.write(EEPROM_ADDRESS, address);
  }
#endif

  SerialPrint("Wire begin ");
  SerialPrintln(address);
  Wire.begin(address);
  Wire.onReceive(onReceiveJob);
  Wire.onRequest(onRequestResult);
}

void onReceiveJob(uint8_t howMany) {    
  if (howMany == 0) return;
  if (working) return;
  if (jobdone) return;
  
  char c = Wire.read();
  buffer[buffer_length++] = c;
  if (buffer_length == BUFFER_MAX) buffer_length--;
  if (c == CHAR_END || c == '$') {
    working = true;
  }
  // flush remaining data
  while (Wire.available()) {
    Wire.read();
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

#if CRC8_EN
static const byte _table[] PROGMEM = {0x0, 0x7, 0xe, 0x9, 0x1c, 0x1b, 0x12, 0x15, 0x38, 0x3f, 0x36, 0x31, 0x24, 0x23, 0x2a, 0x2d, 0x70, 0x77, 0x7e, 0x79, 0x6c, 0x6b, 0x62, 0x65, 0x48, 0x4f, 0x46, 0x41, 0x54, 0x53, 0x5a, 0x5d, 0xe0, 0xe7, 0xee, 0xe9, 0xfc, 0xfb, 0xf2, 0xf5, 0xd8, 0xdf, 0xd6, 0xd1, 0xc4, 0xc3, 0xca, 0xcd, 0x90, 0x97, 0x9e, 0x99, 0x8c, 0x8b, 0x82, 0x85, 0xa8, 0xaf, 0xa6, 0xa1, 0xb4, 0xb3, 0xba, 0xbd, 0xc7, 0xc0, 0xc9, 0xce, 0xdb, 0xdc, 0xd5, 0xd2, 0xff, 0xf8, 0xf1, 0xf6, 0xe3, 0xe4, 0xed, 0xea, 0xb7, 0xb0, 0xb9, 0xbe, 0xab, 0xac, 0xa5, 0xa2, 0x8f, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9d, 0x9a, 0x27, 0x20, 0x29, 0x2e, 0x3b, 0x3c, 0x35, 0x32, 0x1f, 0x18, 0x11, 0x16, 0x3, 0x4, 0xd, 0xa, 0x57, 0x50, 0x59, 0x5e, 0x4b, 0x4c, 0x45, 0x42, 0x6f, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7d, 0x7a, 0x89, 0x8e, 0x87, 0x80, 0x95, 0x92, 0x9b, 0x9c, 0xb1, 0xb6, 0xbf, 0xb8, 0xad, 0xaa, 0xa3, 0xa4, 0xf9, 0xfe, 0xf7, 0xf0, 0xe5, 0xe2, 0xeb, 0xec, 0xc1, 0xc6, 0xcf, 0xc8, 0xdd, 0xda, 0xd3, 0xd4, 0x69, 0x6e, 0x67, 0x60, 0x75, 0x72, 0x7b, 0x7c, 0x51, 0x56, 0x5f, 0x58, 0x4d, 0x4a, 0x43, 0x44, 0x19, 0x1e, 0x17, 0x10, 0x5, 0x2, 0xb, 0xc, 0x21, 0x26, 0x2f, 0x28, 0x3d, 0x3a, 0x33, 0x34, 0x4e, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5c, 0x5b, 0x76, 0x71, 0x78, 0x7f, 0x6a, 0x6d, 0x64, 0x63, 0x3e, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2c, 0x2b, 0x6, 0x1, 0x8, 0xf, 0x1a, 0x1d, 0x14, 0x13, 0xae, 0xa9, 0xa0, 0xa7, 0xb2, 0xb5, 0xbc, 0xbb, 0x96, 0x91, 0x98, 0x9f, 0x8a, 0x8d, 0x84, 0x83, 0xde, 0xd9, 0xd0, 0xd7, 0xc2, 0xc5, 0xcc, 0xcb, 0xe6, 0xe1, 0xe8, 0xef, 0xfa, 0xfd, 0xf4, 0xf3};
uint8_t crc8(uint8_t *args, uint8_t len) {
  uint8_t _sum = 0;
  for (uint8_t i=0; i<len; i++) {
    _sum = pgm_read_byte(&(_table[_sum ^ args[i]]));
  }
  return _sum;
}
#endif

// Calibration of the temperature sensor has to be changed for your own ATtiny85
// per tech note: http://www.atmel.com/Images/doc8108.pdf
uint16_t chipTemp(uint16_t raw) {
  return((raw - TEMPERATURE_OFFSET) / TEMPERATURE_COEFF);
}

// Common code for both sources of an ADC conversion
uint16_t getADC() {
  ADCSRA  |= _BV(ADSC);           // Start conversion
  while((ADCSRA & _BV(ADSC)));    // Wait until conversion is finished
  return ADC;                     // 10-bits
}

uint16_t getTemperature() {
  uint8_t vccIndex, vccGuess, vccLevel;
  uint16_t rawTemp=0, rawVcc;

  // Measure temperature
  ADCSRA |= _BV(ADEN);           // Enable AD and start conversion
  ADMUX = 0xF | _BV( REFS1 );    // ADC4 (Temp Sensor) and Ref voltage = 1.1V;
  delay(3);                      // Settling time min 1ms. give it 3

  // acumulate ADC samples
  for (uint8_t i=0; i<16; i++) {
    rawTemp += getADC();
  }
  rawTemp = rawTemp >> 4;       // take median value
  
  ADCSRA &= ~(_BV(ADEN));        // disable ADC

  rawVcc = getRawVcc();

  // probably using 3.3V Vcc if less than 4V
  if ((rawVcc * 10) < 40) { 
    vccGuess = 33; 
    vccLevel = 16; 
  } 
  // maybe 5V Vcc
  else { 
    vccGuess = 50; 
    vccLevel = 33; 
  } 
  //index 0..13 for vcc 1.7 ... 3.0
  //index 0..33 for vcc 1.7 ... 5.0
  vccIndex = min(max(17,(uint8_t)(rawVcc * 10)),vccGuess) - 17;

  // Temperature compensation using the chip voltage 
  // with 3.0 V VCC is 1 lower than measured with 1.7 V VCC 
  return chipTemp(rawTemp) + vccIndex / vccLevel;
}

uint16_t getRawVcc() {
  uint16_t rawVcc = 0;
  
  // Measure chip voltage (Vcc)
  ADCSRA |= _BV(ADEN);           // Enable ADC
  ADMUX  = 0x0c | _BV(REFS2);    // Use Vcc as voltage reference,
                                 // bandgap reference as ADC input
  delay(3);                      // Settling time min 1 ms. give it 3

  // accumulate ADC samples
  for (uint8_t i=0; i<16; i++) {
    rawVcc += getADC();
  }
  rawVcc = rawVcc >> 4;          // take median value
  
  ADCSRA &= ~(_BV(ADEN));        // disable ADC

  // single ended conversion formula. ADC = Vin * 1024 / Vref, where Vin = 1.1V
  // 1.1 * (1<<5) = 35.20. take 35. fp calculation is expensive
  // divide using right shift
  rawVcc = (1024 * 35 / rawVcc) >> 5;
  
  return rawVcc;
}
