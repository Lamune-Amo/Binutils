/* tc-amo.c -- Assembler for the AMO
   Copyright (C) 2000-2024 Free Software Foundation, Inc.

   Author: Yechan Hong <yechan0815@naver.com>

   This file is part of GAS, the GNU Assembler.

   GAS is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GAS is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GAS; see the file COPYING.  If not, write to the Free
   Software Foundation, 51 Franklin Street - Fifth Floor, Boston, MA
   02110-1301, USA.  */

#include "as.h"
#include "bfd.h"
#include "opcode/amo.h"

#define OPFUNCS(name) amo_opfunc_t name##_opfuncs[] = {
#define ENDOPFUNCS };
#define FUNCX(mnemonic) { {
#define ENDFUNCX } },
#define ENTRY(func, p) { func, p },

/* command line option */
const char *md_shortopts = "";
struct option md_longopts[] = {};
size_t md_longopts_size = sizeof (md_longopts);

/* character which always start a comment */
const char comment_chars[] = ";";
/* character which start a comment at the beginning of a line */
const char line_comment_chars[] = "#";
/* character which may be used to separate multiple commands on a single line.  */
const char line_separator_chars[] = "";

/* there is no floating point */
const char EXP_CHARS[] = "";
const char FLT_CHARS[] = "";

/* no pseudo opcode (directive) */
const pseudo_typeS md_pseudo_table[] =
{
	{ (char *) NULL, (void (*)(int)) NULL, 0 }
};

static struct hash_control *amo_reg_hash;

struct amo_instruction
{
	/* mnemonic */
	char name[MNEMONIC_LENGTH_MAX];

	/* total number of opernad */
	unsigned char number;
	
	/* opernads */
	expressionS operands[OPERAND_PER_INSTRUCTION_MAX];
};

typedef struct amo_opfunc
{
	struct
	{
		/* operation function */
		void (*f)(unsigned char opcode, int param);
		/* operation parameter */
		int parameter;
	} operations[OPERATION_PER_INSTRUCTION_MAX];
} amo_opfunc_t;

struct amo_instruction insn;

int
md_parse_option (int c ATTRIBUTE_UNUSED, const char *arg ATTRIBUTE_UNUSED)
{
	return 0;
#ifdef USE_PARSE_OPTION
	/* should return 1 if option parsing is successful */
	return 1;
#endif
}

void
md_show_usage (FILE *stream)
{
	/* print the help for custom options */
	fprintf(stream, "\namo options: TODO\n");
}

static void
declare_register (const char *name, int number)
{
	const char *err;
	symbolS *regS;
	
	regS = symbol_create (name, reg_section, number, &zero_address_frag);
	if ((err = hash_insert (amo_reg_hash, S_GET_NAME (regS), (PTR) regS)))
		as_fatal (_("register set initialization failed: %s"), err);
}

static void
declare_register_set (void)
{
	char name[5];
	int i;
	
	for (i = 0; i < 32; ++i)
    {
		sprintf (name, "r%d", i);
		declare_register (name, i);
    }
}

/* this function gets invoked once when assembler is initialized. */
void
md_begin (void)
{
	int i;

	/* make register set hash table */
	amo_reg_hash = hash_new();

	/* declare common register set */
	declare_register_set ();
	/* declare special registers */
	declare_register ("sp", 29);
	declare_register ("fp", 30);
	declare_register ("lr", 31);
	/* declare system registers */
	declare_register ("cr0", 32);
}

static int
expression_constant (char *str)
{
	unsigned char radix;

	gas_assert (*str == '$');

	str++;
	radix = 10;

	/* hexadecimal */
	if (!strncmp (str, "0x", 2))
	{
		str += 2;
		radix = 16;
	}

	/* binary */
	if (!strncmp (str, "0b", 2))
	{
		str += 2;
		radix = 2;
	}

	/* decimal (signed) */
	return strtol (str, NULL, radix);
}

static int
parse_dereference (char *name, expressionS *e)
{
	symbolS *sym;
	char *mark;

	name++;
	name [strlen (name) - 1] = '\0';

	e->X_op = O_dereference;

	mark = name;
	while (*mark && *mark != ',')
		mark++;
	if (!*mark)
	{
		sym = hash_find (amo_reg_hash, name);
		if (sym)
		{
			e->X_add_symbol = sym;
			return 1;
		}
		/* abort this if there is no register at first */
		abort ();
	}

	*mark = '\0';
	sym = hash_find (amo_reg_hash, name);
	if (!sym)
		/* abort this if there is no valid register at first */
		abort ();
	e->X_add_symbol = sym;
	e->X_add_number = expression_constant (mark + 1);

	return 1;
}

static int
parse_constant (const char *name, expressionS *e)
{
	e->X_op = O_constant;
	e->X_add_number = expression_constant (name);

	return 1;
}

static int 
parse_operand (char *name, expressionS *e)
{
	/* catch the keywords and let the linker handle undefined symbols. */
	symbolS *sym;

	/* register */
	sym = hash_find (amo_reg_hash, name);
	if (sym)
	{
		e->X_op = O_register;
		e->X_add_number = S_GET_VALUE (sym);
		return 1;
	}
	/* dereference */
	if (*name == '[')
		return parse_dereference (name, e);
	/* constant */
	if (*name == '$')
		return parse_constant (name, e);
	/* symbol */
	/* symbol naming rules */
	/* symbol name cant start from number */
	if (isdigit (*name))
		as_bad ("invalid symbol name %s: symbol can't start from number", name);

	sym = symbol_find_or_make (name);
	
	gas_assert (sym != 0);

	e->X_op = O_symbol;
	e->X_add_symbol = sym;
	
	/* throw the problem to ld */
	return 1;
}

static char *
parse_mnemonic (char *str)
{
	char *mark;

	mark = str;
	while (*mark && !isspace (*mark))
		mark++;
	if (*mark)
	{
		*mark = '\0';
		strcpy (insn.name, str);

		return mark + 1;
	}
	strcpy (insn.name, str);

	return mark;
}

static char *
parse_operands (char *str)
{
	unsigned int brakets_depth;
	char *mark, *name;

	brakets_depth = 0;
	while (*str)
	{
		mark = str;
		while (*mark)
		{
			if (*mark == '[')
				brakets_depth++;
			else if (*mark == ']')
				brakets_depth--;
			if (brakets_depth == 0 && *mark == ',')
				break;
			mark++;
		}
		if (brakets_depth)
		{
			/* syntax error */
			as_bad("incorrect use of brackets");
		}
		name = str;
		str = mark;
		if (*mark)
		{
			*mark = '\0';
			str = mark + 1;
		}
		(void) parse_operand (name, &insn.operands[insn.number++]);
	}
	return str;
}

/*
static void amo_emit_insn(void *insn,
						  expressionS *lhs,
						  expressionS *rhs)
{
	gas_assert(insn != 0);

	int e_insn = 0;
	char *frag = frag_more(AMO_BYTES_SLOT_INSTRUCTION);
	int where;

	e_insn |= (insn->pad << 31);
	e_insn |= ((insn->opcode & 0x7F) << 24);
	e_insn |= ((insn->admode & 0x3F) << 18);
	e_insn |= ((insn->src0 & 0x3F) << 12);
	e_insn |= ((insn->src1 & 0x3F) << 6);
	e_insn |= (insn->dst & 0x3F);

	printf("writing instruction data: %08x\n", e_insn);

	md_number_to_chars(frag, e_insn, AMO_BYTES_SLOT_INSTRUCTION);

	if (insn->pad)
	{
		frag = frag_more(AMO_BYTES_SLOT_IMMEDIATE);

		if (OP_JUMP == insn->opcode || OP_JUMP_REF == insn->opcode)
		{
			expressionS *addr_expr = symbol_get_value_expression(lhs->X_add_symbol);

			know(O_symbol == addr_expr->X_op);

			where = frag - frag_now->fr_literal;

			addr_expr->X_add_number = 0;

			if(OP_JUMP == insn->opcode)
			{
				(void) fix_new_exp(frag_now, where, 4, addr_expr, 0, BFD_RELOC_32);
			}
			else
			{
				(void) fix_new_exp(frag_now, where, 4, addr_expr, 1, BFD_RELOC_AMO_RELATIVE);
			}
		}

		md_number_to_chars(frag, insn->immv, AMO_BYTES_SLOT_IMMEDIATE);
	}
}*/

static void
emit_nop (unsigned char opcode, int param)
{
	char *frag;
	
	frag = frag_more (BYTES_PER_INSTRUCTION);

	md_number_to_chars(frag, 0x00000000, BYTES_PER_INSTRUCTION);
}

static void
emit_arithmetic (unsigned char opcode, int param)
{
	unsigned long binary;
	char *frag;
	int imm;
	
	/* get a new frag */
	frag = frag_more (BYTES_PER_INSTRUCTION);
	binary = 0;

	binary |= (opcode << 26);
	binary |= ((insn.operands[1].X_add_number & MASK_REGISTER) << 21);
	if (param == TYPE_IMM)
	{
		imm = insn.operands[2].X_add_number;
		/* is this overflow ? */
		if (imm & ~MASK_IMM16)
			/* pool ? */
			as_bad ("16-bit immediate must be in the range -32768 to 32767.");
		binary |= ((insn.operands[0].X_add_number & MASK_REGISTER) << 16);
		binary |= imm & MASK_IMM16;
	}
	else if (param == TYPE_REG)
	{
		binary |= ((insn.operands[2].X_add_number & MASK_REGISTER) << 16);
		binary |= ((insn.operands[0].X_add_number & MASK_REGISTER) << 11);
	}
	else
	{
		/* impossible ! */
		abort ();
	}

	md_number_to_chars(frag, binary, BYTES_PER_INSTRUCTION);
}

static void
emit_not (unsigned char opcode, int param)
{
	unsigned long binary;
	char *frag;
	int imm;
	
	/* get a new frag */
	frag = frag_more (BYTES_PER_INSTRUCTION);
	binary = 0;

	binary |= (opcode << 26);
	if (param == TYPE_IMM)
	{
		imm = insn.operands[1].X_add_number;
		/* is this overflow ? */
		if (imm & ~MASK_IMM16)
			as_bad ("16-bit immediate must be in the range %hd to %hd.", 0x8000, 0x7fff);
		binary |= ((insn.operands[0].X_add_number & MASK_REGISTER) << 16);
		binary |= imm & MASK_IMM16;
	}
	else if (param == TYPE_REG)
	{
		binary |= ((insn.operands[1].X_add_number & MASK_REGISTER) << 16);
		binary |= ((insn.operands[0].X_add_number & MASK_REGISTER) << 11);
	}
	else
	{
		/* impossible ! */
		abort ();
	}

	md_number_to_chars(frag, binary, BYTES_PER_INSTRUCTION);
}

static void
emit_mov (unsigned char opcode, int param)
{
	unsigned long binary;
	char *frag;
	int imm;
	
	/* get a new frag */
	frag = frag_more (BYTES_PER_INSTRUCTION);
	binary = 0;

	binary |= (opcode << 26);
	binary |= ((insn.operands[0].X_add_number & MASK_REGISTER) << 21);
	if (param == TYPE_IMM)
	{
		imm = insn.operands[1].X_add_number;
		/* is this overflow ? */
		if (imm & ~MASK_IMM21)
			as_bad ("21-bit immediate must be in the range -1048576 to 1048575");
		binary |= imm & MASK_IMM21;
	}
	else if (param == TYPE_REG)
	{
		binary |= ((insn.operands[1].X_add_number & MASK_REGISTER) << 16);
	}
	else
	{
		/* impossible ! */
		abort ();
	}

	/* literal pool */

	md_number_to_chars(frag, 0x42424242, BYTES_PER_INSTRUCTION);
}

OPFUNCS(amo)

FUNCX(nop)
	ENTRY(emit_nop, TYPE_NONE)
ENDFUNCX
FUNCX(add)
    ENTRY(emit_arithmetic, TYPE_IMM)
	ENTRY(emit_arithmetic, TYPE_REG)
ENDFUNCX
FUNCX(adc)
    ENTRY(emit_arithmetic, TYPE_IMM)
	ENTRY(emit_arithmetic, TYPE_REG)
ENDFUNCX
FUNCX(sub)
    ENTRY(emit_arithmetic, TYPE_IMM)
	ENTRY(emit_arithmetic, TYPE_REG)
ENDFUNCX
FUNCX(and)
    ENTRY(emit_arithmetic, TYPE_IMM)
	ENTRY(emit_arithmetic, TYPE_REG)
ENDFUNCX
FUNCX(or)
    ENTRY(emit_arithmetic, TYPE_IMM)
	ENTRY(emit_arithmetic, TYPE_REG)
ENDFUNCX
FUNCX(xor)
    ENTRY(emit_arithmetic, TYPE_IMM)
	ENTRY(emit_arithmetic, TYPE_REG)
ENDFUNCX
FUNCX(not)
    ENTRY(emit_arithmetic, TYPE_IMM)
	ENTRY(emit_arithmetic, TYPE_REG)
ENDFUNCX
FUNCX(lsl)
    ENTRY(emit_arithmetic, TYPE_IMM)
	ENTRY(emit_arithmetic, TYPE_REG)
ENDFUNCX
FUNCX(lsr)
    ENTRY(emit_arithmetic, TYPE_IMM)
	ENTRY(emit_arithmetic, TYPE_REG)
ENDFUNCX
FUNCX(asr)
    ENTRY(emit_arithmetic, TYPE_IMM)
	ENTRY(emit_arithmetic, TYPE_REG)
ENDFUNCX
FUNCX(mov)
    ENTRY(emit_mov, TYPE_IMM)
	ENTRY(emit_mov, TYPE_REG)
ENDFUNCX

ENDOPFUNCS

static amo_opcode_t *
insn_search (const char *name)
{
	unsigned int i;

	for (i = 0; i < amo_opcodes_size; i++)
	{
		if (!strcmp (amo_opcodes[i].name, name))
			return &amo_opcodes[i];
	}

	return NULL;
}

void
md_assemble(char *str)
{
	amo_opcode_t *insp;
	amo_opfunc_t *fp;
	int i, j;

	memset (&insn, 0, sizeof (struct amo_instruction));

	str = parse_mnemonic (str);
	str = parse_operands (str);

	insp = insn_search (insn.name);
	fp = amo_opfuncs + (unsigned int) (insp - amo_opcodes);
	if (!insp)
	{
		as_bad ("unknown instruction '%s'", insn.name);
		return ;
	}
	for (i = 0; i < OPERATION_PER_INSTRUCTION_MAX; i++)
	{
		if (!fp->operations[i].f || insn.number != insp->operations[i].condition.number)
			/* skip */
			continue;

		for (j = 0; j < insp->operations[i].condition.number; j++)
			if (insn.operands[j].X_op != insp->operations[i].condition.types[j])
				break;
		if (j == insp->operations[i].condition.number)
		{
			/* matched ! */
			fp->operations[i].f (insp->operations[i].opcode, fp->operations[i].parameter);
			break;
		}
	}
	if (i == OPERATION_PER_INSTRUCTION_MAX)
		as_bad ("invalid usage: '%s'", insn.name);
}

symbolS *
md_undefined_symbol (char *name ATTRIBUTE_UNUSED)
{
	/* treat a symbol as extern if assembler finds undefined symbol */
	return NULL;
}

/* again, no floating point exptressions available */
const char *md_atof(int type, char *list, int *size)
{
	(void) type;
	(void) list;
	(void) size;
	return 0;
}

/* cant round up sections yet */
valueT md_section_align(asection *seg, valueT size)
{
	int align = bfd_get_section_alignment(stdoutput, seg);
	int new_size = ((size + (1 << align) - 1) & -(1 << align));

	return new_size;
}

/* also, generally no support for frag conversion */
void md_convert_frag(bfd *abfd, asection *seg, fragS *fragp)
{
	(void) abfd;
	(void) seg;
	(void) fragp;
	as_fatal(_("unexpected call"));
}

/* not possible, instaed convert to relocs and write them */
void md_apply_fix(fixS *fixp, valueT *val, segT seg)
{
	char *buf;

	/* 'fixup_segment' drops 'fx_addsy' and resets 'fx_pcrel' in case of a successful resolution */
	if (0 == fixp->fx_addsy && 0 == fixp->fx_pcrel)
	{
		buf = fixp->fx_frag->fr_literal + fixp->fx_where;
		if (BFD_RELOC_AMO_PCREL == fixp->fx_r_type)
		{
			bfd_put_32(stdoutput, *val, buf);
			fixp->fx_done = 1;
		}
	}
}

/* if not applied/resolved, turn a fixup into a relocation entry */
arelent *tc_gen_reloc(asection *seg, fixS *fixp)
{
	arelent *reloc;
	symbolS *sym;

	gas_assert(fixp != 0);

	reloc = XNEW(arelent);
	reloc->sym_ptr_ptr = XNEW(asymbol*);
	*reloc->sym_ptr_ptr = symbol_get_bfdsym(fixp->fx_addsy);

	reloc->address = fixp->fx_frag->fr_address + fixp->fx_where;
	reloc->howto = bfd_reloc_type_lookup(stdoutput, fixp->fx_r_type);
	reloc->addend = fixp->fx_offset;

	return reloc;
}

/* no pc-relative relocations yet, might provide one later for experimetns */
long md_pcrel_from(fixS *fixp)
{
	return fixp->fx_frag->fr_address + fixp->fx_where;
}

/* all instructions and immediates are fixed in size, therefore
   no relaxation required */
int md_estimate_size_before_relax(fragS *fragp, asection *seg)
{
	(void) fragp;
	(void) seg;
	as_fatal(_("unexpected call"));
	return 0;
}

/* clean up */
#undef OPFUNCS
#undef ENDOPFUNCS
#undef FUNCX
#undef ENDFUNCX
#undef ENTRY
