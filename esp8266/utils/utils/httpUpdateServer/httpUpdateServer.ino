/*
Author: JK Rolling
Description:
    This code is for user to compile for ESP-01S to enable firmware update via WiFi to workaround 1MB flash size limitation and DuinoI2C_ESP .bin file size.
    Once uploaded, LED should start blinking while attempt to establish connection to WiFi AP,
    When connected, LED should light up solidly.
    User will then proceed to upload the latest DuinoI2C_ESP .bin.gz
Instruction:
    1. Open the sketch from Arduino IDE. Choose Tools --> Board --> ESP8266 Board --> Generic ESP8266 Module
    2. Go to menu. Sketch --> Export compiled Binary. or keyboard shortcut Ctrl+Alt+S
    3. (Optional) Compress the exported binary using Gzip with compression ratio 9 for best result
    4. Visit the ESP-01S firmware update page. example: http://192.168.0.12/firmware
    5. Select the .bin or .bin.gz (preferred) file for upload
*/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>

#define LED_PIN 2
#define SERIAL_EN false
#define LED_EN true

// User to replace wifi credential below
const char* ssid = "MY_SSID";
const char* password = "MY_PASSWORD";

#if (SERIAL_EN)
#define SERIAL_LOGGER Serial
#define SERIALBEGIN SERIAL_LOGGER.begin
#define SERIALPRINT SERIAL_LOGGER.print
#define SERIALPRINTF SERIAL_LOGGER.printf
#define SERIALPRINTLN SERIAL_LOGGER.println
#else
#define SERIAL_LOGGER
#define SERIALBEGIN
#define SERIALPRINT
#define SERIALPRINTLN
#endif

ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

void setup() {
  delay(500);
  SERIALBEGIN(500000);
  #if (LED_EN)
    pinMode(LED_PIN, OUTPUT);
  #endif
  WiFi.mode(WIFI_STA);

  WiFi.begin(ssid, password); 
//  WiFi.begin(); // passwordless, but not reliable
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    delay(500);
    #if (LED_EN)
      digitalWrite(LED_PIN, !(digitalRead(LED_PIN)));
    #endif
    SERIALPRINT(".");
  }
  SERIALPRINTLN("");
  SERIALPRINTLN("WiFi connected");
  SERIALPRINTLN(WiFi.localIP());

  httpUpdater.setup(&httpServer, "/firmware");
  httpServer.begin();
  #if (LED_EN)
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(LED_PIN, LOW);
  }
  #endif
}

void loop() {
  httpServer.handleClient();
}
