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

#define ENTRY(func, p) { dec_##func, p },
#define DECODE(name, ptype, pname) static void dec_##name (ptype pname)

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

DECODE(nop, int, type ATTRIBUTE_UNUSED)
{
	sprintf (buf, "nop");
}

/* in decoding parts, decode the arithmetic and logical instruction together */
DECODE(arithmetic, int, type)
{
	unsigned int src, dst, opn;
	unsigned short imm;

	src = (insn_dec.binary >> 21) & MASK_REGISTER;
	if (type == TYPE_IMM)
	{
		dst = (insn_dec.binary >> 16) & MASK_REGISTER;
		imm = insn_dec.binary & MASK_IMM16;

		sprintf (buf, "%-5sr%d, r%d, $0x%hx (%hd)", insn_dec.name, dst, src, imm, imm);
		return ;
	}
	else if (type == TYPE_REG)
	{
		opn = (insn_dec.binary >> 16) & MASK_REGISTER;
		dst = (insn_dec.binary >> 11) & MASK_REGISTER;

		sprintf (buf, "%-5sr%d, r%d, r%d", insn_dec.name, dst, src, opn);
		return ;
	}

	/* impossible */
	abort ();
}

DECODE(not, int, type)
{
	unsigned int src, dst;
	unsigned short imm;

	if (type == TYPE_IMM)
	{
		dst = (insn_dec.binary >> 16) & MASK_REGISTER;
		imm = insn_dec.binary & MASK_IMM16;

		sprintf (buf, "%-5sr%d, $0x%hx (%hd)", insn_dec.name, dst, imm, imm);
		return ;
	}
	else if (type == TYPE_REG)
	{
		src = (insn_dec.binary >> 16) & MASK_REGISTER;
		dst = (insn_dec.binary >> 11) & MASK_REGISTER;

		sprintf (buf, "%-5sr%d, r%d", insn_dec.name, dst, src);
		return ;
	}

	/* impossible */
	abort ();
}

DECODE(mov, int, type)
{
	unsigned int src, dst;
	unsigned int imm;

	dst = (insn_dec.binary >> 21) & MASK_REGISTER;
	if (type == TYPE_IMM)
	{
		imm = insn_dec.binary & MASK_IMM21;
		if ((imm >> 20))
			imm |= 0xFFE00000;

		sprintf (buf, "%-5sr%d, $0x%x (%d)", insn_dec.name, dst, imm, imm);
		return ;
	}
	else if (type == TYPE_REG)
	{
		src = (insn_dec.binary >> 16) & MASK_REGISTER;

		sprintf (buf, "%-5sr%d, r%d", insn_dec.name, dst, src);
		return ;
	}

	/* impossible */
	abort ();
}

DECODE(ldr, int, type)
{
	unsigned int base, dst;
	unsigned int imm;

	if (type == TYPE_DREL)
	{
		dst = (insn_dec.binary >> 21) & MASK_REGISTER;
		imm = insn_dec.binary & MASK_IMM21;
		if ((imm >> 20))
			imm |= 0xFFE00000;

		sprintf (buf, "%-5sr%d, [$0x%x]", insn_dec.name, dst, imm);
		return ;
	}
	else if (type == TYPE_DEREF)
	{
		base = (insn_dec.binary >> 21) & MASK_REGISTER;
		dst = (insn_dec.binary >> 16) & MASK_REGISTER;
		imm = insn_dec.binary & MASK_IMM16;
		if ((imm >> 15))
			imm |= 0xFFFF0000;

		if (imm)
			sprintf (buf, "%-5sr%d, [r%d, $0x%x]", insn_dec.name, dst, base, imm);
		else
			sprintf (buf, "%-5sr%d, [r%d]", insn_dec.name, dst, base);
		return ;
	}

	/* impossible */
	abort ();
}

DECODE(str, int, type)
{
	unsigned int base, dst;
	unsigned int imm;

	if (type == TYPE_DREL)
	{
		dst = (insn_dec.binary >> 21) & MASK_REGISTER;
		imm = insn_dec.binary & MASK_IMM21;
		if ((imm >> 20))
			imm |= 0xFFE00000;

		sprintf (buf, "%-5s[$0x%x], r%d", insn_dec.name, imm, dst);
		return ;
	}
	else if (type == TYPE_DEREF)
	{
		base = (insn_dec.binary >> 21) & MASK_REGISTER;
		dst = (insn_dec.binary >> 16) & MASK_REGISTER;
		imm = insn_dec.binary & MASK_IMM16;
		if ((imm >> 15))
			imm |= 0xFFFF0000;

		if (imm)
			sprintf (buf, "%-5s[r%d, $0x%x], r%d", insn_dec.name, base, imm, dst);
		else
			sprintf (buf, "%-5s[r%d], r%d", insn_dec.name, base, dst);
		return ;
	}

	/* impossible */
	abort ();
}

DECODE(branch, int, type ATTRIBUTE_UNUSED)
{
	unsigned int src, opn;
	unsigned int imm;

	src = (insn_dec.binary >> 21) & MASK_REGISTER;
	opn = (insn_dec.binary >> 16) & MASK_REGISTER;

	imm = insn_dec.binary & MASK_IMM16;
	if ((imm >> 15))
		imm |= 0xFFFF0000;

	sprintf (buf, "%-5sr%d, r%d, $0x%x (%d)", insn_dec.name, src, opn, imm << 2, imm << 2);
}

DECODE(jump, int, type)
{
	unsigned int src;
	unsigned int imm;

	if (type == TYPE_IMM)
	{
		imm = insn_dec.binary & MASK_IMM21;
		if ((imm >> 20))
			imm |= 0xFFE00000;

		sprintf (buf, "%-5s$0x%x", insn_dec.name, imm << 2);
		return ;
	}
	else if (type == TYPE_REG)
	{
		src = (insn_dec.binary >> 21) & MASK_REGISTER;

		sprintf (buf, "%-5sr%d", insn_dec.name, src);
		return ;
	}

	/* impossible */
	abort ();
}

DECODE(swi, int, type ATTRIBUTE_UNUSED)
{
	unsigned int imm;

	imm = insn_dec.binary & MASK_IMM8;

	sprintf (buf, "%-5s$0x%x (%d)", insn_dec.name, imm, imm);
}

DECFUNCS(amo)

DECFUNC(nop)
	ENTRY(nop, TYPE_NONE)
ENDDECFUNC
DECFUNC(add)
	ENTRY(arithmetic, TYPE_IMM)
	ENTRY(arithmetic, TYPE_REG)
ENDDECFUNC
DECFUNC(adc)
	ENTRY(arithmetic, TYPE_IMM)
	ENTRY(arithmetic, TYPE_REG)
ENDDECFUNC
DECFUNC(sub)
	ENTRY(arithmetic, TYPE_IMM)
	ENTRY(arithmetic, TYPE_REG)
ENDDECFUNC
DECFUNC(and)
	ENTRY(arithmetic, TYPE_IMM)
	ENTRY(arithmetic, TYPE_REG)
ENDDECFUNC
DECFUNC(or)
	ENTRY(arithmetic, TYPE_IMM)
	ENTRY(arithmetic, TYPE_REG)
ENDDECFUNC
DECFUNC(xor)
	ENTRY(arithmetic, TYPE_IMM)
	ENTRY(arithmetic, TYPE_REG)
ENDDECFUNC
DECFUNC(not)
	ENTRY(not, TYPE_IMM)
	ENTRY(not, TYPE_REG)
ENDDECFUNC
DECFUNC(lsl)
	ENTRY(arithmetic, TYPE_IMM)
	ENTRY(arithmetic, TYPE_REG)
ENDDECFUNC
DECFUNC(lsr)
	ENTRY(arithmetic, TYPE_IMM)
	ENTRY(arithmetic, TYPE_REG)
ENDDECFUNC
DECFUNC(asr)
	ENTRY(arithmetic, TYPE_IMM)
	ENTRY(arithmetic, TYPE_REG)
ENDDECFUNC
DECFUNC(mov)
	ENTRY(mov, TYPE_IMM)
	ENTRY(mov, TYPE_REG)
ENDDECFUNC
DECFUNC(ldr)
	ENTRY(ldr, TYPE_DREL)
	ENTRY(ldr, TYPE_DEREF)
ENDDECFUNC
DECFUNC(str)
	ENTRY(str, TYPE_DREL)
	ENTRY(str, TYPE_DEREF)
ENDDECFUNC
DECFUNC(beq)
	ENTRY(branch, TYPE_NONE)
ENDDECFUNC
DECFUNC(bne)
	ENTRY(branch, TYPE_NONE)
ENDDECFUNC
DECFUNC(blt)
	ENTRY(branch, TYPE_NONE)
ENDDECFUNC
DECFUNC(ble)
	ENTRY(branch, TYPE_NONE)
ENDDECFUNC
DECFUNC(jmp)
	ENTRY(jump, TYPE_IMM)
	ENTRY(jump, TYPE_REG)
ENDDECFUNC
DECFUNC(jal)
	ENTRY(jump, TYPE_IMM)
	ENTRY(jump, TYPE_REG)
ENDDECFUNC
DECFUNC(swi)
	ENTRY(swi, TYPE_NONE)
ENDDECFUNC
DECFUNC(bltu)
	ENTRY(branch, TYPE_NONE)
ENDDECFUNC
DECFUNC(bleu)
	ENTRY(branch, TYPE_NONE)
ENDDECFUNC
DECFUNC(ext)
	ENTRY(arithmetic, TYPE_IMM)
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
#undef DECODE
