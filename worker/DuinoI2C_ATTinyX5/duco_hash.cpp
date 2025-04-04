#pragma GCC optimize ("-O2")
#include "duco_hash.h"

#define sha1_rotl(bits,word)     (((word) << (bits)) | ((word) >> (32 - (bits))))

// implemented in DuinoI2C_ATTinyX5.ino
extern char* get_buffer_ptr();

// stores precomputed hex32 of last_hash
uint32_t w10[10]; // 40 bytes
uint32_t tempState[5]; // 20 bytes

void duco_hash_block() {
  // group into 4 bytes
  uint32_t *w = (uint32_t*)(get_buffer_ptr() + SHA1_HASH_LEN);
  // single byte access
  uint8_t *b8 = (uint8_t*)(get_buffer_ptr() + SHA1_HASH_LEN);

  // reload pre-calculated hex from duco_hash_init()
  for (uint8_t i = 0; i < 10; i++) {
    w[i] = w10[i];
  }
//  memcpy(w, w10, sizeof(w10));

  // resume hex conversion from last_hash for nonce + padding
  uint32_t temp;
  for (uint8_t i = 10; i < 16; i++) {
    temp =  (uint32_t(b8[(i << 2) + 0])  << 24) |
            (uint32_t(b8[(i << 2) + 1])  << 16) |
            (uint32_t(b8[(i << 2) + 2])  <<  8) |
            (uint32_t(b8[(i << 2) + 3]));
    w[i & 15] = temp;
  }

  uint32_t a = tempState[0];
  uint32_t b = tempState[1];
  uint32_t c = tempState[2];
  uint32_t d = tempState[3];
  uint32_t e = tempState[4];

  for (uint8_t i = 10; i < 80; i++) {
    if (i >= 16) {
      w[i & 15] = sha1_rotl(1, w[(i-3) & 15] ^ w[(i-8) & 15] ^ w[(i-14) & 15] ^ w[(i-16) & 15]);
    }

    temp = sha1_rotl(5, a) + e + w[i & 15];
    if (i < 20) {
      temp += ((b & c) | ((~b) & d)) + 0x5a827999;
    } else if(i < 40) {
      temp += (b ^ c ^ d) + 0x6ed9eba1;
    } else if(i < 60) {
      temp += ((b & c) | (b & d) | (c & d)) + 0x8f1bbcdc;
    } else {
      temp += (b ^ c ^ d) + 0xca62c1d6;
    }

    e = d;
    d = c;
    c = sha1_rotl(30, b);
    b = a;
    a = temp;
  }

  a += 0x67452301;
  b += 0xefcdab89;
  c += 0x98badcfe;
  d += 0x10325476;
  e += 0xc3d2e1f0;

  // write result back to buffer
//  for (uint8_t i = 0; i < 4; i++) {
//    b8[ 0 + i] = (a >> (24 - (i << 3)));
//    b8[ 4 + i] = (b >> (24 - (i << 3)));
//    b8[ 8 + i] = (c >> (24 - (i << 3)));
//    b8[12 + i] = (d >> (24 - (i << 3)));
//    b8[16 + i] = (e >> (24 - (i << 3)));
//  }

  // unrolllled
  b8[0 * 4 + 0] = a >> 24;
  b8[0 * 4 + 1] = a >> 16;
  b8[0 * 4 + 2] = a >> 8;
  b8[0 * 4 + 3] = a;
  b8[1 * 4 + 0] = b >> 24;
  b8[1 * 4 + 1] = b >> 16;
  b8[1 * 4 + 2] = b >> 8;
  b8[1 * 4 + 3] = b;
  b8[2 * 4 + 0] = c >> 24;
  b8[2 * 4 + 1] = c >> 16;
  b8[2 * 4 + 2] = c >> 8;
  b8[2 * 4 + 3] = c;
  b8[3 * 4 + 0] = d >> 24;
  b8[3 * 4 + 1] = d >> 16;
  b8[3 * 4 + 2] = d >> 8;
  b8[3 * 4 + 3] = d;
  b8[4 * 4 + 0] = e >> 24;
  b8[4 * 4 + 1] = e >> 16;
  b8[4 * 4 + 2] = e >> 8;
  b8[4 * 4 + 3] = e;
}

void duco_hash_init() {
  
  // reuse ctx.buffer[20:BUF_SIZE] memory
  uint8_t *last_hash = (uint8_t *)(get_buffer_ptr() + SHA1_HASH_LEN);

  if (get_buffer_ptr() == (void*)(0xffffffff)) {
		// NOTE: THIS IS NEVER CALLED
		// This is here to keep a live reference to hash_block
		// Otherwise GCC tries to inline it entirely into the main loop
		// Which causes massive perf degradation
		//duco_hash_block(nullptr);
   duco_hash_block();
	}

	// Do first 10 rounds as these are going to be the same all time
  uint32_t a = 0x67452301;
  uint32_t b = 0xefcdab89;
  uint32_t c = 0x98badcfe;
  uint32_t d = 0x10325476;
  uint32_t e = 0xc3d2e1f0;

  // stores precomputed partial hex
  for (uint8_t i = 0; i < 10; i++) {
    w10[i] =  (uint32_t(last_hash[(i << 2) + 0])  << 24) |
              (uint32_t(last_hash[(i << 2) + 1])  << 16) |
              (uint32_t(last_hash[(i << 2) + 2])  <<  8) |
              (uint32_t(last_hash[(i << 2) + 3]));
              
    uint32_t temp = sha1_rotl(5, a) + e + w10[i & 15];
    temp += (b & c) | ((~b) & d);
    temp += 0x5a827999;

    e = d;
    d = c;
    c = sha1_rotl(30, b);
    b = a;
    a = temp;
  }

  tempState[0] = a;
  tempState[1] = b;
  tempState[2] = c;
  tempState[3] = d;
  tempState[4] = e;
}

void duco_hash_set_nonce(char const * nonce) {
  uint8_t * b = (uint8_t*) (get_buffer_ptr() + SHA1_HASH_LEN);
	uint8_t off = SHA1_HASH_ASCII_LEN;
  
  // append nonce to last_hash
  // <last_hash><nonce>
  uint8_t i=0;
  while (i < 5 && nonce[i] != 0) {
    b[off++] = nonce[i];
    i++;
  }

	uint8_t total_bytes = off;

  // append 0x80 to nonce
  // <last_hash><nonce><0x80>
	b[off++] = 0x80;

  // pad with zeroes up to offset 62
  // <last_hash><nonce><0x80><62 0x0>
	while (off < 62) {
		b[off++] = 0;
	}

  // closing with byte length
	//b[62] = total_bytes >> 5; // always 1
  b[62] = 1;
  // possible value: 0x48, 0x50, 0x58, 0x60, 0x68 for total_bytes from 41-45 respectively
	b[63] = total_bytes << 3;
}

uint8_t const * duco_hash_try_nonce(char const * nonce) {
  duco_hash_set_nonce(nonce);
  duco_hash_block();

  return (uint8_t*) (get_buffer_ptr() + SHA1_HASH_LEN);
}
