/* sha1-asm.S */
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
/*
 * Author:  Daniel Otte
 *
 * License: GPLv3 or later
*/
/*
 * Author:  JK Rolling
 *
 * Changes:
 *   * Optimize for Duino-Coin hashing
*/
; SHA1 implementation in assembler for AVR

; global variable
.section .data

.align 2
.global w10
w10:
  .space 40  ; 10 words x 4 bytes
  
.align 2
.global tempState
tempState:
  .space 20  ; 5 words x 4 bytes

.align 2
.global workState
workState:
  .space 20 ; 5 words x 4 bytes

.global nonceStr
nonceStr:
  .space 11 ; 10 chars max

.align 2
.global buffer
buffer:
  .space 90 ; stores job, result, and work area

.section .text

SPL = 0x3D
SPH = 0x3E
SREG = 0x3F

xtmp = 0
xNULL = 1
TS1 = 8
TS2 = 9
W1 = 10
W2 = 11
T1  = 12
T2  = 13
T3  = 14
T4  = 15
LoopC = 16
S   = 17
tmp1 = 18
tmp2 = 19
tmp3 = 20
tmp4 = 21
F1 = 22
F2 = 23
F3 = 24
F4 = 25

ZH = 31
ZL = 30
YH = 29
YL = 28
XH = 27
XL = 26

;###########################################################

.global duco_hash_init
; this function calculate and store first 40 chars state and digest
; param1: the 16-bit pointer to message (last_hash)
; param1 is given in r25:r24 (r25 is the most significant)
duco_hash_init:
  push  r13 
  push  r14 
  push  r15 
  push  r16
  in r18, SPL
  push r18
  in r18, SPH
  push r18
  
  movw r12, r24 /* backup of msg pointer */

  ; update X to point to tempState
  ; rcall sha1_iv_init to copy IV from ram into tempState
  ldi r24, lo8(tempState)
  ldi r25, hi8(tempState)
  rcall sha1_iv_init

  ; perform last_hash endian32 swap and store the result in w10
  movw ZL, r12 /* mov last_hash ptr to Z */
  ldi r20, lo8(w10)
  ldi r21, hi8(w10)
  movw XL, r20 /* X points to w10 in RAM */
  movw W1, r20 /* save pointer to w10[0] */
  ldi LoopC, 10
1:
  ldd tmp1, Z+3
  st X+, tmp1
  ldd tmp1, Z+2
  st X+, tmp1
  ldd tmp1, Z+1
  st X+, tmp1
  ld tmp1, Z
  st X+, tmp1
  adiw r30, 4 ; advance Z to next 32-bit word
  dec LoopC
  brne 1b

  ; clr LoopC /* LoopC is already 0 anyway. LoopC is t in FIPS 180-2 */
  clr xtmp
sha1_prehash_loop:
  ; execute first 10 loops of sha-1 round function
  mov S, LoopC
  lsl S
  lsl S
  /* load w10[S] */
  movw XL, W1
  add XL, S /* X points at w10[S] */
  adc XH, xNULL
  ld T1, X+
  ld T2, X+
  ld T3, X+
  ld T4, X+

sha1_prehash_loop_core:
  /* T=ROTL5(a) + e + w10[LoopC] */
  ; point X to IV
  ldi XL, lo8(tempState)
  ldi XH, hi8(tempState)
  movw TS1, XL /* save pointer to tempState */
  adiw XL, 4*4  /* X points at tempState[4] aka e */
  
  ; load and add e
  ld tmp1, X+
  add T1, tmp1
  ld tmp1, X+
  adc T2, tmp1
  ld tmp1, X+
  adc T3, tmp1
  ld tmp1, X+
  adc T4, tmp1    /* T = w[s]+e */

  ; load a
  ;sbiw r26, 4*5   /* X points at a[0] aka a */
  movw XL, TS1  /* X points to a[0] aka a */
  ld F1, X+
  ld F2, X+
  ld F3, X+
  ld F4, X+
  mov tmp1, F4    /* X points at a[1] aka b */

  ; ROTL5(a)
  ldi tmp2, 5
1:
  rol tmp1
  rol F1
  rol F2
  rol F3
  rol F4
  dec tmp2
  brne 1b

  add T1, F1
  adc T2, F2
  adc T3, F3
  adc T4, F4 /* T = ROTL(a,5) + e + w[s] */

  movw YL, TS1   
  adiw YL, 1*4  ; Y = pointer to a[1] (b)
  rcall sha1_nextBlock_Ch
  mov F1, tmp1
  rcall sha1_nextBlock_Ch
  mov F2, tmp1
  rcall sha1_nextBlock_Ch
  mov F3, tmp1
  rcall sha1_nextBlock_Ch

  add T1, F1
  adc T2, F2
  adc T3, F3
  adc T4, tmp1
  
  ; Add constant K = 0x5A827999
  ldi ZL, lo8(sha1_nextBlock_KTable)
  ldi ZH, hi8(sha1_nextBlock_KTable)
  lpm tmp1, Z+
  add T1, tmp1
  lpm tmp1, Z+
  adc T2, tmp1
  lpm tmp1, Z+
  adc T3, tmp1
  lpm tmp1, Z+
  adc T4, tmp1

  /* T = ROTL5(a) + f_t(b,c,d) + e + k_t + w[s] */
  /* X points still at a[1] aka b, Y points at a[2] aka c */
  /* update a[] */
sha1_prehash_update_a:
  /*first we move all vars in a[] "one up" e=d, d=c, c=b, b=a*/
  movw r28, TS1
  adiw r28, 4*4 /* Y should point at a[4] aka e */
  ldi tmp2, 4*4
1:
  ld tmp1, -Y
  std Y+4, tmp1
  dec tmp2
  brne 1b
  /* Y points at a[0] aka a*/
  movw r28, TS1

  /* store T in a[0] aka a */
  st Y+, T1
  st Y+, T2
  st Y+, T3
  st Y+, T4
  
  /* Y points at a[1] aka b*/
  /* rotate c */
  ldd T1, Y+1*4
  ldd T2, Y+1*4+1
  ldd T3, Y+1*4+2
  ldd T4, Y+1*4+3

  mov tmp1, T1
  ldi tmp2, 2
1:  ror tmp1
  ror T4
  ror T3
  ror T2
  ror T1
  dec tmp2
  brne 1b
  std Y+1*4+0, T1
  std Y+1*4+1, T2
  std Y+1*4+2, T3
  std Y+1*4+3, T4

  inc LoopC
  cpi LoopC, 10
  brge duco_hash_init_epilog
  rjmp sha1_prehash_loop

duco_hash_init_epilog:
  pop r18
  out SPH, r18
  pop r18
  out SPL, r18
  pop r16
  pop r15 
  pop r14 
  pop r13 
  ret

;###########################################################

; this function finishes the remaining 69 loops of sha-1 hashing
; pre-requisite: run duco_hash_init with last_hash to setup w10 and tempState
;
; param1: the 16-bit pointer to message (buffer)
; param1 is given in r25:r24 (r25 is the most significant)
; param2: the 16-bit pointer to nonce
; param2 is given in r23:r22
;
; result shall be stored back into last_hash
.global duco_hash_try_nonce
duco_hash_try_nonce:
  in r18, SPL
  push r18
  in r18, SPH
  push r18
  
  movw XL, r24
  movw W1, r24  ; saves buffer ptr
  movw ZL, r22

duco_hash_set_nonce:
  ldi r20, 40          ; off = 40 (stored in r20)
  add XL, r20
  adc XH, r1

1:
  ld r22, Z+
  tst r22
  breq append_0x80

  st X+, r22
  inc r20
  rjmp 1b

append_0x80:
  mov r21, r20
  ldi r22, 0x80
  st X+, r22
  inc r20

2:
  cpi r20, 62
  brsh write_length

  st X+, r1
  inc r20
  rjmp 2b

write_length:
  ldi r22, 1
  st X+, r22

  lsl r21
  lsl r21
  lsl r21
  st X, r21

duco_hash_block:
  ; copy w10 into buffer, overwriting last_hash
  ldi ZL, lo8(w10) ; source w10
  ldi ZH, hi8(w10)
  movw XL, W1 ; destination w
  ldi r16, 10 ; 10*4 bytes to copy
1:
  ld r21, Z+
  ld r20, Z+
  ld r19, Z+
  ld r18, Z+
  st X+, r21
  st X+, r20
  st X+, r19
  st X+, r18
  dec r16
  brne 1b

  ; copy tempState into workState (a-e)
  ldi ZL, lo8(tempState)
  ldi ZH, hi8(tempState)
  ldi XL, lo8(workState)
  ldi XH, hi8(workState)
  movw TS1, XL ; saves workState
  ldi r16, 5 ; 5*4 bytes to copy
2:
  ld r21, Z+
  ld r20, Z+
  ld r19, Z+
  ld r18, Z+
  st X+, r21
  st X+, r20
  st X+, r19
  st X+, r18
  dec r16
  brne 2b

  ; convert the paddings into hex32
  ; in-place replace
  movw ZL, W1
  adiw ZL, 40
  adc ZH, r1
  movw XL, ZL

  ldi LoopC, 6
3:
  ld tmp1, Z+
  ld tmp2, Z+
  ld tmp3, Z+
  ld tmp4, Z+
  st X+, tmp4
  st X+, tmp3
  st X+, tmp2
  st X+, tmp1
  dec LoopC
  brne 3b
  
  ; do sha-1 round from iteration 10-79
  ; a-e loaded in workState from tempState
  ; message schedule loaded in W1 from w10
  ; message schedule loop 0-9 completed
  ; hashing resume at loop 10-79
duco_nextBlock:
  ldi LoopC, 10 ; starts from iteration 10
  clr xtmp
duco_nextBlock_mainloop:
  mov S, LoopC
  lsl S
  lsl S
  andi S, 0x3C
  movw XL, W1
  add XL, S
  adc XH, xNULL
  ld T1, X+
  ld T2, X+
  ld T3, X+
  ld T4, X+

  cpi LoopC, 16
  brlt duco_nextBlock_mainloop_core
  ldi tmp1, 2*4
  rcall 1f
  ldi tmp1, 8*4
  rcall 1f
  ldi tmp1, 13*4
  rcall 1f
  rjmp 2f
  
1:
  add tmp1, S
  andi tmp1, 0x3f
  movw XL, W1
  add XL, tmp1
  adc XH, xNULL
  ld tmp2, X+
  eor T1, tmp2
  ld tmp2, X+
  eor T2, tmp2
  ld tmp2, X+
  eor T3, tmp2
  ld tmp2, X+
  eor T4, tmp2
  ret
2: /* ROTL and save T back */
  mov tmp2, T4
  rol tmp2
  rol T1
  rol T2
  rol T3
  rol T4
  movw XL, W1
  add XL, S
  adc XH, xNULL
  st X+, T1
  st X+, T2
  st X+, T3
  st X+, T4

duco_nextBlock_mainloop_core:
  /* T already contain w[s] */
  movw XL, TS1
  adiw XL, 4*4 ; X points at e
  ld tmp1, X+
  add T1, tmp1
  ld tmp1, X+
  adc T2, tmp1
  ld tmp1, X+
  adc T3, tmp1
  ld tmp1, X+
  adc T4, tmp1 ; T = w[s] + e
  
  movw XL, TS1 ; X points at a
  ld F1, X+
  ld F2, X+
  ld F3, X+
  ld F4, X+
  mov tmp1, F4 ; X points at b
  ldi tmp2, 5
  
1:
  rol tmp1
  rol F1
  rol F2
  rol F3
  rol F4
  dec tmp2
  brne 1b
  
  add T1, F1
  adc T2, F2
  adc T3, F3
  adc T4, F4 ; T = ROTL5(a) + e + w[s]

  ; choose between algo
  ldi ZL, lo8(sha1_nextBlock_xTable)
  ldi ZH, hi8(sha1_nextBlock_xTable)
  add ZL, xtmp
  adc ZH, xNULL
  lpm tmp1, Z
  cp tmp1, LoopC
  brne 1f
  inc xtmp
1:
  ldi ZL, lo8(sha1_nextBlock_KTable)
  ldi ZH, hi8(sha1_nextBlock_KTable)
  lsl xtmp
  lsl xtmp
  add ZL, xtmp
  adc ZH, xNULL
  lsr xtmp
  lsr xtmp
  
  lpm tmp1, Z+
  add T1, tmp1
  lpm tmp1, Z+
  adc T2, tmp1
  lpm tmp1, Z+
  adc T3, tmp1
  lpm tmp1, Z+
  adc T4, tmp1 ; T = ROTL5(a) + e + kt + w[s] (validated)
  
  /* Z-4 is pointing at kt */
  movw YL, XL
  adiw ZL, 3*4 ; Z points to the right location in the jump-vector-table
  lsr r31
  ror r30
  
  ; run 4 times to fully process 32-bits
  icall
  mov F1, tmp1
  icall
  mov F2, tmp1
  icall
  mov F3, tmp1
  icall
  
  add T1, F1
  adc T2, F2
  adc T3, F3
  adc T4, tmp1 ; T = ROTL5(a) + f_t(b,c,d) + e + k_t + w[s]
  /* X pointing at b, Y pointing at c */
  
  ; update a-e
  movw YL, TS1
  adiw YL, 4*4 /* Y should point at a[4] aka e */
  ldi tmp2, 4*4
1:
  ld tmp1, -Y
  std Y+4, tmp1
  dec tmp2
  brne 1b
  /* Y points at a[0] aka a*/
  movw YL, TS1

  /* store T in a[0] aka a */
  st Y+, T1
  st Y+, T2
  st Y+, T3
  st Y+, T4
  
  /* Y points at a[1] aka b*/
  /* rotate c */
  ldd T1, Y+1*4
  ldd T2, Y+1*4+1
  ldd T3, Y+1*4+2
  ldd T4, Y+1*4+3

  mov tmp1, T1
  ldi tmp2, 2
1:  ror tmp1
  ror T4
  ror T3
  ror T2
  ror T1
  dec tmp2
  brne 1b
  std Y+1*4+0, T1
  std Y+1*4+1, T2
  std Y+1*4+2, T3
  std Y+1*4+3, T4
  
  inc LoopC
  cpi LoopC, 80
  brge duco_add_iv
  rjmp duco_nextBlock_mainloop

duco_add_iv:
  movw XL, TS1 ; destination
  ldi ZL, lo8(sha1_init_vector)
  ldi ZH, hi8(sha1_init_vector) ; source
  ldi LoopC, 5 ; 20 bytes
1:
  lpm tmp1, Z+
  ld tmp2, X
  add tmp2, tmp1
  st X+, tmp2
  lpm tmp1, Z+
  ld tmp2, X
  adc tmp2, tmp1
  st X+, tmp2
  lpm tmp1, Z+
  ld tmp2, X
  adc tmp2, tmp1
  st X+, tmp2
  lpm tmp1, Z+
  ld tmp2, X
  adc tmp2, tmp1
  st X+, tmp2
  dec LoopC
  brne 1b

  ; write result back to buffer
  movw XL, W1  ; destination
  movw ZL, TS1 ; source
  ldi LoopC, 5 ; 20 bytes
duco_write_digest:
  ldd tmp1, Z+3
  st X+, tmp1
  ldd tmp1, Z+2
  st X+, tmp1
  ldd tmp1, Z+1
  st X+, tmp1
  ld tmp1, Z
  st X+, tmp1
  adiw ZL, 4
  dec LoopC
  brne duco_write_digest
  
duco_hash_try_nonce_epilog:
  pop r18
  out SPH, r18
  pop r18
  out SPL, r18
  ret

;###########################################################

sha1_nextBlock_xTable:
.byte 20,40,60,0
sha1_nextBlock_KTable:
.int  0x5a827999
.int  0x6ed9eba1
.int  0x8f1bbcdc
.int  0xca62c1d6
sha1_nextBlock_JumpTable:
rjmp sha1_nextBlock_Ch
  nop
rjmp sha1_nextBlock_Parity
  nop
rjmp sha1_nextBlock_Maj
  nop
rjmp sha1_nextBlock_Parity

   /* X and Y still point at a[1] aka b ; return value in tmp1 */
sha1_nextBlock_Ch:
  ld tmp1, Y+
  mov tmp2, tmp1
  com tmp2
  ldd tmp3, Y+3 /* load from c */
  and tmp1, tmp3
  ldd tmp3, Y+7 /* load from d */
  and tmp2, tmp3
  eor tmp1, tmp2
  ret

sha1_nextBlock_Maj:
  ld tmp1, Y+
  mov tmp2, tmp1
  ldd tmp3, Y+3 /* load from c */
  and tmp1, tmp3
  ldd tmp4, Y+7 /* load from d */
  and tmp2, tmp4
  eor tmp1, tmp2
  and tmp3, tmp4
  eor tmp1, tmp3
  ret

sha1_nextBlock_Parity:
  ld tmp1, Y+
  ldd tmp2, Y+3 /* load from c */
  eor tmp1, tmp2
  ldd tmp2, Y+7 /* load from d */
  eor tmp1, tmp2
  ret

sha1_iv_init:
  movw r26, r24 ; (24,25) --> (26,27) load X with param1
  ldi r30, lo8((sha1_init_vector))
  ldi r31, hi8((sha1_init_vector))
  ldi r22, 5*4 /* bytes to copy */
sha1_iv_init_vloop:
  lpm r23, Z+
  st X+, r23
  dec r22
  brne sha1_iv_init_vloop
  ret

sha1_init_vector:
.int 0x67452301;
.int 0xefcdab89;
.int 0x98badcfe;
.int 0x10325476;
.int 0xc3d2e1f0;
