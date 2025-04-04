/*
 DuinoI2C_ATTinyX5
 by JK Rolling
 31-3-2025
 Validated by jpx13

 Target: ATtiny85, ATtiny45
 Board Library: https://github.com/SpenceKonde/ReleaseScripts/raw/refs/heads/master/package_drazzy.com_index.json
 ** Tools --> Burn Bootloader; then only upload sketch with, Sketch --> Upload Using Programmer **
 Board attributes: 
  Attribute Value
  Board               ATtiny25/45/85 (No Bootloader)
  Chip                ATtiny85 or ATtiny45
  Clock Source        16.5 MHz (PLL,tweaked)
  Timer 1 Clock       CPU
  LTO                 Enabled
  millis()/micros()   Enabled
  Save EEPROM         EEPROM retained
  B.O.D Level         Disabled

  Libraries:
    Optional:
      ATtinySerialOut - https://github.com/ArminJo/ATtinySerialOut

 */
/****************** USER MODIFICATION START ****************/
#define I2C_AD                      0x08           // I2C 7-bits address 0x01-0x7F
#define WDT_EN                      true           // watchdog to reset when hung
#define LED_EN                      true           // Enable LED output on LED_PIN
#define CRC_EN                      true           // crc8 protection for data integrity
/****************** USER MODIFICATION END ******************/
/*---------------------------------------------------------*/
/****************** FINE TUNING START **********************/
#define LED_PIN                     PIN_PB1        // ATTiny->PB1:PWM PB3:GPIO PB4:GPIO; AT328P->LED_BUILTIN
#define LED_DIM                     false          // Dim LED. Set to true when LED do not have resistor
#define LED_MAX                     8              // 1-255. LED default brightness
#define SERIAL_EN                   false          // sw serial on PB4 for debug purpose. use ATTiny85 here.
/****************** FINE TUNING END ************************/
#include "def.h"

// SCL - PB2 - 2
// SDA - PB0 - 0
// TX  - PB4 - 4
uint8_t led_dim = LED_MAX;

alignas(4) char buffer[BUF_SIZE]; // 4-bytes aligned for duco_hash_block reuse
uint8_t buffer_pos;
uint8_t buffer_len;
uint8_t state; // IDLE:2'b00 BUSY:2'b01 DONE:2'b10 [done, working]
uint8_t clear_buffer_flag;
volatile char volatile_c; // vol strongly recommended for isr

// --------------------------------------------------------------------- //
// setup
// --------------------------------------------------------------------- //
void setup() {
  // setup serial
  serialBegin();

  // setup watchdog timer
  wdtBegin();

  // setup I2C
  BUS.begin(I2C_AD);
  BUS.onReceive(receiveISR);
  BUS.onRequest(requestISR);

  // setup LED
  ledSetup();

  // print startup messages
  serialPrint("Wire begin ");
  serialPrintln(I2C_AD);
}

// --------------------------------------------------------------------- //
// loop
// --------------------------------------------------------------------- //
void loop() {
  led_startup_blink();
  do_work();
  stopCheck(); // TinyWireS recommended frequent STOP check
}

// --------------------------------------------------------------------- //
// work
// --------------------------------------------------------------------- //
void do_work()
{
  if (clear_buffer_flag) {
    memset(buffer, 0, BUF_SIZE);
    clear_buffer_flag = 0;
  }
  if (state & 1) // busy
  {
    ledOn();
    // should print example content below
    // "buffer=[get,id]"
    // "buffer"[<last_hash>,<new_hash>,<diff>]"
    serialPrint("buffer=["); serialPrint(get_buffer_ptr()); serialPrintln("]");

    // figure out payload purpose
    char cmd = buffer[0];
    char op  = buffer[4];

    switch(cmd) {
        case 's': // set request
        case 'g': // get request
            serialPrint("cmd=["); serialPrint(op); serialPrint("] rsp=[");
            // i2c_cmd
            // pos 0123[4]5678
            //     get,[c]rc8$
            //     get,[i]d$
            switch(op) {
                case 'i': // ducoid
                    buffer_len = build_ducoid();
                    break;
                    
                case 'd': // led dim
                    #if LED_EN && LED_DIM
                    {
                    const char* ptr = get_buffer_ptr();
                    ptr += 8; // directly skip "set,dim,"

                    // Extract the number
                    uint8_t user_dim = custom_atoi(ptr);
                    led_dim = (user_dim > LED_MAX) ? LED_MAX : user_dim;
                    buffer_pos = buffer_len = state = 0; // mark idle
                    wdtReset();
                    }
                    #endif // LED_DIM
                    return;
                    break; // should not reach here

                #if CRC_EN
                case 'c': //crc8
                    //#if CRC_EN
                    {
                      buffer[0] = '1';
                      buffer_len = 1;
                    }
                    break;
                #endif

                default: // reply 0 for any unsupported command
                    buffer[0] = '0';
                    buffer_len = 1;
                    break;
            }
            serialPrint(get_buffer_ptr());
            serialPrintln("]");
            buffer_pos = 0;
            state = 2; // mark ans ready
            wdtReset();
            return;
  
        default:
            process_job();
            wdtReset();
            break;
    }
    ledOff();
  }
}

void process_job() {
  // default value
  uint16_t nonce = 1;
  uint16_t elapsed = 100;

  // check job format correctness
  if (buffer[SHA1_HASH_ASCII_LEN] != CHAR_DOT || 
      buffer[SHA1_HASH_ASCII_LEN * 2 + 1] != CHAR_DOT) {
    nonce = 0;
    serialPrintln("job corrupted!");
  }
  char *p = buffer;
  #if CRC_EN
  uint8_t calc_crc = 0, mstr_crc = 0;
  {
    uint8_t comma_count = 0;
    while (*p && comma_count < 3) {
      if (*p == CHAR_DOT) comma_count++;
      p++;
    }
    // check if master is sending crc
    mstr_crc = (comma_count > 2) ? 1 : 0;

    // calculate expected crc8
    if (mstr_crc & 1) calc_crc = crc8((uint8_t*)buffer, p - buffer);
  }
  #endif // CRC_EN

  
  /* swap hashes to reuse buffer memory in duco_hash_block later
   * original format. length = 86, free = 0
   * | last_hash | new_hash | diff |
   * |    40     |    40    |  2   |
   * 
   * swap hashes. length = 86, free = 0
   * | new_hash | last_hash | diff |
   * |    40     |    40    |  2   |
   * 
   * hex_to_bytes(new_hash). length = 66, free = 20
   * | new_hash(bytes) | free | last_hash | diff |
   * |    20           |  20  |     40    |  2   |
   * 
   * move last_hash to occupy free space. length = 66, free = 20
   * | new_hash(bytes) | last_hash | diff | free |
   * |    20           |     40    |  2   |  20  |
   * 
   * duco_hash_block to use this freed buffer which requires 64 bytes
   * | new_hash(bytes) | buffer |
   * |    20           |  67    |
   * 
   * duco_hash_block to write the result back to buffer
   * | new_hash(bytes) | result(bytes) | garbage |
   * |    20           |  20           |  47     |
   * 
   * memcmp to compare new_hash and resullt
   * | new_hash(bytes) | result(bytes) |
   * |    20           |  20           |
   */
  // swap hashes in-place
  swap_hashes_inplace(buffer);

  // replace all commas with NULL terminator
  {
    p = buffer;
    while (*p) {
      if (*p == CHAR_DOT) *p = '\0';
      p++;
    }
  }
  
  // Parse components
  // CRC:OFF "<last_hash>\0<new_hash>\0<diff>"
  // CRC:ON  "<last_hash>\0<new_hash>\0<diff>\0<crc>"
  char *new_hash  = buffer;
  char *last_hash = new_hash  + SHA1_HASH_ASCII_LEN + 1;
  char *diff      = last_hash + SHA1_HASH_ASCII_LEN + 1;

  serialPrint("new_hash=["); serialPrint(new_hash); serialPrintln("]");
  serialPrint("las_hash=["); serialPrint(last_hash); serialPrintln("]");
  serialPrint("difficul=["); serialPrint(diff); serialPrintln("]");
  
  #if CRC_EN
  char *recv_crc8 = diff; // point to diff, will adjust later

  // adjust crc pointer 
  while (*recv_crc8) recv_crc8++;
  recv_crc8++;

  // check if calculated crc matches with received crc. skip checks if not supported by master
  if ((calc_crc != custom_atoi(recv_crc8)) && mstr_crc) {
    serialPrint("crc8 mismatched. Re-request job");
    nonce = 0;
  }
  #endif // CRC_EN
  
  if (nonce) {
    // Convert hashes to byte in-place
    hex_to_bytes((uint8_t*)new_hash);

    // move last_hash left
    {
    uint8_t *dst = (uint8_t*)new_hash + SHA1_HASH_LEN;
    p = last_hash;
    for (uint8_t i = 0; i < SHA1_HASH_ASCII_LEN; i++) *dst++ = *p++;
    }

    // update last_hash pointer to new shifted location
    //last_hash = new_hash + SHA1_HASH_LEN; // replaced by (get_buffer_ptr() + SHA1_HASH_LEN) calls
    
    // Mining
    uint16_t start_time = (uint16_t) millis();
    nonce = mine(new_hash, custom_atoi(diff));
    elapsed = (uint16_t)((uint16_t) millis() - start_time);
  }

  // re-init buffer, prepare for result
  memset(buffer, 0, BUF_SIZE);
  buffer_len = 0;
  
  if (nonce) { // result found
    custom_utoa(nonce, buffer);
    while (buffer[buffer_len]) buffer_len++;
  }
  else { // no result found or corrupted job. re-request job
    buffer[buffer_len++] = CHAR_HSH;
  }

  // append ','
  buffer[buffer_len++] = CHAR_DOT;

  // append elapsed time
  custom_utoa(elapsed, &buffer[buffer_len]);
  while (buffer[buffer_len]) buffer_len++;

  #if CRC_EN
  // append ','
  buffer[buffer_len++] = CHAR_DOT;

  // append new crc8
  custom_utoa(crc8((uint8_t*)buffer, buffer_len), &buffer[buffer_len]);
  while (buffer[buffer_len]) buffer_len++;
  #endif // CRC_EN

  // print final answer to be sent back to master
  serialPrintln(get_buffer_ptr());

  buffer_pos = 0;
  state = 2; // mark ans ready
  
  wdtReset();
}

__attribute__((always_inline)) inline uint8_t htoi(uint8_t c) {
    return (c & 0x0F) + (c >> 6) * 9; // Valid for '0'-'9', 'a'-'f'
}

void hex_to_bytes(uint8_t *hex) {
    uint8_t *p = hex;
    for (uint8_t i = 0; i < SHA1_HASH_LEN; i++) {
        hex[i] = (htoi(p[0]) << 4 | htoi(p[1]));
        p += 2;
    }
}

uint16_t mine(char *target, uint8_t diff) {
  //if (diff > 16) return 0; // give up when diff too high
  duco_hash_init();
  
  static char nstr[5];
  uint16_t n = 0;
  uint16_t n_max = diff * 100 + 1;
  for(; n < n_max; n++){
    custom_utoa(n, nstr);
    //uint8_t const * hash_bytes = duco_hash_try_nonce(&hs, nstr);
    uint8_t const * hash_bytes = duco_hash_try_nonce(nstr);
    if (memcmp(hash_bytes, target, SHA1_HASH_LEN) == 0) {
      return n;
    }
  }
  serialPrintln("No result found");
  return 0;
}

uint8_t build_ducoid() {
    char *resp = get_buffer_ptr();
    
    // Copy "DUCOID" from PROGMEM directly
    memcpy_P(resp, PSTR("DUCOID"), 6); //+16B progmem -6B ram
    //memcpy(resp, "DUCOID", 6);
    resp += 6;  // Move pointer forward
    
    // https://microchip.my.site.com/s/article/Serial-number-in-AVR---Mega-Tiny-devices
    /*  ATTiny - 10 bytes
     *  Byte Address --------- Description
        0x0E --------- Lot Number 2nd Char
        0x0F --------- Lot Number 1st Char
        0x10 --------- Lot Number 4th Char
        0x11 --------- Lot Number 3rd Char
        0x12 --------- Lot Number 6th Char
        0x13 --------- Lot Number 5th Char
        0x14 --------- Reserved
        0x15 --------- Wafer Number
        0x16 --------- Y-coordinate
        0x17 --------- X-coordinate
        example : DUCOID756e6b776f0002012 (ArduinoUniqueID)
        thiscode: DUCOID6e756e6b776f
     */
    // Read and convert lot number
    for (uint8_t j = 0; j < 6; j++) {
      uint8_t id = boot_signature_byte_get(0x0E + j);

      // Add '0' padding if needed
      if (id < 0x10) {
        *resp++ = '0';
      }

      // Convert and store hex representation
      //utoa(id, resp); // too heavy for attiny
      custom_utoa(id, resp); // ducoid not as important at the moment
      resp += 2;  // Move pointer forward
    }

    *resp = '\0';  // Null-terminate the string
    return resp - get_buffer_ptr();  // Return the final length
}

// swap last_hash with new_hash
// from <last_hash>,<new_hash>
// to   <new_hash>,<last_hash>
void swap_hashes_inplace(char *buf) {
    char *last_hash = buf;  // Start of last_hash
    char *new_hash = buf + SHA1_HASH_ASCII_LEN + 1; // Start of new_hash (after first comma)

    // Swap characters in place
    uint8_t j=0;
    for (; j < SHA1_HASH_LEN*2; j++) {
        char temp = last_hash[j];
        last_hash[j] = new_hash[j];
        new_hash[j] = temp;
    }
}

void receiveISR(intWire howMany) {
  if (howMany  == 0) return; // false trigger
  if (state & 3) return; // busy or still have pending answer not sent

  // read 1 character
  volatile_c = BUS.read();

  // disable interrupt so the data can be stored into buffer safely
  cli();

  // save it in buffer and increment length counter
  buffer[buffer_len++] = volatile_c;

  // overflow prevention
  if (buffer_len == BUF_SIZE) buffer_len = BUF_SIZE - 1;

  // check for end of data
  if (volatile_c == CHAR_END || volatile_c == CHAR_DSN) {
    buffer[buffer_len] = '\0';
    buffer[BUF_SIZE-1] = '\0'; // for safety purpose?
    state |= 1; // mark busy
  }

  // re-enable interrupts
  sei();
  
  // flush the rest
  while (BUS.available()) BUS.read();
}

void requestISR() {
  // default answer
  char c = CHAR_END;
  
  if (state & 2) { // answer available
    // put 1 char at a time and increment buffer index
    c = buffer[buffer_pos++];

    // end of data
    if (buffer_pos == buffer_len) {
      state = 0; // mark idle
      
      // re-init
      buffer_pos = buffer_len = 0;
      clear_buffer_flag = 1;
    }
  }

  // send 1 byte
  BUS.write(c);
}

#if CRC_EN
// +192B progmem +4 sram
uint8_t crc8(uint8_t *data, uint8_t len) {
  uint8_t crc = 0;
  for (uint8_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t bit = 0; bit < 8; bit++) {
      if (crc & 0x80) {
        crc = (crc << 1) ^ 0x07;
      } else {
        crc = (crc << 1);
      }
    }
  }
  return crc;
}
#endif // CRC_EN

void led_startup_blink() {
    #if LED_EN
    static uint16_t previousMillisAndState = 0; // Packed: time (13 bits) + state (3 bits)
    uint16_t currentMillis = millis();
    uint8_t blinkState = previousMillisAndState & 0x07;
    
    if (blinkState >= 6) return;
    
    if (currentMillis - (previousMillisAndState >> 3) >= 100) {
        previousMillisAndState = (currentMillis << 3) | (blinkState + 1);
        if (blinkState & 1) ledOff(); else ledOn();
    }
    #endif
}

// --------------------------------------------------------------------- //
// helper function
// --------------------------------------------------------------------- //
char* get_buffer_ptr() { return buffer; }

uint8_t custom_atoi(const char* str) {
  uint8_t result = 0;
  while (*str >= '0' && *str <= '9') {
    result = result * 10 + (*str - '0');
    str++;
  }
  return result;
}

void custom_utoa(uint16_t num, char* str) {
    uint8_t i = 0;
    do {
        str[i++] = (num % 10) + '0';
        num /= 10;
    } while (num > 0);
    str[i] = '\0';
    // Reverse the string (optional, if order matters)
    for (uint8_t j = 0; j < i / 2; j++) {
        char tmp = str[j];
        str[j] = str[i - j - 1];
        str[i - j - 1] = tmp;
    }
}
