/* sha1.h */
/*
    This file is part of the AVR-Crypto-Lib.
    Copyright (C) 2006-2015 Daniel Otte (bg@nerilex.org)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/**
 * \file  sha1.h
 * \author  Daniel Otte
 * \email   bg@nerilex.org
 * \date  2006-10-08
 * \license GPLv3 or later
 * \brief   SHA-1 declaration.
 * \ingroup SHA-1
 * 
 */
 /**
 * \file  renamed to duco_hash.h
 * \author  JK Rolling
 * \email   jk.rolling.here@gmail.com
 * \date  2025-04-04
 * 
 */
 
 #ifndef SHA1_H_
 #define SHA1_H_
 
 #include <stdint.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 /** \fn duco_hash_init(void *msg)
  * \brief prehash for last_hash
  * This function sets the initialization vector
  * and ran first 10 loops of SHA-1 hashing.
  * \param pointer to the last_hash
  */
extern void duco_hash_init(void *msg);
 
 /** \fn duco_hash_try_nonce(void *buf, void *nonce)
  * \brief hashing a message which in located entirely in RAM
  * This function automatically hashes a message which is entirely in RAM with
  * the SHA-1 hashing algorithm.
  * \param dest pointer to the hash value destination
  * \param msg  pointer to the message which should be hashed
  * \param length_b length of the message in bits
  */ 
extern void duco_hash_try_nonce(void *buf, void *nonce);
 
extern uint32_t w10[10];
extern uint32_t tempState[5];
extern uint32_t workState[5];
extern char nonceStr[10+1];
extern char buffer[90];
#ifdef __cplusplus
}
#endif
 
#endif /*SHA1_H_*/