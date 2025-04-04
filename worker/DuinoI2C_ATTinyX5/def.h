#ifndef __DEF__
#define __DEF__

#if CRC_EN
  #define BUF_SIZE 89       // "<last_hash>,<new_hash>,<diff>,<crc>" = 40+1+40+1+2+1+3 = 88 + '\0' = 89
#else
  #define BUF_SIZE 85       // "<last_hash>,<new_hash>,<diff>"       = 40+1+40+1+2     = 84 + '\0' = 85
#endif // CRC_EN

// character
#define CHAR_END '\n'
#define CHAR_DOT ','
#define CHAR_DSN '$'
#define CHAR_HSH '#'

// do..while(0) loop is one of the defensive coding technique. it gets optimized away anyway
#if LED_EN
  #if (LED_DIM)
    #if (LED_PIN != PIN_PB1)
    #error "PWM LED must be on PB1 for ATtinyX5"
    #endif
    #define _ledPwmInit()            do { \
                                      TCCR0A = (1 << WGM00) | (1 << WGM01) | (1 << COM0B1); \
                                      TCCR0B = (1 << CS01) | (1 << CS00); \
                                      OCR0B = 0; /* Start with LED off */ \
                                    } while(0)
    #define ledOn()                 do { _ledPwmEnable(); OCR0B = led_dim; } while(0)
    #define ledOff()                do { OCR0B = 0; _ledPwmDisable(); } while(0) /* clear duty before disable */
    #define _ledPwmDisable()        do { TCCR0A &= ~(1 << COM0B1); } while(0)
    #define _ledPwmEnable()         do { TCCR0A |= (1 << COM0B1); } while(0)
  #else // LED_DIM
    #define _ledPwmInit()           do { /* Nothing needed */ } while(0)
    #define ledOn()                 do { PORTB |=  (1 << LED_PIN); } while(0)
    #define ledOff()                do { PORTB &= ~(1 << LED_PIN); } while(0)
  #endif // LED_DIM
  #define ledSetup()                do { DDRB |= (1 << LED_PIN); _ledPwmInit(); } while (0) /* Set as output */
#else // LED_EN
  #define ledSetup()                do { /* Nothing needed */ } while(0)
  #define ledOn()                   do { /* Nothing needed */ } while(0)
  #define ledOff()                  do { /* Nothing needed */ } while(0)
#endif // LED_EN

#if WDT_EN
  #include <avr/wdt.h>
  #define wdtDisable()              do { wdt_disable(); } while(0)
  #define wdtReset()                do { wdt_reset(); } while(0)
  #define wdtEnable(x)              do { wdt_enable(x); } while(0)
  #define wdtBegin()                do { wdtDisable(); wdtEnable(WDTO_4S); } while(0)
#else // WDT_EN
  #define wdtDisable()              do { /* Nothing needed */ } while(0)
  #define wdtReset()                do { /* Nothing needed */ } while(0)
  #define wdtEnable(x)              do { /* Nothing needed */ } while(0)
  #define wdtBegin()                do { /* Nothing needed */ } while(0)
#endif // WDT_EN

#if defined(__AVR_ATtiny85__) || defined(__AVR_ATtiny45__)
  #include "src/TinyWireS-1.0.0/TinyWireS.h"
  #define BUS TinyWireS
  typedef uint8_t intWire;
  #define stopCheck()              do { TinyWireS_stop_check(); } while(0)
#else // non-attiny
  #include <Wire.h>
  #define BUS Wire
  typedef int intWire;
  #define stopCheck()               do { /* Nothing needed */ } while(0)
#endif // attiny

#if SERIAL_EN
  #if defined(__AVR_ATtiny85__) || defined(__AVR_ATtiny45__)
    // https://github.com/ArminJo/ATtinySerialOut
    // baudrate 115200 only works on 8MHz/16MHz CPU. 16.5MHz will not work.
    #define TX_PIN PIN_PB4 // PB3/PB4 as software serial out
    #include "ATtinySerialOut.hpp"
  #endif // attiny
  #define SERIAL_LOGGER              Serial
  #define serialBegin()              do { SERIAL_LOGGER.begin(115200); } while(0)
  #define serialPrint(x)             do { SERIAL_LOGGER.print(x); } while(0)  // each call cost 4 bytes
  #define serialPrintln(x)           do { SERIAL_LOGGER.println(x); } while(0)
#else // SERIAL_EN
  #define serialBegin()              do { /* Nothing needed */ } while(0)
  #define serialPrint(x)             do { /* Nothing needed */ } while(0)
  #define serialPrintln(x)           do { /* Nothing needed */ } while(0)
#endif // SERIAL_EN

#include <avr/boot.h>
#include "duco_hash.h"
#endif // __DEF__
