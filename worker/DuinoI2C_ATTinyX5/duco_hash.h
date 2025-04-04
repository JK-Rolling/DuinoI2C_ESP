#pragma once

#include <arduino.h>

#define SHA1_HASH_LEN 20
#define SHA1_HASH_ASCII_LEN SHA1_HASH_LEN * 2

void duco_hash_init();
uint8_t const * duco_hash_try_nonce(char const * nonce);
