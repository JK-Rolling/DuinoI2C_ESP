/*
 * core.h
 * created by JK Rolling
 * 16 03 2025
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
 #ifndef __CORE_H__
 #define __CORE_H__
 
 #include <Wire.h>
 #include "duco_hash.h"
 
 #define BUFFER_MAX 90
 #define HASH_BUFFER_SIZE 20
 #define CHAR_END '\n'
 #define CHAR_DOT ','
 
 //#define HTOI(c) ((c<='9')?(c-'0'):((c<='F')?(c-'A'+10):((c<='f')?(c-'a'+10):(0))))
 #define HTOI(c) ((c<='9')?(c-'0'):((c<='f')?(c-'a'+10):(0)))
 #define TWO_HTOI(h, l) ((HTOI(h) << 4) + HTOI(l))
 //byte HTOI(char c) {return ((c<='9')?(c-'0'):((c<='f')?(c-'a'+10):(0)));}
 //byte TWO_HTOI(char h, char l) {return ((HTOI(h) << 4) + HTOI(l));}
 byte _table [] = {0x0, 0x7, 0xe, 0x9, 0x1c, 0x1b, 0x12, 0x15, 0x38, 0x3f, 0x36, 0x31, 0x24, 0x23, 0x2a, 0x2d, 0x70, 0x77, 0x7e, 0x79, 0x6c, 0x6b, 0x62, 0x65, 0x48, 0x4f, 0x46, 0x41, 0x54, 0x53, 0x5a, 0x5d, 0xe0, 0xe7, 0xee, 0xe9, 0xfc, 0xfb, 0xf2, 0xf5, 0xd8, 0xdf, 0xd6, 0xd1, 0xc4, 0xc3, 0xca, 0xcd, 0x90, 0x97, 0x9e, 0x99, 0x8c, 0x8b, 0x82, 0x85, 0xa8, 0xaf, 0xa6, 0xa1, 0xb4, 0xb3, 0xba, 0xbd, 0xc7, 0xc0, 0xc9, 0xce, 0xdb, 0xdc, 0xd5, 0xd2, 0xff, 0xf8, 0xf1, 0xf6, 0xe3, 0xe4, 0xed, 0xea, 0xb7, 0xb0, 0xb9, 0xbe, 0xab, 0xac, 0xa5, 0xa2, 0x8f, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9d, 0x9a, 0x27, 0x20, 0x29, 0x2e, 0x3b, 0x3c, 0x35, 0x32, 0x1f, 0x18, 0x11, 0x16, 0x3, 0x4, 0xd, 0xa, 0x57, 0x50, 0x59, 0x5e, 0x4b, 0x4c, 0x45, 0x42, 0x6f, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7d, 0x7a, 0x89, 0x8e, 0x87, 0x80, 0x95, 0x92, 0x9b, 0x9c, 0xb1, 0xb6, 0xbf, 0xb8, 0xad, 0xaa, 0xa3, 0xa4, 0xf9, 0xfe, 0xf7, 0xf0, 0xe5, 0xe2, 0xeb, 0xec, 0xc1, 0xc6, 0xcf, 0xc8, 0xdd, 0xda, 0xd3, 0xd4, 0x69, 0x6e, 0x67, 0x60, 0x75, 0x72, 0x7b, 0x7c, 0x51, 0x56, 0x5f, 0x58, 0x4d, 0x4a, 0x43, 0x44, 0x19, 0x1e, 0x17, 0x10, 0x5, 0x2, 0xb, 0xc, 0x21, 0x26, 0x2f, 0x28, 0x3d, 0x3a, 0x33, 0x34, 0x4e, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5c, 0x5b, 0x76, 0x71, 0x78, 0x7f, 0x6a, 0x6d, 0x64, 0x63, 0x3e, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2c, 0x2b, 0x6, 0x1, 0x8, 0xf, 0x1a, 0x1d, 0x14, 0x13, 0xae, 0xa9, 0xa0, 0xa7, 0xb2, 0xb5, 0xbc, 0xbb, 0x96, 0x91, 0x98, 0x9f, 0x8a, 0x8d, 0x84, 0x83, 0xde, 0xd9, 0xd0, 0xd7, 0xc2, 0xc5, 0xcc, 0xcb, 0xe6, 0xe1, 0xe8, 0xef, 0xfa, 0xfd, 0xf4, 0xf3};
 const float alpha = 0.2;         // Weight factor (0 < alpha <= 1), higher value gives more weight to recent values
 
 class core {
   private:
     TwoWire* bus;
     uint8_t addr;
     uint8_t sdaPin;
     uint8_t sclPin;
     char coreBuffer[BUFFER_MAX];
     uint8_t bufferPosition;
     uint8_t bufferLength;
     bool working;
     bool jobDone;
     float weightedMean = 0.0;        // Variable to store the weighted mean value
     int processedResults = 0;        // Counter to track the number of processed results
   
     // static instance pointers for callback routing
     static core* instance0;
     static core* instance1;
   
   public:
     // constructor
     core(TwoWire* wireInstance, uint8_t address, uint8_t sda_pin, uint8_t scl_pin) : 
       bus(wireInstance), addr(address), sdaPin(sda_pin), sclPin(scl_pin) {
         if (bus == &I2C0) instance0 = this;
         if (bus == &I2C1) instance1 = this;
         
       }
   
     void bus_setup() {
       bus->setSDA(sdaPin);
       bus->setSCL(sclPin);
       bus->setClock(WIRE_CLOCK);
       bus->begin(addr);
   
       if (bus == &I2C0) {
         I2C0.onReceive(handleReceive0);
         I2C0.onRequest(handleRequest0);
       }
       else if (bus == &I2C1) {
         I2C1.onReceive(handleReceive1);
         I2C1.onRequest(handleRequest1);
       }
 
       printMsgln("core" + String(get_core_num()) + ": addr=0x" + String(addr, HEX) + " SDA/SCL Pin=" + String(sdaPin) + "/" + String(sclPin) + " SCL Freq=" + String(WIRE_CLOCK) + "Hz");
     }
   
     // Static callback handlers
     static void handleReceive0(int numBytes) { if(instance0) instance0->receiveEvent(numBytes); }
     static void handleRequest0()             { if(instance0) instance0->requestEvent(); }
     static void handleReceive1(int numBytes) { if(instance1) instance1->receiveEvent(numBytes); }
     static void handleRequest1()             { if(instance1) instance1->requestEvent(); }
   
     void receiveEvent(int howMany) {
       if (howMany == 0) return;
       if (working) return;
       if (jobDone) return;
     
       for (int i=0; i < howMany; i++) {
         if (i == 0) {
           char c = bus->read();
           coreBuffer[bufferLength++] = c;
           if (bufferLength == BUFFER_MAX) bufferLength--;
           if ((c == CHAR_END) || (c == '$'))
           {
             working = true;
           }
         }
         else {
           // dump the rest
           bus->read();
         }
       }
     }
   
     void requestEvent() {
       char c = CHAR_END;
       if (jobDone) {
         c = coreBuffer[bufferPosition++];
         if (bufferPosition == bufferLength) {
           resetBuffer();
         }
       }
       bus->write(c);
     }
   
     void resetBuffer() {
       jobDone = false;
       bufferPosition = 0;
       bufferLength = 0;
       memset(coreBuffer, 0, sizeof(coreBuffer));
     }
   
     String getResponse() {
       return String(coreBuffer);
     }
   
     bool maxElapsed(unsigned long current, unsigned long max_elapsed) {
       static unsigned long _start = 0;
     
       if ((current - _start) > max_elapsed) {
         _start = current;
         return true;
       }
       return false;
     }
   
     bool coreLoop() {
       return doWork();
     }
   
     bool doWork()
     {
       if (working)
       {
         if (WDT_EN && wdt_pet) {
           watchdog_update();
         }
         
         // worker related command
         if (coreBuffer[0] == 'g') {
           // i2c_cmd
           //pos 0123[4]5678
           //    get,[t]emperature$
           //    get,[c]rc8$
           //    get,[b]aton$
           //    get,[s]inglecore$
           //    get,[f]req$
           //    get,[i]d$
           char f = coreBuffer[4];
           switch (tolower(f)) {
             case 't': // temperature
               if (SENSOR_EN) {
                 snprintf(coreBuffer, sizeof(coreBuffer), "%.2f", read_temperature());
               }
               else strcpy(coreBuffer, "0");
               printMsg("SENSOR_EN: ");
               break;
             case 'f': // i2c clock frequency
               ltoa(WIRE_CLOCK, coreBuffer, 10);
               printMsg("WIRE_CLOCK: ");
               break;
             case 'c': // crc8 status
               snprintf(coreBuffer, sizeof(coreBuffer), "%d", CRC8_EN);
               printMsg("CRC8_EN: ");
               break;
             case 'b': // core_baton
               snprintf(coreBuffer, sizeof(coreBuffer), "%d", CORE_BATON_EN);
               printMsg("CORE_BATON_EN: ");
               break;
             case 's' : // single core only
               snprintf(coreBuffer, sizeof(coreBuffer), "%d", SINGLE_CORE_ONLY);
               printMsg("SINGLE_CORE_ONLY: ");
               break;
             case 'n': // worker name
               strcpy(coreBuffer, WORKER_NAME);
               printMsg("WORKER: ");
               break;
             case 'i': // ducoid
               strcpy(coreBuffer, DUCOID.c_str());
               printMsg("DUCOID: ");
               break;
             default:
               strcpy(coreBuffer, "unkn");
               printMsg("command: ");
               printMsg(String(f));
               printMsg("response: ");
           }
           printMsgln(coreBuffer);
           bufferPosition = 0;
           bufferLength = strlen(coreBuffer);
           working = false;
           jobDone = true;
         }
         else if (coreBuffer[0] == 's') {
           char f = coreBuffer[4];
           // i2c_cmd
           //pos 0123[4]5678
           //    set,[d]im,123$
           switch (f) {
             case 'd': // led dim
               // Tokenize the buffer string using commas as delimiters
               char* token = strtok(coreBuffer, ","); // "set"
               token = strtok(NULL, ",");         // "dim"
               token = strtok(NULL, "$");         // "123" (or "1")
             
               // Convert the extracted token to a number
               if (token != NULL) {
                 led_brightness = atoi(token);
               }
               break;
           }
           bufferPosition = 0;
           bufferLength = 0;
           working = false;
           return false;
         }
         else {
           return doJob();
         }
       }
       return false;
     }
   
     bool doJob()
     {
       uint16_t job = 1;
       #if CRC8_EN
         // calculate crc8
         char *delim_ptr = strchr(coreBuffer, CHAR_DOT);
         delim_ptr = strchr(delim_ptr+1, CHAR_DOT);
         uint8_t job_length = strchr(delim_ptr+1, CHAR_DOT) - coreBuffer + 1;
         uint8_t calc_crc8 = crc8((uint8_t *)coreBuffer, job_length);
       
         // validate integrity
         char *rx_crc8 = strchr(delim_ptr+1, CHAR_DOT) + 1;
         if (calc_crc8 != atoi(rx_crc8)) {
           // data corrupted
           printMsgln("core" + String(get_core_num()) + ": CRC8 mismatched. Abort..");
           Blink(BLINK_BAD_CRC, LED_PIN);
           job = 0;
         }
       #endif
       
       uint32_t startTime = millis();
       if (job) {
         job = work();
       }
       uint32_t elapsedTime = millis() - startTime;
       
       memset(coreBuffer, 0, sizeof(coreBuffer));
       char cstr[6];
       // Job
       if (job == 0) {
         strcpy(cstr,"#"); // re-request job
       }
       else
         itoa(job, cstr, 10);
       strcpy(coreBuffer, cstr);
       coreBuffer[strlen(coreBuffer)] = CHAR_DOT;
     
       // Time
       // handles less than a millisecond
       if (elapsedTime == 0) elapsedTime = 1;
       // handles outlier
       float current_hr = (float)job / elapsedTime;
       // If fewer than 10 results have been processed, calculate a simple mean
       if (processedResults < 10) {
         weightedMean = ((weightedMean * processedResults) + current_hr) / (processedResults + 1);
         processedResults++;
       } else {
         // Calculate the weighted mean value
         weightedMean = (alpha * current_hr) + ((1 - alpha) * weightedMean);
     
         // Check if the new result deviates by more than 7% from the weighted mean
         if ((current_hr > weightedMean) && (abs(current_hr - weightedMean) > weightedMean * 0.07)) {
           elapsedTime += 1;
         }
       }
       itoa(elapsedTime, cstr, 10);
       strcpy(coreBuffer + strlen(coreBuffer), cstr);
       
       #if CRC8_EN
       char gen_crc8[3];
       coreBuffer[strlen(coreBuffer)] = CHAR_DOT;
     
       // CRC8
       itoa(crc8((uint8_t *)coreBuffer, strlen(coreBuffer)), gen_crc8, 10);
       strcpy(coreBuffer + strlen(coreBuffer), gen_crc8);
       #endif
     
       bufferPosition = 0;
       bufferLength = strlen(coreBuffer);
       working = false;
       jobDone = true;
       
       if (WDT_EN && wdt_pet) {
         watchdog_update();
       }
     
       if (job == 0) return false;
       return true;
     }
     
     int work()
     {
       // tokenize
       const char *delim = ",";
       char *lastHash = strtok(coreBuffer, delim);
       char *newHash = strtok(NULL, delim);
       char *diff = strtok(NULL, delim);
       
       bufferLength = 0;
       bufferPosition = 0;
       return work(lastHash, newHash, atoi(diff));
     }
     
     void HEX_TO_BYTE(char * address, char * hex, int len)
     {
       for (uint8_t i = 0; i < len; i++) address[i] = TWO_HTOI(hex[2 * i], hex[2 * i + 1]);
     }
     
     // DUCO-S1A hasher
     uint32_t work(char * lastblockhash, char * newblockhash, int difficulty)
     {
       HEX_TO_BYTE(newblockhash, newblockhash, HASH_BUFFER_SIZE);
       //static duco_hash_state_t hash;
       duco_hash_state_t hash;
       duco_hash_init(&hash, lastblockhash);
     
       char nonceStr[10 + 1];
       int nonce_max = difficulty*100+1;
       for (int nonce = 0; nonce < nonce_max; nonce++) {
         ultoa(nonce, nonceStr, 10);
     
         uint8_t const * hash_bytes = duco_hash_try_nonce(&hash, nonceStr);
         if (memcmp(hash_bytes, newblockhash, HASH_BUFFER_SIZE) == 0) {
           return nonce;
         }
         if (WDT_EN && wdt_pet) {
           if (maxElapsed(millis(), wdt_period_half)) {
             watchdog_update();
           }
         }
       }
     
       return 0;
     }
   
     #if CRC8_EN
     uint8_t crc8(uint8_t *args, uint8_t len) {
       uint8_t _sum = 0;
       for (uint8_t i=0; i<len; i++) {
         _sum = _table[_sum ^ args[i]];
       }
       return _sum;
     }
     #endif
 };
 
 #endif // __CORE_H__
 