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
#define REG 5

#define N { 0, {  } }
#define RR { 2, { REG, REG } }
#define RC { 2, { REG, CONST } }
#define RRR { 3, { REG, REG, REG } }
#define RRC { 3, { REG, REG, CONST } }

#define OPCODESX(name) amo_opcode_t name##_opcodes[] = {
#define ENDOPCODESX };
#define OPX(mnemonic) { #mnemonic, {
#define ENDOPX } },
#define ENTRY(opcode, cond) { cond, opcode },

OPCODESX(amo)

OPX(nop)
	ENTRY(0b000000, N)
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

/* d */
OPX(mov)
	ENTRY(0b010100, RC)
	ENTRY(0b010101, RR)
ENDOPX

ENDOPCODESX

unsigned int amo_opcodes_size = ARRAY_SIZE (amo_opcodes);

/* clean up */
#undef CONST
#undef REG
#undef RR
#undef RC
#undef RRR
#undef RRC
#undef OPCODESX
#undef ENDOPCODESX
#undef OPX
#undef ENDOPX
#undef ENTRY

