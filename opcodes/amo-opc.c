/* amo-opc.c -- Instruction opcode for AMO
   Copyright (C) 2000-2024 Free Software Foundation, Inc.

   Author: Yechan Hong <yechan0815@naver.com>

   This file is part of libopcodes.

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   It is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this file; see the file COPYING.  If not, write to the Free
   Software Foundation, 51 Franklin Street - Fifth Floor, Boston, MA
   02110-1301, USA.  */

#include "opcode/amo.h"

#define ARRAY_SIZE(a) (sizeof (a) / sizeof ((a)[0]))

#define CONST 2
#define SYM 3
#define REG 5
#define DEREF 30
#define DEREF_REL 31

#define N { 0, {  } }
#define C { 1, { CONST } }
#define S { 1, { SYM } }
#define R { 1, { REG } }
#define RC { 2, { REG, CONST } }
#define RS { 2, { REG, SYM } }
#define RR { 2, { REG, REG } }
#define RD { 2, { REG, DEREF } }
#define RD_R { 2, { REG, DEREF_REL } }
#define DR { 2, { DEREF, REG } }
#define D_RR { 2, { DEREF_REL, REG } }
#define RRC { 3, { REG, REG, CONST } }
#define RRS { 3, { REG, REG, SYM } }
#define RRR { 3, { REG, REG, REG } }

#define ENTRY(opcode, cond) { cond, opcode },

OPCODESX(amo)

OPX(nop)
	ENTRY(PSEUDO_NOP, N)
ENDOPX
OPX(add)
	ENTRY(0b000000, RRC)
	ENTRY(0b000001, RRR)
ENDOPX
OPX(adc)
	ENTRY(0b000010, RRC)
	ENTRY(0b000011, RRR)
ENDOPX
OPX(sub)
	ENTRY(0b000100, RRC)
	ENTRY(0b000101, RRR)
ENDOPX
OPX(and)
	ENTRY(0b000110, RRC)
	ENTRY(0b000111, RRR)
ENDOPX
OPX(or)
	ENTRY(0b001000, RRC)
	ENTRY(0b001001, RRR)
ENDOPX
OPX(xor)
	ENTRY(0b001010, RRC)
	ENTRY(0b001011, RRR)
ENDOPX
OPX(not)
	ENTRY(0b001100, RC)
	ENTRY(0b001101, RR)
ENDOPX
OPX(lsl)
	ENTRY(0b001110, RRC)
	ENTRY(0b001111, RRR)
ENDOPX
OPX(lsr)
	ENTRY(0b010000, RRC)
	ENTRY(0b010001, RRR)
ENDOPX
OPX(asr)
	ENTRY(0b010010, RRC)
	ENTRY(0b010011, RRR)
ENDOPX
OPX(mov)
	ENTRY(0b010100, RC)
	ENTRY(0b010101, RR)
	ENTRY(PSEUDO_MOV, RS)
ENDOPX
OPX(ldr)
	ENTRY(0b010110, RD_R)
	ENTRY(0b010111, RD)
ENDOPX
OPX(str)
	ENTRY(0b011000, D_RR)
	ENTRY(0b011001, DR)
ENDOPX
OPX(beq)
	ENTRY(0b011010, RRS)
ENDOPX
OPX(bne)
	ENTRY(0b011011, RRS)
ENDOPX
OPX(blt)
	ENTRY(0b011100, RRS)
ENDOPX
OPX(ble)
	ENTRY(0b011101, RRS)
ENDOPX
OPX(jmp)
	ENTRY(0b011110, S)
	ENTRY(0b011111, R)
ENDOPX
OPX(jal)
	ENTRY(0b100000, S)
	ENTRY(0b100001, R)
ENDOPX
OPX(swi)
	ENTRY(0b100010, C)
ENDOPX
OPX(bltu)
	ENTRY(0b100011, RRS)
ENDOPX
OPX(bleu)
	ENTRY(0b100100, RRS)
ENDOPX
OPX(ext)
	ENTRY(0b100101, RRC)
ENDOPX
OPX(ldrb)
	ENTRY(0b100110, RD_R)
	ENTRY(0b100111, RD)
ENDOPX
OPX(strb)
	ENTRY(0b101000, D_RR)
	ENTRY(0b101001, DR)
ENDOPX
OPX(ldrh)
	ENTRY(0b101010, RD_R)
	ENTRY(0b101011, RD)
ENDOPX
OPX(strh)
	ENTRY(0b101100, D_RR)
	ENTRY(0b101101, DR)
ENDOPX

ENDOPCODESX

unsigned int amo_opcodes_size = ARRAY_SIZE (amo_opcodes);

/* clean up */
#undef CONST
#undef SYM
#undef REG
#undef N
#undef C
#undef S
#undef R
#undef RC
#undef RR
#undef RRC
#undef RRS
#undef RRR
#undef ENTRY
