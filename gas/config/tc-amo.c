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
#include <ctype.h>
#include "obstack.h"
#include "bfd.h"
#include "opcode/amo.h"

#define ENTRY(func, p) { emit_##func, p },
#define EMIT(name, optype, opname, ptype, pname) static void emit_##name (optype opname, ptype pname)

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
		void (*f)(unsigned char opcode, int type);
		/* operation parameter */
		int parameter;
	} operations[OPERATION_PER_INSTRUCTION_MAX];
} amo_opfunc_t;

struct litpool
{
	/* literal array */
	expressionS *literals;
	int index;
	int size;

	/* literal pool */
	symbolS *symbol;
	unsigned int id;
};

static struct amo_instruction insn;
static struct litpool literal_pool;

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

static void
literal_pool_init (void)
{
	literal_pool.literals = XNEWVEC (expressionS, LITERAL_POOL_SIZE_DEFAULT);
	literal_pool.size = LITERAL_POOL_SIZE_DEFAULT;
	literal_pool.index = 0;

	literal_pool.symbol = symbol_create (FAKE_LABEL_NAME, undefined_section, (valueT) 0, &zero_address_frag);
	literal_pool.id = 0;
}

static void
literal_pool_expand (void)
{
	expressionS *new_lit;

	new_lit = XNEWVEC (expressionS, literal_pool.size * 2);
	memcpy (new_lit, literal_pool.literals, literal_pool.size * sizeof (expressionS));
	XDELETEVEC (literal_pool.literals);
	literal_pool.literals = new_lit;
	literal_pool.size *= 2;
}

static void
literal_pool_constant (unsigned int bytes)
{
	/* get a slot */
	if (literal_pool.index == literal_pool.size)
		literal_pool_expand ();

	memset (&literal_pool.literals[literal_pool.index], 0, sizeof (expressionS));
	literal_pool.literals[literal_pool.index].X_op = O_constant;
	literal_pool.literals[literal_pool.index].X_add_number = bytes;
	literal_pool.index++;
}

static void
literal_pool_symbol (symbolS *symP, int where)
{
	/* get a slot */
	if (literal_pool.index == literal_pool.size)
		literal_pool_expand ();

	gas_assert (symP != 0);

	memset (&literal_pool.literals[literal_pool.index], 0, sizeof (expressionS));
	literal_pool.literals[literal_pool.index].X_op = O_symbol;
	literal_pool.literals[literal_pool.index].X_add_symbol = symP;
	literal_pool.literals[literal_pool.index].X_add_number = where;
	literal_pool.index++;
}

/* this function gets invoked once when assembler is initialized. */
void
md_begin (void)
{
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

	literal_pool_init ();
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
	e->X_add_number = 0;

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
		if (*name != '$')
		{
			as_bad ("invalid usage: '%s'", insn.name);
			return 0;
		}
		e->X_op = O_dereference_rel;
		e->X_add_number = expression_constant (name);
		return 1;
	}

	*mark = '\0';
	sym = hash_find (amo_reg_hash, name);
	if (!sym)
	{
		/* bad if there is no register at first */
		as_bad ("register must be placed first in dereference");
		return 0;
	}
	e->X_add_symbol = sym;
	if (*(mark + 1) != '$')
	{
		as_bad ("invalid usage: '%s'", insn.name);
		return 0;
	}
	e->X_add_number = expression_constant (mark + 1);
	return 1;
}

static int
parse_constant (char *name, expressionS *e)
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

EMIT(nop, unsigned char, opcode ATTRIBUTE_UNUSED, int, type ATTRIBUTE_UNUSED)
{
	char *frag;
	
	frag = frag_more (BYTES_PER_INSTRUCTION);

	md_number_to_chars (frag, 0x00000000, BYTES_PER_INSTRUCTION);
}

EMIT(arithmetic, unsigned char, opcode, int, type)
{
	unsigned long binary;
	char *frag;
	int imm;
	
	/* get a new frag */
	frag = frag_more (BYTES_PER_INSTRUCTION);
	binary = 0;

	binary |= (opcode << 26);
	binary |= ((insn.operands[1].X_add_number & MASK_REGISTER) << 21);
	if (type == TYPE_IMM)
	{
		imm = insn.operands[2].X_add_number;
		/* is this overflow ? */
		if (!(-32768 <= imm && imm <= 32767))
			as_bad ("16-bit immediate must be in the range -32768 to 32767.");
		binary |= ((insn.operands[0].X_add_number & MASK_REGISTER) << 16);
		binary |= imm & MASK_IMM16;
	}
	else if (type == TYPE_REG)
	{
		binary |= ((insn.operands[2].X_add_number & MASK_REGISTER) << 16);
		binary |= ((insn.operands[0].X_add_number & MASK_REGISTER) << 11);
	}
	else
	{
		/* impossible ! */
		abort ();
	}

	md_number_to_chars (frag, binary, BYTES_PER_INSTRUCTION);
}

EMIT(logical, unsigned char, opcode, int, type)
{
	unsigned long binary;
	char *frag;
	unsigned int imm;
	
	/* get a new frag */
	frag = frag_more (BYTES_PER_INSTRUCTION);
	binary = 0;

	binary |= (opcode << 26);
	binary |= ((insn.operands[1].X_add_number & MASK_REGISTER) << 21);
	if (type == TYPE_IMM)
	{
		imm = (unsigned int) insn.operands[2].X_add_number;
		/* is this overflow ? */
		if (!(imm <= 65535))
			as_bad ("16-bit immediate must be in the range 0 to 65535 in logical calculation.");
		binary |= ((insn.operands[0].X_add_number & MASK_REGISTER) << 16);
		binary |= imm & MASK_IMM16;
	}
	else if (type == TYPE_REG)
	{
		binary |= ((insn.operands[2].X_add_number & MASK_REGISTER) << 16);
		binary |= ((insn.operands[0].X_add_number & MASK_REGISTER) << 11);
	}
	else
	{
		/* impossible ! */
		abort ();
	}

	md_number_to_chars (frag, binary, BYTES_PER_INSTRUCTION);
}

EMIT(not, unsigned char, opcode, int, type)
{
	unsigned long binary;
	char *frag;
	unsigned int imm;
	
	/* get a new frag */
	frag = frag_more (BYTES_PER_INSTRUCTION);
	binary = 0;

	binary |= (opcode << 26);
	if (type == TYPE_IMM)
	{
		imm = (unsigned int) insn.operands[1].X_add_number;
		/* is this overflow ? */
		if (!(imm <= 65535))
			as_bad ("16-bit immediate must be in the range 0 to 65535 in logical calculation.");
		binary |= ((insn.operands[0].X_add_number & MASK_REGISTER) << 16);
		binary |= imm & MASK_IMM16;
	}
	else if (type == TYPE_REG)
	{
		binary |= ((insn.operands[1].X_add_number & MASK_REGISTER) << 16);
		binary |= ((insn.operands[0].X_add_number & MASK_REGISTER) << 11);
	}
	else
	{
		/* impossible ! */
		abort ();
	}

	md_number_to_chars (frag, binary, BYTES_PER_INSTRUCTION);
}

EMIT(mov, unsigned char, opcode, int, type)
{
	unsigned long binary;
	expressionS exp;
	fixS *fixP;
	char *frag;
	int imm;
	
	/* get a new frag */
	frag = frag_more (BYTES_PER_INSTRUCTION);
	binary = 0;

	binary |= (opcode << 26);
	binary |= ((insn.operands[0].X_add_number & MASK_REGISTER) << 21);
	if (type == TYPE_IMM)
	{
		imm = insn.operands[1].X_add_number;
		/* is this overflow ? */
		if (!(-1048576 <= imm && imm <= 1048575))
			goto use_load;
		binary |= imm & MASK_IMM21;
	}
	else if (type == TYPE_REG)
		binary |= ((insn.operands[1].X_add_number & MASK_REGISTER) << 16);
	else if (type == TYPE_SYM)
		goto use_load;
	else
		/* impossible ! */
		abort ();

	goto end;

use_load:
#define OPCODE_LDR 0b010110

	binary = OPCODE_LDR << 26;
	binary |= ((insn.operands[0].X_add_number & MASK_REGISTER) << 21);

	memset (&exp, 0, sizeof (expressionS));

	exp.X_op = O_symbol;
	exp.X_add_symbol = literal_pool.symbol;

	fixP = fix_new_exp (frag_now, frag - frag_now->fr_literal, BYTES_PER_INSTRUCTION, &exp, 1, BFD_RELOC_AMO_LITERAL);
	fixP->fx_offset = literal_pool.index * 4 - 4;

	if (type == TYPE_IMM)
		literal_pool_constant (insn.operands[1].X_add_number);
	else if (type == TYPE_SYM)
		literal_pool_symbol (insn.operands[1].X_add_symbol, frag - frag_now->fr_literal);

#undef OPCODE_LDR
end:
	md_number_to_chars (frag, binary, BYTES_PER_INSTRUCTION);
}

EMIT(ldr, unsigned char, opcode, int, type)
{
	unsigned long binary;
	char *frag;
	int imm;
	
	/* get a new frag */
	frag = frag_more (BYTES_PER_INSTRUCTION);
	binary = 0;
	
	binary |= (opcode << 26);
	if (type == TYPE_DREL)
	{
		binary |= ((insn.operands[0].X_add_number & MASK_REGISTER) << 21);
		imm = insn.operands[1].X_add_number;
		/* is this overflow ? */
		if (!(-1048576 <= imm && imm <= 1048575))
			as_bad ("21-bit immediate in ldr must be in the range -1048576 to 1048575.");
		binary |= imm & MASK_IMM21;
	}
	else if (type == TYPE_DEREF)
	{
		binary |= ((S_GET_VALUE (insn.operands[1].X_add_symbol) & MASK_REGISTER) << 21);
		binary |= ((insn.operands[0].X_add_number & MASK_REGISTER) << 16);
		imm = insn.operands[1].X_add_number;
		/* is this overflow ? */
		if (!(-32768 <= imm && imm <= 32767))
			as_bad ("16-bit immediate in ldr must be in the range -32768 to 32767.");
		binary |= imm & MASK_IMM16;
	}
	else
	{
		/* impossible ! */
		abort ();
	}

	md_number_to_chars (frag, binary, BYTES_PER_INSTRUCTION);
}

EMIT(str, unsigned char, opcode, int, type)
{
	unsigned long binary;
	char *frag;
	int imm;
	
	/* get a new frag */
	frag = frag_more (BYTES_PER_INSTRUCTION);
	binary = 0;
	
	binary |= (opcode << 26);
	if (type == TYPE_DREL)
	{
		binary |= ((insn.operands[1].X_add_number & MASK_REGISTER) << 21);
		imm = insn.operands[0].X_add_number;
		/* is this overflow ? */
		if (!(-1048576 <= imm && imm <= 1048575))
			as_bad ("21-bit immediate in ldr must be in the range -1048576 to 1048575.");
		binary |= imm & MASK_IMM21;
	}
	else if (type == TYPE_DEREF)
	{
		binary |= ((S_GET_VALUE (insn.operands[0].X_add_symbol) & MASK_REGISTER) << 21);
		binary |= ((insn.operands[1].X_add_number & MASK_REGISTER) << 16);
		imm = insn.operands[0].X_add_number;
		/* is this overflow ? */
		if (!(-32768 <= imm && imm <= 32767))
			as_bad ("16-bit immediate in ldr must be in the range -32768 to 32767.");
		binary |= imm & MASK_IMM16;
	}
	else
	{
		/* impossible ! */
		abort ();
	}

	md_number_to_chars (frag, binary, BYTES_PER_INSTRUCTION);
}

EMIT(branch, unsigned char, opcode, int, type ATTRIBUTE_UNUSED)
{
	unsigned long binary;
	fixS *fixP;
	char *frag;
	int where;
	
	/* get a new frag */
	frag = frag_more (BYTES_PER_INSTRUCTION);
	binary = 0;

	binary |= (opcode << 26);
	binary |= ((insn.operands[0].X_add_number & MASK_REGISTER) << 21);
	binary |= ((insn.operands[1].X_add_number & MASK_REGISTER) << 16);

	know (insn.operands[2].X_op = O_symbol);

	insn.operands[2].X_add_number = 0;
	where = frag - frag_now->fr_literal;

	/* need to check the overflow */

	fixP = fix_new_exp (frag_now, where, BYTES_PER_INSTRUCTION, &insn.operands[2], 1, BFD_RELOC_AMO_PCREL);
	fixP->fx_offset = -4;

	md_number_to_chars (frag, binary, BYTES_PER_INSTRUCTION);
}

EMIT(jump, unsigned char, opcode, int, type)
{
	unsigned long binary;
	char *frag;
	int where;
	
	/* get a new frag */
	frag = frag_more (BYTES_PER_INSTRUCTION);
	binary = 0;

	binary |= (opcode << 26);
	if (type == TYPE_SYM)
	{
		know (insn.operands[0].X_op = O_symbol);

		insn.operands[0].X_add_number = 0;
		where = frag - frag_now->fr_literal;

		/* need to check the overflow */

		fix_new_exp (frag_now, where, BYTES_PER_INSTRUCTION, &insn.operands[0], 0, BFD_RELOC_AMO_28);
	}
	else if (type == TYPE_REG)
	{
		binary |= ((insn.operands[0].X_add_number & MASK_REGISTER) << 21);
	}
	else
	{
		/* impossible ! */
		abort ();
	}

	md_number_to_chars (frag, binary, BYTES_PER_INSTRUCTION);
}

EMIT(swi, unsigned char, opcode, int, type ATTRIBUTE_UNUSED)
{
	unsigned long binary;
	char *frag;
	int imm;
	
	/* get a new frag */
	frag = frag_more (BYTES_PER_INSTRUCTION);
	binary = 0;

	binary |= (opcode << 26);

	imm = insn.operands[0].X_add_number;
	/* is this overflow ? */
	if (!(0 <= imm && imm <= 255))
		as_bad ("8-bit immediate in ldr must be in the range 0 to 255.");

	binary |= imm & MASK_IMM8;

	md_number_to_chars (frag, binary, BYTES_PER_INSTRUCTION);
}

OPFUNCS(amo)

FUNC(nop)
	ENTRY(nop, TYPE_NONE)
ENDFUNC
FUNC(add)
    ENTRY(arithmetic, TYPE_IMM)
	ENTRY(arithmetic, TYPE_REG)
ENDFUNC
FUNC(adc)
    ENTRY(arithmetic, TYPE_IMM)
	ENTRY(arithmetic, TYPE_REG)
ENDFUNC
FUNC(sub)
    ENTRY(arithmetic, TYPE_IMM)
	ENTRY(arithmetic, TYPE_REG)
ENDFUNC
FUNC(and)
    ENTRY(logical, TYPE_IMM)
	ENTRY(logical, TYPE_REG)
ENDFUNC
FUNC(or)
    ENTRY(logical, TYPE_IMM)
	ENTRY(logical, TYPE_REG)
ENDFUNC
FUNC(xor)
    ENTRY(logical, TYPE_IMM)
	ENTRY(logical, TYPE_REG)
ENDFUNC
FUNC(not)
    ENTRY(not, TYPE_IMM)
	ENTRY(not, TYPE_REG)
ENDFUNC
FUNC(lsl)
    ENTRY(logical, TYPE_IMM)
	ENTRY(logical, TYPE_REG)
ENDFUNC
FUNC(lsr)
    ENTRY(logical, TYPE_IMM)
	ENTRY(logical, TYPE_REG)
ENDFUNC
FUNC(asr)
    ENTRY(logical, TYPE_IMM)
	ENTRY(logical, TYPE_REG)
ENDFUNC
FUNC(mov)
    ENTRY(mov, TYPE_IMM)
	ENTRY(mov, TYPE_REG)
	ENTRY(mov, TYPE_SYM)
ENDFUNC
FUNC(ldr)
    ENTRY(ldr, TYPE_DREL)
	ENTRY(ldr, TYPE_DEREF)
ENDFUNC
FUNC(str)
    ENTRY(str, TYPE_DREL)
	ENTRY(str, TYPE_DEREF)
ENDFUNC
FUNC(beq)
    ENTRY(branch, TYPE_NONE)
ENDFUNC
FUNC(bne)
    ENTRY(branch, TYPE_NONE)
ENDFUNC
FUNC(blt)
    ENTRY(branch, TYPE_NONE)
ENDFUNC
FUNC(ble)
    ENTRY(branch, TYPE_NONE)
ENDFUNC
FUNC(jmp)
    ENTRY(jump, TYPE_SYM)
    ENTRY(jump, TYPE_REG)
ENDFUNC
FUNC(jal)
    ENTRY(jump, TYPE_SYM)
    ENTRY(jump, TYPE_REG)
ENDFUNC
FUNC(swi)
    ENTRY(swi, TYPE_NONE)
ENDFUNC
FUNC(bltu)
    ENTRY(branch, TYPE_NONE)
ENDFUNC
FUNC(bleu)
    ENTRY(branch, TYPE_NONE)
ENDFUNC
FUNC(ext)
    ENTRY(logical, TYPE_IMM)
ENDFUNC

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
md_assemble (char *str)
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

static void
pseudo_literals (int ignored ATTRIBUTE_UNUSED)
{
	unsigned long binary;
	char *preserved_name;
	char pool_name[LITERAL_POOL_NAME_LENGTH];
	int i, where;
	char *frag;

	if (literal_pool.index == 0)
		/* there is no need to build the literal pool */
		return ;

	/* align segment */
	record_alignment (now_seg, 2);

	/* get the preserved name */
	sprintf (pool_name, LITERAL_POOL_NAME_DEFAULT, literal_pool.id);
	obstack_grow (&notes, pool_name, strlen (pool_name) + 1);
	preserved_name = (char *) obstack_finish (&notes);

#ifdef tc_canonicalize_symbol_name
	preserved_name = tc_canonicalize_symbol_name (preserved_name);
#endif

	/* attribute */
	S_SET_NAME (literal_pool.symbol, preserved_name);
	S_SET_SEGMENT (literal_pool.symbol, now_seg);
	S_SET_VALUE (literal_pool.symbol, (valueT) frag_now_fix ());

	symbol_clear_list_pointers (literal_pool.symbol);
	symbol_set_frag (literal_pool.symbol, frag_now);

	/* link to end of symbol chain.  */
	{
		extern int symbol_table_frozen;

		if (symbol_table_frozen)
			abort ();
	}
	/* insert the symbol at fragment */
	symbol_append (literal_pool.symbol, symbol_lastP, &symbol_rootP, &symbol_lastP);

	obj_symbol_new_hook (literal_pool.symbol);
	symbol_table_insert (literal_pool.symbol);

	for (i = 0; i < literal_pool.index; i++)
	{
		frag = frag_more (BYTES_PER_INSTRUCTION);

		switch (literal_pool.literals[i].X_op)
		{
			case O_constant:
				binary = literal_pool.literals[i].X_add_number;
				break;

			case O_symbol:
				binary = 0;
				where = frag - frag_now->fr_literal;
				literal_pool.literals[i].X_add_number = 0;

				fix_new_exp (frag_now, where, BYTES_PER_INSTRUCTION, &literal_pool.literals[i], 0, BFD_RELOC_AMO_32);
				break;

			default:
				abort ();
		}

		md_number_to_chars (frag, binary, BYTES_PER_INSTRUCTION);
	}

	literal_pool.symbol = symbol_create (FAKE_LABEL_NAME, undefined_section, (valueT) 0, &zero_address_frag);
	literal_pool.index = 0;
	literal_pool.id++;
}

const pseudo_typeS md_pseudo_table[] =
{
	/* word should be 4-bytes */
    { "word", cons, 4 },
	{ "ltorg", pseudo_literals, 0 },
	{ NULL, (void *) NULL, 0 }
};

void
amo_md_end (void)
{
	pseudo_literals (0);
}

symbolS *
md_undefined_symbol (char *name ATTRIBUTE_UNUSED)
{
	/* handle a symbol as extern if assembler finds undefined symbol */
	return NULL;
}

/* no floating point expressions available */
const char *
md_atof (int type ATTRIBUTE_UNUSED, char *list ATTRIBUTE_UNUSED, int *size ATTRIBUTE_UNUSED)
{
	return NULL;
}

valueT
md_section_align (asection *seg, valueT size)
{
	int new_size;
	int align;

	align = bfd_get_section_alignment (stdoutput, seg);
	new_size = ((size + (1 << align) - 1) & -(1 << align));

	return new_size;
}

void
md_convert_frag (bfd *abfd ATTRIBUTE_UNUSED, asection *seg ATTRIBUTE_UNUSED, fragS *fragp ATTRIBUTE_UNUSED)
{
	as_fatal(_("unexpected call"));
}

void md_apply_fix (fixS *fixP, valueT *valP, segT seg ATTRIBUTE_UNUSED)
{
	char *buf;
	long val;
	
	val = * (long *) valP;
	buf = fixP->fx_frag->fr_literal + fixP->fx_where;

	switch (fixP->fx_r_type)
	{
		case BFD_RELOC_AMO_LITERAL:
			fixP->fx_no_overflow = (-1048576 <= val && val <= 1048575);
			if (!fixP->fx_no_overflow)
				as_bad_where (fixP->fx_file, fixP->fx_line, _("literal out of range"));
			if (fixP->fx_addsy)
			{
				fixP->fx_done = 0;
			}
			else
			{	
				*buf++ = val & MASK_IMM8;
				*buf++ = (val >> 8) & MASK_IMM8;
				*buf++ |= (val >> 16) & 0x1f;
				fixP->fx_done = 1;
			}
			break;	

		case BFD_RELOC_AMO_PCREL:
			fixP->fx_no_overflow = (-32768 <= val && val <= 32767);
			if (!fixP->fx_no_overflow)
				as_bad_where (fixP->fx_file, fixP->fx_line, _("relative jump out of range"));
			if (val & 0x3)
				as_bad_where (fixP->fx_file, fixP->fx_line, _("invalid address"));
			if (fixP->fx_addsy)
			{
				fixP->fx_done = 0;
			}
			else
			{	
				*buf++ = (val >> 2) & MASK_IMM8;
				*buf++ = (val >> 10) & MASK_IMM8;
				fixP->fx_done = 1;
			}
			break;

		case BFD_RELOC_AMO_28:
			fixP->fx_no_overflow = (-134217728 <= val && val <= 134217727);
			if (!fixP->fx_no_overflow)
				as_bad_where (fixP->fx_file, fixP->fx_line, _("absolute jump out of range"));
			if (val & 0x3)
				as_bad_where (fixP->fx_file, fixP->fx_line, _("invalid address"));
			break;
		
		case BFD_RELOC_AMO_32:
			if (val & 0x3)
				as_bad_where (fixP->fx_file, fixP->fx_line, _("invalid address"));
			break;

		default:
			/* something is wrong */
			abort ();
	}
}

arelent *
tc_gen_reloc (asection *seg ATTRIBUTE_UNUSED, fixS *fixP)
{
	arelent *reloc;

	gas_assert (fixP != 0);

	reloc = XNEW (arelent);
	reloc->sym_ptr_ptr = XNEW (asymbol *);
	*reloc->sym_ptr_ptr = symbol_get_bfdsym (fixP->fx_addsy);

	reloc->address = fixP->fx_frag->fr_address + fixP->fx_where;
	reloc->howto = bfd_reloc_type_lookup (stdoutput, fixP->fx_r_type);
	reloc->addend = fixP->fx_offset;

	return reloc;
}

long
md_pcrel_from (fixS *fixP)
{
	return fixP->fx_frag->fr_address + fixP->fx_where;
}

int md_estimate_size_before_relax (fragS *fragp ATTRIBUTE_UNUSED, asection *seg ATTRIBUTE_UNUSED)
{
	as_fatal (_("unexpected call"));
}

/* clean up */
#undef ENTRY
#undef EMIT
