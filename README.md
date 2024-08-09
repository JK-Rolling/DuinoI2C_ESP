# DuinoI2C_ESP
This project is designed to mine Duino-Coin using an ESP8622 as a I2C master and up to 10 AVR as worker.

## Feature Highlights
1. **High share rate**. The DUCO earning highly dependent on number of share found and submitted. Higher share rate means higher earning!
2. **Portability**. DuinoI2C_ESP allow up to 2 WiFi credential to be stored. So no more worries if you're moving the mining rig between 2 places.
3. **View live stats on OLED**. DuinoI2C_ESP provides mining statistic summary and detailed worker status, right from the OLED
4. **View live stats on web serial**. Detailed info like share acceptance, ping time will scroll up non-stop during normal operation
5. **Firmware update via webserver**. After loading DuinoI2C_ESP via cable the first time, subsequent update can be done over WiFi
6. **Breathing LED**. Onboard LED will indicate if the ESP is running properly and not hanging
7. **Dark ambient friendly**. Control the brightness of the OLED, turn ON/OFF the OLED and onboard LED at will
8. **ESP Restart**. Any time when ESP act abnormally, restart it with a click of a button

## Mining Rig Setup
### Connection Pinouts
|| ESP8266 | ESP01 | Logic Level Converter | Arduino | ATTiny85 |
|:-:| :----: | :----: | :-----: | :-----: | :-----: |
||3.3V | 3.3V | <---> | 5V | 5V |
||GND | GND | <---> | GND | GND |
|`SCL`|D1 (GPIO5) | GPIO2 | <---> | A5 | PB2 |
|`SDA`|D2 (GPIO4) | GPIO0 | <---> | A4 | PB0 |

## ESP8266 Setup
See [esp8266](https://github.com/JK-Rolling/DuinoI2C_ESP/tree/main/esp8266) section of this repository.

## AVR Setup
See [worker](https://github.com/JK-Rolling/DuinoI2C_ESP/tree/main/worker) section of this repository.

## 
