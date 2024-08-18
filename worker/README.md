# DuinoI2C_ESP

## Overview
Worker is only responsible to receive job from ESP, find the result, and return the result to ESP, all via I2C. User will need to compile the source code in Arduino IDE and upload the firmware to the AVR.

## Supported Devices
|| Arduino UNO | Arduino NANO | Atmel ATTiny85 |
| :-: | :-: | :-: | :-: |
| DuinoCoinI2C_Tiny_Slave | ✅ | ✅ | :x: |
| DuinoCoinI2C_ATTiny_Slave | :x: | :x: | ✅ |

## Library Dependency
* [ArduinoUniqueID](https://github.com/ricaun/ArduinoUniqueID) (Handle the chip ID)

## Arduino IDE
recommended version: 1.8.19

## I2C Address
I2C address need to be manually assigned per device. Clashing address may produce unexpected behavior.

### Tiny_Slave
Increment the `DEV_INDEX` per device and upload.

Example:
||`DEV_INDEX`|I2C address|
|:-:|:-:|:-:|
|AVR0|0|1|
|AVR1|1|2|
|AVRx|X|3|

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

### ATTiny_Slave
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
