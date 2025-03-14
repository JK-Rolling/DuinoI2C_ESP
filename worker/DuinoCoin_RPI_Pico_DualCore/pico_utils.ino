/*
 * pico_utils.ino
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
 */

 #pragma GCC optimize ("-Ofast")

 #include "pico/unique_id.h"
 #include "hardware/structs/rosc.h"
 #include "hardware/adc.h"
 
 #ifndef LED_EN
 #define LED_EN false
 #endif
 
 
 
 #if RP2040_ZERO
   #include <NeoPixelConnect.h>
   NeoPixelConnect p(16, 1, pio0, 0);
 #endif
 
 // https://stackoverflow.com/questions/51731313/cross-platform-crc8-function-c-and-python-parity-check
 //uint8_t crc8( uint8_t *addr, uint8_t len) {
 //      uint8_t crc=0;
 //      for (uint8_t i=0; i<len;i++) {
 //         uint8_t inbyte = addr[i];
 //         for (uint8_t j=0;j<8;j++) {
 //             uint8_t mix = (crc ^ inbyte) & 0x01;
 //             crc >>= 1;
 //             if (mix) 
 //                crc ^= 0x8C;
 //         inbyte >>= 1;
 //      }
 //    }
 //   return crc;
 //}
 
 byte _table [] = {0x0, 0x7, 0xe, 0x9, 0x1c, 0x1b, 0x12, 0x15, 0x38, 0x3f, 0x36, 0x31, 0x24, 0x23, 0x2a, 0x2d, 0x70, 0x77, 0x7e, 0x79, 0x6c, 0x6b, 0x62, 0x65, 0x48, 0x4f, 0x46, 0x41, 0x54, 0x53, 0x5a, 0x5d, 0xe0, 0xe7, 0xee, 0xe9, 0xfc, 0xfb, 0xf2, 0xf5, 0xd8, 0xdf, 0xd6, 0xd1, 0xc4, 0xc3, 0xca, 0xcd, 0x90, 0x97, 0x9e, 0x99, 0x8c, 0x8b, 0x82, 0x85, 0xa8, 0xaf, 0xa6, 0xa1, 0xb4, 0xb3, 0xba, 0xbd, 0xc7, 0xc0, 0xc9, 0xce, 0xdb, 0xdc, 0xd5, 0xd2, 0xff, 0xf8, 0xf1, 0xf6, 0xe3, 0xe4, 0xed, 0xea, 0xb7, 0xb0, 0xb9, 0xbe, 0xab, 0xac, 0xa5, 0xa2, 0x8f, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9d, 0x9a, 0x27, 0x20, 0x29, 0x2e, 0x3b, 0x3c, 0x35, 0x32, 0x1f, 0x18, 0x11, 0x16, 0x3, 0x4, 0xd, 0xa, 0x57, 0x50, 0x59, 0x5e, 0x4b, 0x4c, 0x45, 0x42, 0x6f, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7d, 0x7a, 0x89, 0x8e, 0x87, 0x80, 0x95, 0x92, 0x9b, 0x9c, 0xb1, 0xb6, 0xbf, 0xb8, 0xad, 0xaa, 0xa3, 0xa4, 0xf9, 0xfe, 0xf7, 0xf0, 0xe5, 0xe2, 0xeb, 0xec, 0xc1, 0xc6, 0xcf, 0xc8, 0xdd, 0xda, 0xd3, 0xd4, 0x69, 0x6e, 0x67, 0x60, 0x75, 0x72, 0x7b, 0x7c, 0x51, 0x56, 0x5f, 0x58, 0x4d, 0x4a, 0x43, 0x44, 0x19, 0x1e, 0x17, 0x10, 0x5, 0x2, 0xb, 0xc, 0x21, 0x26, 0x2f, 0x28, 0x3d, 0x3a, 0x33, 0x34, 0x4e, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5c, 0x5b, 0x76, 0x71, 0x78, 0x7f, 0x6a, 0x6d, 0x64, 0x63, 0x3e, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2c, 0x2b, 0x6, 0x1, 0x8, 0xf, 0x1a, 0x1d, 0x14, 0x13, 0xae, 0xa9, 0xa0, 0xa7, 0xb2, 0xb5, 0xbc, 0xbb, 0x96, 0x91, 0x98, 0x9f, 0x8a, 0x8d, 0x84, 0x83, 0xde, 0xd9, 0xd0, 0xd7, 0xc2, 0xc5, 0xcc, 0xcb, 0xe6, 0xe1, 0xe8, 0xef, 0xfa, 0xfd, 0xf4, 0xf3};
 uint8_t crc8(uint8_t *args, uint8_t len) {
   uint8_t _sum = 0;
   for (uint8_t i=0; i<len; i++) {
     _sum = _table[_sum ^ args[i]];
   }
   return _sum;
 }
 
 uint8_t calc_crc8( String msg ) {
     int msg_len = msg.length() + 1;
     char char_array[msg_len];
     msg.toCharArray(char_array, msg_len);
     return crc8((uint8_t *)char_array,msg.length());
 }
 
 String get_DUCOID() {
   int len = 2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1;
   uint8_t buff[len] = "";
   pico_get_unique_board_id_string((char *)buff, len);
   String uniqueID = String ((char *)buff, strlen((char *)buff));
   return "DUCOID"+uniqueID;
 }
 
 void enable_internal_temperature_sensor() {
   adc_init();
   adc_set_temp_sensor_enabled(true);
   adc_select_input(0x4);
 }
 
 double read_temperature() {
   uint16_t adcValue = adc_read();
   double temperature;
   temperature = 3.3f / 0x1000;
   temperature *= adcValue;
   // celcius degree
   temperature = 27.0 - ((temperature - 0.706)/ 0.001721);
   // fahrenheit degree
   // temperature = temperature * 9 / 5 + 32;
   return temperature;
 }
 
 double read_humidity() {
   // placeholder for future external sensor
   return 0.0;
 }
 
 /* Von Neumann extractor: 
 From the input stream, this extractor took bits, 
 two at a time (first and second, then third and fourth, and so on). 
 If the two bits matched, no output was generated. 
 If the bits differed, the value of the first bit was output. 
 */
 uint32_t rnd_whitened(void){
     uint32_t k, random = 0;
     uint32_t random_bit1, random_bit2;
     volatile uint32_t *rnd_reg=(uint32_t *)(ROSC_BASE + ROSC_RANDOMBIT_OFFSET);
     
     for (k = 0; k < 32; k++) {
         while(1) {
             random_bit1=0x00000001 & (*rnd_reg);
             random_bit2=0x00000001 & (*rnd_reg);
             if (random_bit1 != random_bit2) break;
         }
         random = random + random_bit1;
         random = random << 1;    
     }
     return random;
 }
 
 struct LEDState {
   float fade;
   uint8_t green, red, blue;
 };
 
 volatile LEDState ledState[2] = {  
   {0, 255, 0, 0},   // Core 0 (green) GRB
   {0, 0, 0, 255}    // Core 1 (blue)  GRB
 };
 
 
 void Blink(uint8_t count, uint8_t pin = LED_PIN) {
   if (!LED_EN) return;
   uint8_t core = get_core_num();
   uint8_t state = LOW;
 
   for (int x = 0; x < (count << 1); ++x) {
     #if RP2040_ZERO
       sleep_ms(50);
       p.neoPixelFill(ledState[core].green, ledState[core].red, ledState[core].blue, true);
       sleep_ms(50);
       p.neoPixelClear(true);
     #else
       analogWrite(pin, state ^= LED_BRIGHTNESS);
       sleep_ms(50);
     #endif
   }
 }
 
 bool repeating_timer_callback(struct repeating_timer *t) {
   uint8_t core = get_core_num();
 
   if (ledState[core].fade > 0) {
     ledState[core].fade = (ledState[core].fade * 0.9) - 0.1;
     if (ledState[core].fade < 1) ledState[core].fade = 0;
 
     #if RP2040_ZERO
       p.neoPixelFill(ledState[core].green * ledState[core].fade / 255, 
                      ledState[core].red * ledState[core].fade / 255, 
                      ledState[core].blue * ledState[core].fade / 255, true);
     #else
       pinMode(LED_PIN, OUTPUT);
       gpio_set_drive_strength(LED_PIN, GPIO_DRIVE_STRENGTH_12MA);
       gpio_set_function(LED_PIN, GPIO_FUNC_PWM);
       analogWrite(LED_PIN, (uint8_t)ledState[core].fade);
     #endif
 
     if (ledState[core].fade == 0) {
       #if RP2040_ZERO
         p.neoPixelClear(true);
       #else
         analogWrite(LED_PIN, -1);
         gpio_set_function(LED_PIN, GPIO_FUNC_SIO);
       #endif
     }
   }
   return true;
 }
 
 void BlinkFade(uint8_t led_brightness) {
   if (!LED_EN) return;
   uint8_t core = get_core_num();
   mutex_enter_blocking(&led_fade_mutex);
   ledState[core].fade = led_brightness;
   mutex_exit(&led_fade_mutex);
 }
 
