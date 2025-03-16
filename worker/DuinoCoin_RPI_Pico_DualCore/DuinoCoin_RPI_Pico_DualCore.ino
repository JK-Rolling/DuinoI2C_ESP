/*
 * DuinoCoin_RPI_Pico_DualCore.ino
 * created by JK Rolling
 * 27 06 2022
 * 
 * Dual core and dual I2C (400kHz/1MHz)
 * DuinoCoin worker
 * 
 * See kdb.ino on Arduino IDE settings to compile for RP2040
 * 
 * with inspiration from 
 * * Revox (Duino-Coin founder)
 * * Ricaun (I2C Nano sketch)
 * 
Sketch uses 74300 bytes (3%) of program storage space. Maximum is 2093056 bytes.
Global variables use 12660 bytes (4%) of dynamic memory, leaving 249484 bytes for local variables. Maximum is 262144 bytes.
 */

#pragma GCC optimize ("-Ofast")

#include <Wire.h>
#include "pico/mutex.h"
extern "C" {
  #include <hardware/watchdog.h>
};

/****************** USER MODIFICATION START ****************/
#define DEV_INDEX                   0
#define I2CS_START_ADDRESS          8
#define WIRE_CLOCK                  400000              // >>> see kdb before changing this I2C clock frequency <<<
#define I2C0_SDA                    20
#define I2C0_SCL                    21
#define I2C1_SDA                    26
#define I2C1_SCL                    27
#define CRC8_EN                     true
#define WDT_EN                      true
#define CORE_BATON_EN               false
#define LED_EN                      true
#define SENSOR_EN                   true
#define SINGLE_CORE_ONLY            false               // >>> see kdb before setting it to true <<<
#define WORKER_NAME                 "rp2040"
#define NEOPIXEL_EN                 false               // >>> see kdb before setting it to true <<<
/****************** USER MODIFICATION END ******************/
/*---------------------------------------------------------*/
/****************** FINE TUNING START **********************/
#if NEOPIXEL_EN
  #define LED_PIN                     16  // RP2040_ZERO:16, Maker Pi Pico:28
#else
  #define LED_PIN                     LED_BUILTIN
#endif
#define LED_BRIGHTNESS              255                 // 1-255
#define BLINK_SHARE_FOUND           1
#define BLINK_SETUP_COMPLETE        2
#define BLINK_BAD_CRC               3
#define WDT_PERIOD                  8300
// USB-Serial --> Serial
// UART0-Serial --> Serial1 (preferred, connect FTDI RX pin to Pico GP0)
#define SERIAL_LOGGER               Serial1
#define BAUDRATE                    115200
#define I2C0                        Wire
#define I2C1                        Wire1
#define DIFF_MAX                    1000
#define MCORE_WDT_THRESHOLD         20
/****************** FINE TUNING END ************************/

#ifdef SERIAL_LOGGER
#define SerialBegin()               SERIAL_LOGGER.begin(BAUDRATE);
#define SerialPrint(x)              SERIAL_LOGGER.print(x);
#define SerialPrintln(x)            SERIAL_LOGGER.println(x);
#else
#define SerialBegin()
#define SerialPrint(x)
#define SerialPrintln(x)
#endif

#if SINGLE_CORE_ONLY
#define DISABLE_2ND_CORE
#endif

// prototype
void Blink(uint8_t count, uint8_t pin);
void BlinkFade(uint8_t led_brightness);
bool repeating_timer_callback(struct repeating_timer *t);

static String DUCOID;
static mutex_t serial_mutex;
static mutex_t led_fade_mutex;
static bool core_baton;
static bool wdt_pet = true;
static uint16_t wdt_period_half = WDT_PERIOD/2;
// 40+40+20+3 is the maximum size of a job
const uint16_t job_maxsize = 104;
byte i2c1_addr=0;
bool core0_started = false, core1_started = false;
uint32_t core0_shares = 0, core0_shares_ss = 0, core0_shares_local = 0;
uint32_t core1_shares = 0, core1_shares_ss = 0, core1_shares_local = 0;
struct repeating_timer timer;
uint8_t led_brightness = 255;

// protect scarce resource
void printMsg(String msg) {
  uint32_t owner;
  if (!mutex_try_enter(&serial_mutex, &owner)) {
    if (owner == get_core_num()) return;
    mutex_enter_blocking(&serial_mutex);
  }
  SerialPrint(msg);
  mutex_exit(&serial_mutex);
}

void printMsgln(String msg) {
  printMsg(msg+"\n");
}

#include "core.h"
// initialize static members
core* core::instance0 = nullptr;
core* core::instance1 = nullptr;

// instantiate slaves
core bus0(&I2C0, 2 * DEV_INDEX + I2CS_START_ADDRESS,     I2C0_SDA, I2C0_SCL);
core bus1(&I2C1, 2 * DEV_INDEX + I2CS_START_ADDRESS + 1, I2C1_SDA, I2C1_SCL);

// Core0
void setup() {
  bool print_on_por = true;
  SerialBegin();

  // initialize mutex
  mutex_init(&serial_mutex);
  mutex_init(&led_fade_mutex);
  
  // core status indicator
  if (LED_EN) {
    pinMode(LED_PIN, OUTPUT);
    add_repeating_timer_ms(10, repeating_timer_callback, NULL, &timer);
  }

  // initialize pico internal temperature sensor
  if (SENSOR_EN) {
    enable_internal_temperature_sensor();
  }

  // initialize watchdog
  if (WDT_EN) {
    if (watchdog_caused_reboot()) {
      printMsgln(F("\nRebooted by Watchdog!"));
      Blink(BLINK_SETUP_COMPLETE, LED_PIN);
      print_on_por = false;
    }
    // Enable the watchdog, requiring the watchdog to be updated every 8000ms or the chip will reboot
    // Maximum of 0x7fffff, which is approximately 8.3 seconds
    // second arg is pause on debug which means the watchdog will pause when stepping through code
    watchdog_enable(WDT_PERIOD, 1);
  }
  
  // let core0 run first
  core_baton = true;
  
  DUCOID = get_DUCOID();
  bus0.bus_setup();
  Blink(BLINK_SETUP_COMPLETE, LED_PIN);
  if (print_on_por) {
    printMsgln("DUCOID: " + DUCOID);
    printMsgln("I2CS_START_ADDRESS: "+String(I2CS_START_ADDRESS));
    printMsgln("DEV_INDEX: "+String(DEV_INDEX));
    printMsgln("WIRE_CLOCK: "+String(WIRE_CLOCK));
    printMsgln("I2C0_SDA: "+String(I2C0_SDA));
    printMsgln("I2C0_SCL: "+String(I2C0_SCL));
    printMsgln("I2C1_SDA: "+String(I2C1_SDA));
    printMsgln("I2C1_SCL: "+String(I2C1_SCL));
    printMsgln("CRC8_EN: "+String(CRC8_EN));
    printMsgln("WDT_EN: "+String(WDT_EN));
    printMsgln("CORE_BATON_EN: "+String(CORE_BATON_EN));
    printMsgln("LED_EN: "+String(LED_EN));
    printMsgln("SENSOR_EN: "+String(SENSOR_EN));
    printMsgln("SINGLE_CORE_ONLY: "+String(SINGLE_CORE_ONLY));
  }
  printMsgln("core" + String(get_core_num()) + ": startup done!");
}

void loop() {
  if (core_baton || !CORE_BATON_EN) {
    if (bus0.coreLoop()) {
      core0_started = true;
      printMsg("core" + String(get_core_num()) + ": ");
      printMsgln(bus0.getResponse());
      BlinkFade(led_brightness);
      if (WDT_EN && wdt_pet) {
        watchdog_update();
      }
      
      if (core0_started && core1_started && !SINGLE_CORE_ONLY) {
        core0_shares++;
        if (core1_shares != core1_shares_ss) {
          core1_shares_ss = core1_shares;
          core1_shares_local = 0;
        }
        else {
          if ((MCORE_WDT_THRESHOLD - core1_shares_local) < MCORE_WDT_THRESHOLD)
            printMsgln("core" + String(get_core_num()) + ": core1 " + String(MCORE_WDT_THRESHOLD - core1_shares_local) + " remaining count to WDT pet disable");
          if (core1_shares_local >= MCORE_WDT_THRESHOLD) {
            printMsgln("core" + String(get_core_num()) + ": Detected core1 hung. Disable WDT pet");
            wdt_pet = false;
          }
          if ((MCORE_WDT_THRESHOLD - core1_shares_local) != 0) {
            core1_shares_local++;
          }
        }
      }
    }
    if (!SINGLE_CORE_ONLY)
      core_baton = false;
  }
}

#ifndef DISABLE_2ND_CORE
// Core1
void setup1() {
  sleep_ms(100);
  bus1.bus_setup();
  Blink(BLINK_SETUP_COMPLETE, LED_PIN);
  printMsgln("core" + String(get_core_num()) + ": startup done!");
}

void loop1() {
  if (!core_baton || !CORE_BATON_EN) {
    if (bus1.coreLoop()) {
      core1_started = true;
      printMsg("core" + String(get_core_num()) + ": ");
      printMsgln(bus1.getResponse());
      BlinkFade(led_brightness);
      if (WDT_EN && wdt_pet) {
        watchdog_update();
      }

      if (core0_started && core1_started && !SINGLE_CORE_ONLY) {
        core1_shares++;
        if (core0_shares != core0_shares_ss) {
          core0_shares_ss = core0_shares;
          core0_shares_local = 0;
        }
        else {
          if ((MCORE_WDT_THRESHOLD - core0_shares_local) < MCORE_WDT_THRESHOLD)
            printMsgln("core1: core0 " + String(MCORE_WDT_THRESHOLD - core0_shares_local) + " remaining count to WDT pet disable");
          if (core0_shares_local >= MCORE_WDT_THRESHOLD) {
            printMsgln("core" + String(get_core_num()) + ": Detected core0 hung. Disable WDT pet");
            wdt_pet = false;
          }
          if ((MCORE_WDT_THRESHOLD - core0_shares_local) != 0) {
            core0_shares_local++;
          }
        }
      }
    }
    core_baton = true;
  }
}
#endif
