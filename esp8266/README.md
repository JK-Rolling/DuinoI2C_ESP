# DuinoI2C_ESP

## Overview
DuinoI2C_ESP is a project designed to run in ESP8266. It'll act as a host to get jobs from Duino-Coin server, then distribute jobs to AVR workers via I2C. DuinoI2C_ESP provides a pre-compiled `.bin` file that you can easily download and upload to your ESP8266.

## Supported Devices
This project currently supports the following ESP8266 devices:
- Adafruit ESP8266
- ESP-01S (min. 1MB)
- Wemos D1 Mini

*Note: Additional devices may be added upon request. Limited to ESP8266 listed in Arduino IDE board manager*

## How to Download and Upload the `.bin` File

### Downloading the `.bin` File
1. Navigate to the [esp8266](https://github.com/JK-Rolling/DuinoI2C_ESP/tree/main/esp8266) section of this repository.
2. Download the latest `.bin` file to your local machine.

### Uploading the `.bin` File to ESP8266
There are 2 ways to upload the `.bin`. Web Browser is the easier way.

#### > Web Browser
1. Visit https://esp.huhn.me/
2. Click `CONNECT` button
3. Select the ESP8266 and click `Connect`
4. On first row, click `SELECT`, then browse to the .bin file downloaded
5. Click `ERASE`
6. Click `PROGRAM`

[espwebtool help](https://blog.spacehuhn.com/espwebtool)

#### > esptool
##### >> Linux
1. (First Time) `pip install esptool`
2. `esptool.py --chip esp8266 --port /dev/ttyUSB0 erase_flash`. Replace device path to your actual one. Note that erase flash will wipe out all existing data including WiFi credential. Skip this step if you want to keep them.
3. `esptool.py --chip esp8266 --port /dev/ttyUSB0 --baud 115200 write_flash 0x00000 DuinoI2C_ESP.bin`. Replace device path and .bin path to your actual one

##### >> Windows
1. (First Time) Download esptool from [esp_tool](https://github.com/espressif/esptool/releases). Example: esptool-v4.7.0-win64.zip
2. (First Time) Extract the .zip then look for esptool or esptool.exe in Windows Explorer
3. Open up a command prompt. You may use `Shift + Right click` in Windows Explorer to pick `Open in Terminal` from menu
4. `esptool.exe --chip esp8266 --port COM4 erase_flash`. Replace COM port to your actual one
5. `esptool.exe --chip esp8266 --port COM4 --baud 115200 write_flash 0x00000 DuinoI2C_ESP.bin`. Replace COM port and .bin path to your actual one

## Configure DuinoI2C_ESP
After flashing and powering on your ESP8266 for the first time, it will create a soft access point and launch a WiFi Manager web interface, allowing the user to configure it for proper operation.

### Connecting to the WiFi Manager
1. Connect your computer or phone to the WiFi network hosted by the ESP8266. You may select the WiFi network manually or use QR scanner if OLED is connected to the ESP8266.
2. Open a web browser and navigate to http://192.168.4.1.
3. The web interface will allow you to configure your WiFi credentials, Duino username and mining key, and optionally the firmware OTA credential.
4. Click on Save button to store the inputs in ESP8266. The ESP should restart automatically and start working!

## Control and Monitor DuinoI2C_ESP
TODO
