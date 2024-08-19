# DuinoI2C_ESP

## Overview
Worker is only responsible to receive job from ESP, find the result, and return the result to ESP, all via I2C. User will need to compile the source code in Arduino IDE and upload the firmware to the AVR.

## Supported Devices
|| UNO | NANO | Pro Mini | Atmel ATTiny85 |
| :-: | :-: | :-: | :-: | :-: |
| DuinoCoinI2C_Tiny_Slave | ✅ | ✅ | ✅ | :x: |
| DuinoCoinI2C_ATTiny_Slave | :x: | :x: | :x: | ✅ |

## Library Dependency
* [ArduinoUniqueID](https://github.com/ricaun/ArduinoUniqueID) (Handle the chip ID)

## Arduino IDE
Recommended version: 1.8.19

## Arduino Uno/Nano/Pro - Tiny_Slave
Arduino ATmega168/328 shall use `DuinoCoinI2C_Tiny_Slave` sketch. Logic-Level-Converter (LLC) is required if Arduino is operating at 5V and master at 3.3V.

Specific for Arduino Nano or Nano cloned, it is strongly recommended to update the bootloader to Optiboot to leverage watchdog timer (WDT) feature. WDT is important to make sure the AVR is restarted automatically whenever it is hanging for whatever reason over 4 seconds.

Nano old bootloader will still run fine without Optiboot but the setting `WDT_EN` must be changed to `false` or the Nano will hang once timed out.

### I2C Address
Increment the `DEV_INDEX` per device and upload.

Example:
||`DEV_INDEX`|I2C address|
|:-:|:-:|:-:|
|AVR0|0|1|
|AVR1|1|2|
|AVRx|X|X+1|

```C
/****************** USER MODIFICATION START ****************/
#define DEV_INDEX                   0
#define I2CS_START_ADDRESS          1
#define I2CS_FIND_ADDR              false
#define WDT_EN                      true        // recommended 'true', but do not turn on if using old bootloader
#define CRC8_EN                     true
#define LED_EN                      true
#define SENSOR_EN                   false
/****************** USER MODIFICATION END ******************/
```

## Atmel ATTiny85 - ATTiny_Slave
Use `DuinoCoinI2C_ATTiny_Slave` for ATtiny85. LLC is required if worker and host is operating at different voltage. 4k7 pullup resistors for `SDA/SCL` pins are strongly recommended. The TWI/I2C/IIC seems to work well with SCL 100KHz `WIRE_CLOCK 100000`.

Add `http://drazzy.com/package_drazzy.com_index.json` to `Additional Board Manager URLs` in Arduino IDE, then go to board manager and search for `attiny` and install ATTinyCore from Spence Konde.

ATTiny85 default system clock is 1MHz. This needs to be changed to get good hashrate. This sketch is applicable to Adafruit Trinket ATtiny85 too but the bootloader will be removed during fuse update to regain full 8KB flash capacity.

You may use dedicated ATTiny programmer or any Uno/Nano to set the fuse via `Tools --> Burn Bootloader`. See table below on setting that worked for me on ATtiny85. Make sure the `Tools --> Programmer --> Arduino as ISP` is selected. Finally upload sketch using `Sketch --> Upload Using Programmer`.

|Attribute|Value|
|:-|:-|
|Board|ATtiny25/45/85 (No Bootloader)|
|Chip|ATtiny85|
|Clock Source|16.5 MHz (PLL,tweaked)|
|Timer 1 Clock|CPU|
|LTO|Enabled|
|millis()/micros()|Enabled|
|Save EEPROM|EEPROM retained|
|B.O.D Level|Disabled|

### I2C Address
Increment the `ADDRESS_I2C` per device and upload.

Example:
||`ADDRESS_I2C`|I2C address|
|:-:|:-:|:-:|
|AVR0|1|1|
|AVR1|2|2|
|AVRx|X+1|X+1|
```C
/****************** USER MODIFICATION START ****************/
#define ADDRESS_I2C                 1             // manual I2C address assignment
#define CRC8_EN                     true
#define WDT_EN                      true
#define SENSOR_EN                   false         // use ATTiny85 internal temperature sensor
#define LED_EN                      true          // brightness controllable on pin 1
/****************** USER MODIFICATION END ******************/
```

## Benchmarks of tested devices

  | Device                                                    | Average hashrate<br>(all threads) | Mining<br>threads |
  |-----------------------------------------------------------|-----------------------------------|-------------------|
  | Arduino Pro Mini, Uno, Nano etc.<br>(Atmega 328p/pb/16u2) | 340 H/s                           | 1                 |
  | ATtiny85                                                  | 340 H/s                           | 1                 |
