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

#define RRR
#define RRC
#define RRS

#define O_constant 2
#define O_register 5

static void
emit_nop (unsigned char opcode, int param)
{
}

static void
emit_arithmetic (unsigned char opcode, int param)
{
}

static void
emit_not (unsigned char opcode, int param)
{
}

static void
emit_mov (unsigned char opcode, int param)
{
}

amo_opcode_t amo_opcodes[] = {
	{ "nop", { { { 0, {  } }, emit_nop, 0b000000, 0 } } },
	{ "add", { { { 3, { O_register, O_register, O_constant } }, emit_arithmetic, 0b000000, INSN_TYPE_IMMEDIATE },
			   { { 3, { O_register, O_register, O_register } }, emit_arithmetic, 0b000001, INSN_TYPE_REGISTER } } },
	{ "adc", { { { 3, { O_register, O_register, O_constant } }, emit_arithmetic, 0b000010, INSN_TYPE_IMMEDIATE },
			   { { 3, { O_register, O_register, O_register } }, emit_arithmetic, 0b000011, INSN_TYPE_REGISTER } } },
	{ "sub", { { { 3, { O_register, O_register, O_constant } }, emit_arithmetic, 0b000100, INSN_TYPE_IMMEDIATE },
			   { { 3, { O_register, O_register, O_register } }, emit_arithmetic, 0b000101, INSN_TYPE_REGISTER } } },
	{ "and", { { { 3, { O_register, O_register, O_constant } }, emit_arithmetic, 0b000110, INSN_TYPE_IMMEDIATE },
			   { { 3, { O_register, O_register, O_register } }, emit_arithmetic, 0b000111, INSN_TYPE_REGISTER } } },
	{ "or", { { { 3, { O_register, O_register, O_constant } }, emit_arithmetic, 0b001000, INSN_TYPE_IMMEDIATE },
			   { { 3, { O_register, O_register, O_register } }, emit_arithmetic, 0b001001, INSN_TYPE_REGISTER } } },
	{ "xor", { { { 3, { O_register, O_register, O_constant } }, emit_arithmetic, 0b001010, INSN_TYPE_IMMEDIATE },
			   { { 3, { O_register, O_register, O_register } }, emit_arithmetic, 0b001011, INSN_TYPE_REGISTER } } },
	{ "not", { { { 2, { O_register, O_constant } }, emit_not, 0b001100, INSN_TYPE_IMMEDIATE },
			   { { 2, { O_register, O_register } }, emit_not, 0b001101, INSN_TYPE_REGISTER } } },
	{ "lsl", { { { 3, { O_register, O_register, O_constant } }, emit_arithmetic, 0b001110, INSN_TYPE_IMMEDIATE },
			   { { 3, { O_register, O_register, O_register } }, emit_arithmetic, 0b001111, INSN_TYPE_REGISTER } } },
	{ "lsr", { { { 3, { O_register, O_register, O_constant } }, emit_arithmetic, 0b010000, INSN_TYPE_IMMEDIATE },
			   { { 3, { O_register, O_register, O_register } }, emit_arithmetic, 0b010001, INSN_TYPE_REGISTER } } },
	{ "asr", { { { 3, { O_register, O_register, O_constant } }, emit_arithmetic, 0b010010, INSN_TYPE_IMMEDIATE },
			   { { 3, { O_register, O_register, O_register } }, emit_arithmetic, 0b010011, INSN_TYPE_REGISTER } } },
	{ "mov", { { { 2, { O_register, O_constant } }, emit_mov, 0b010100, INSN_TYPE_IMMEDIATE },
			   { { 2, { O_register, O_register } }, emit_mov, 0b010101, INSN_TYPE_REGISTER } } }
};

unsigned int amo_opcodes_size = ARRAY_SIZE (amo_opcodes);