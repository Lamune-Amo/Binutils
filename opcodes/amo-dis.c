/* amo-dis.c -- Disassembler interface for AMO
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

#include "sysdep.h"
#include "opcode/amo.h"
#include "disassemble.h"
#include "bfd.h"
#include <stdio.h>

#define DECFUNCS(name) amo_decfunc_t name##_decfuncs[] = {
#define ENDDECFUNCS };
#define DECFUNC(mnemonic) { {
#define ENDDECFUNC } },
#define ENTRY(func, p) { func, p },

struct amo_instruction_dec
{
	char name[MNEMONIC_LENGTH_MAX];
	unsigned int binary;
	unsigned char opcode;
};

typedef struct amo_decfunc
{
	struct
	{
		void (*f)(int param);
		int parameter;
	} operations[OPERATION_PER_INSTRUCTION_MAX];
} amo_decfunc_t;

static char buf[DISASSEMBLE_DECODE_LENGTH + 1];
static struct amo_instruction_dec insn_dec;

void dec_nop (int param)
{
	sprintf (buf, "nop");
}

void dec_arithmetic (int type)
{
	unsigned int src, dst, opn;
	unsigned short imm;

	src = (insn_dec.binary >> 21) & MASK_REGISTER;
	if (type == TYPE_IMM)
	{
		dst = (insn_dec.binary >> 16) & MASK_REGISTER;
		imm = insn_dec.binary & MASK_IMM16;

		sprintf (buf, "%-5sr%d, r%d, $0x%hx", insn_dec.name, dst, src, imm);
	}
	else if (type == TYPE_REG)
	{
		opn = (insn_dec.binary >> 16) & MASK_REGISTER;
		dst = (insn_dec.binary >> 11) & MASK_REGISTER;

		sprintf (buf, "%-5sr%d, r%d, r%d", insn_dec.name, dst, src, opn);
	}
}

void dec_not (int type)
{
	sprintf (buf, "mov");
}

void dec_mov (int type)
{
	sprintf (buf, "mov");
}

DECFUNCS(amo)

DECFUNC(nop)
	ENTRY(dec_nop, TYPE_NONE)
ENDDECFUNC
DECFUNC(add)
	ENTRY(dec_arithmetic, TYPE_IMM)
	ENTRY(dec_arithmetic, TYPE_REG)
ENDDECFUNC
DECFUNC(adc)
	ENTRY(dec_arithmetic, TYPE_IMM)
	ENTRY(dec_arithmetic, TYPE_REG)
ENDDECFUNC
DECFUNC(sub)
	ENTRY(dec_arithmetic, TYPE_IMM)
	ENTRY(dec_arithmetic, TYPE_REG)
ENDDECFUNC
DECFUNC(and)
	ENTRY(dec_arithmetic, TYPE_IMM)
	ENTRY(dec_arithmetic, TYPE_REG)
ENDDECFUNC
DECFUNC(or)
	ENTRY(dec_arithmetic, TYPE_IMM)
	ENTRY(dec_arithmetic, TYPE_REG)
ENDDECFUNC
DECFUNC(xor)
	ENTRY(dec_arithmetic, TYPE_IMM)
	ENTRY(dec_arithmetic, TYPE_REG)
ENDDECFUNC
DECFUNC(not)
	ENTRY(dec_not, TYPE_IMM)
	ENTRY(dec_not, TYPE_REG)
ENDDECFUNC
DECFUNC(lsl)
	ENTRY(dec_arithmetic, TYPE_IMM)
	ENTRY(dec_arithmetic, TYPE_REG)
ENDDECFUNC
DECFUNC(lsr)
	ENTRY(dec_arithmetic, TYPE_IMM)
	ENTRY(dec_arithmetic, TYPE_REG)
ENDDECFUNC
DECFUNC(asr)
	ENTRY(dec_arithmetic, TYPE_IMM)
	ENTRY(dec_arithmetic, TYPE_REG)
ENDDECFUNC
DECFUNC(mov)
	ENTRY(dec_mov, TYPE_IMM)
	ENTRY(dec_mov, TYPE_REG)
ENDDECFUNC

ENDDECFUNCS

int print_insn_amo(bfd_vma addr, disassemble_info *info)
{
	unsigned char b[BYTES_PER_INSTRUCTION];
	unsigned int i, j;

	memset (buf, 0, sizeof (buf));
	memset (&insn_dec, 0, sizeof (struct amo_instruction_dec));
	if (info->read_memory_func (addr, (bfd_byte *) b, BYTES_PER_INSTRUCTION, info))
	{
		info->fprintf_func (info->stream, "Error: failed to read binary.\n");
		return -1;
	}	
	insn_dec.binary = (b[3] << 24) | (b[2] << 16) | (b[1] << 8) | b[0];
	insn_dec.opcode = (insn_dec.binary >> 26);

	for (i = 0; i < amo_opcodes_size; i++)
	{
		for (j = 0; j < OPERATION_PER_INSTRUCTION_MAX; j++)
		{
			if (!amo_decfuncs[i].operations[j].f)
				break;
			if (amo_opcodes[i].operations[j].opcode == insn_dec.opcode)
				goto matched;
		}
	}

	sprintf (buf, "unsupported opcode");
	goto end;

matched:
	strcpy (insn_dec.name, amo_opcodes[i].name);
	amo_decfuncs[i].operations[j].f (amo_decfuncs[i].operations[j].parameter);

end:
	for (i = strlen (buf); i < DISASSEMBLE_DECODE_LENGTH; i++)
		buf[i] = ' ';
	printf ("%s", buf);

	return BYTES_PER_INSTRUCTION;
}

/* clean up */
#undef ENTRY
