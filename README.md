# DuinoI2C_ESP
This project is designed to mine Duino-Coin using an ESP8622 as a I2C master and up to ~~10~~ **20** AVR workers.

## Feature Highlights
1. **High share rate**. The DUCO earning highly dependent on number of share found and submitted. Higher share rate means higher earning!
2. **More workers**. Up to 20 workers* without significant impact on the share rate!
3. **Portability**. DuinoI2C_ESP allow up to 2 WiFi credential to be stored. So no more worries if you're moving the mining rig between 2 places.
4. **View live stats on OLED**. DuinoI2C_ESP provides mining statistic summary and detailed worker status, right from the OLED.
5. **View live stats on web serial**. Detailed info like share acceptance, share rate, ping time will scroll up non-stop during normal operation.
6. **Firmware update via WiFi**. After loading DuinoI2C_ESP via cable the first time, subsequent update can be done over WiFi. Bye cable!
7. **Breathing LED**. Onboard LED will indicate if the ESP is running happily.
8. **Dark ambient friendly**. Control the brightness of the OLED, turn ON/OFF the OLED and onboard LED at will from your phone/computer.
9. **ESP Restart**. Any time when ESP act abnormally, restart it with a click of a button or from your phone/computer.
10. **CRC8 Integrity**. I2C communication between ESP and AVR is CRC8 protected to ensure data is corruption free

*\*Unlocked*

## Mining Rig Setup
![](resource/DuinoI2C_ESP-rig.gif)

*GIF edited to 1 second/screen, no speed up*

### Connection Pinouts
|| ESP8266 | ESP01 | OLED | Logic Level Converter | Arduino | ATTiny85 |
|:-:| :----: | :----: | :--: | :-----: | :-----: | :-----: |
||3.3V | 3.3V | VCC | <---> | 5V | 5V |
||GND | GND | GND | <---> | GND | GND |
|`SCL`|D1 (GPIO5) | GPIO2 | SCL | <---> | A5 | PB2 |
|`SDA`|D2 (GPIO4) | GPIO0 | SDA | <---> | A4 | PB0 |

## ESP8266 Setup
See [esp8266](https://github.com/JK-Rolling/DuinoI2C_ESP/tree/main/esp8266) section of this repository.

## AVR Setup
See [worker](https://github.com/JK-Rolling/DuinoI2C_ESP/tree/main/worker) section of this repository.

## Contact
Official Discord server: [discord.gg/kvBkccy](https://discord.com/invite/kvBkccy) look for `JK Rolling` `jpx13` `Dark_Hunter`
