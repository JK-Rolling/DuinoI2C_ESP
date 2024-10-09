#include <Wire.h>
// serial baudrate: 115200

// change to 'true' for ESP01
#define ESP01 false

#if ESP01
  #define SDA 0 // GPIO0 - A4
  #define SCL 2 // GPIO2 - A5
#else // other ESP8266
  #define SDA 4 // D2 - A4 - GPIO4
  #define SCL 5 // D1 - A5 - GPIO5
#endif
#define WIRE_CLOCK 100000

void setup() {
  Wire.begin(SDA, SCL);
  Wire.setClock(WIRE_CLOCK);
  Serial.begin(115200);
  Serial.println("\nI2C Scanner");
}
 
void loop() {
  byte error, address;
  int nDevices;
  Serial.println("Scanning...");
  nDevices = 0;
  for(address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address<16) {
        Serial.print("0");
      }
      Serial.println(address,HEX);
      nDevices++;
    }
    else if (error==4) {
      Serial.print("Unknow error at address 0x");
      if (address<16) {
        Serial.print("0");
      }
      Serial.println(address,HEX);
    }    
  }
  if (nDevices == 0) {
    Serial.println("No I2C devices found\n");
  }
  else {
    Serial.print("Found ");
    Serial.print(nDevices);
    Serial.println(" done\n");
  }
  delay(3000);          
}