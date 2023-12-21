/* tc-amo.c -- Assembler for the amo.
   Copyright (C) 2000-2023 Free Software Foundation, Inc.

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
   along with GAS; see the file COPYING.  If not, write to
   the Free Software Foundation, 51 Franklin Street - Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* absolutely minimal assembler setup, things that are required to exist */

#include "as.h"
#include "bfd.h"
#include "opcode/amo.h"

const char *md_shortopts = "";
struct option md_longopts[] = {};
size_t md_longopts_size = sizeof(md_longopts);

/* comment characters and separators */
const char comment_chars[] = "";
const char line_comment_chars[] = "//";
const char line_separator_chars[] = ";";

/* no floating point */
const char EXP_CHARS[] = "";
const char FLT_CHARS[] = "";

/* no pseudo ops yet */
const pseudo_typeS md_pseudo_table[] =
{
	{ (char *) NULL, (void(*)(int)) NULL, 0 }
};

struct hash_control *reg_table;

/* cant parse if there are no opts */
int md_parse_option(int c, const char *arg)
{
	(void) c;
	(void) arg;
	return 0;
}

/* command line usage */
void md_show_usage(FILE *stream)
{
	fprintf(stream, "\namo options: none available yet\n");
}

/*****************************************************************************/
/* See if the specified name is a register (r0-r63). Nothing actually to	 */
/* parse here, just look up in the register table							 */
/*****************************************************************************/

static int amo_parse_register(const char *name, expressionS *resultP)
{
	symbolS *reg_sym = hash_find(reg_table, name);

	if (reg_sym != 0)
	{
		resultP->X_op = O_register;
		resultP->X_add_number = S_GET_VALUE(reg_sym);
		return 1;
	}
	return 0;
}

static int amo_parse_port(const char *name,
				   expressionS *resultP)
{
	if (0 == strcmp(name, "port32"))
	{
		resultP->X_op_symbol = 0;
		resultP->X_add_symbol = 0;
		resultP->X_op = O_port;
		return 1;
	}
	return 0;
}

static int amo_parse_branch(const char *name,
					 expressionS *resultP,
					 char *next_char)
{
	expressionS op_expr;

	int seen_parenthesis = 0;
	int is_jump = !strcmp(name, "jump");
	int is_jrel = !strcmp(name, "jrel");

	if (!(is_jump || is_jrel))
	{
		return 0;
	}

	(void) restore_line_pointer(*next_char);
	SKIP_WHITESPACE();

	if ('(' == *input_line_pointer)
	{
		seen_parenthesis = 1;
		input_line_pointer++;
	}
	else
	{
		as_warn("missing '(' in 'jump/jrel' instruction syntax");
	}
	
	memset(&op_expr, 0, sizeof(expressionS));
	expression(&op_expr);

	resultP->X_add_symbol = make_expr_symbol(&op_expr);
	resultP->X_op = is_jump ? O_jump : O_jrel;

	if (1 == seen_parenthesis)
	{
		if (')' != *input_line_pointer)
		{
			as_bad("missing ')' in 'jump/jrel' instruction syntax");
		}
		else
			++input_line_pointer;
	}

	if (*input_line_pointer != '\0')
	{
		as_bad("unexpected '%c' in 'jump/jrel' instruction syntax", *input_line_pointer);
	}

	/* need to mark next_char properly otherwise input_line_pointer 
	   will be reset to the position right after 'jump/jrel' */
	*next_char = *input_line_pointer;
	return 1;
}

static int amo_parse_symbol(const char *name,
				     expressionS *resultP)
{
	symbolS *sym = symbol_find_or_make(name);
	know(sym != 0);

	resultP->X_op = O_symbol;
	resultP->X_add_symbol = sym;

	resultP->X_add_number = 1;
	return 1;
}

/*****************************************************************************/
/* In order to catch assembgler keywords and assign them properly, hook in a */
/* custom name checking routine												 */
/*****************************************************************************/

int amo_parse_name(const char *name,
				   expressionS *resultP,
				   char *next_char)
{
	gas_assert(name != 0 && resultP != 0);

	if (amo_parse_register(name, resultP) != 0)
		return 1;
	if (amo_parse_port(name, resultP) != 0)
		return 1;
	if (amo_parse_branch(name, resultP, next_char) != 0)
		return 1;
	if (amo_parse_symbol(name, resultP) != 0)
		return 1;
	return 0;
}

/*custom initializer hook, called once per assembler invokation */
void md_begin(void)
{
	int i;

	reg_table = hash_new();

	for (i = 0; i < AMO_NUM_REGISTERS; ++i)
	{
		char name[5];

		sprintf(name, "r%d", i);

		symbolS *reg_sym =
			symbol_new(name, reg_section, i, &zero_address_frag);

		if (hash_insert(reg_table, S_GET_NAME(reg_sym), (PTR) reg_sym))
		{
			as_fatal(_("failed creating register symbols"));
		}
	}
}

static bfd_boolean amo_build_insn(amo_slot_insn *insn,
								  expressionS *lhs,
								  expressionS *rhs)
{
	gas_assert(lhs != 0 && insn != 0);

	switch(lhs->X_op)
	{
		case O_register:
		{
			gas_assert(rhs != 0);

			insn->dst = lhs->X_add_number;

			if (O_register == rhs->X_op)
			{
				insn->opcode = OP_MOV;
				insn->admode = AM_01001;
				insn->src0 = rhs->X_add_number;
				return TRUE;
			}
			else if (O_constant == rhs->X_op)
			{
				insn->opcode = OP_MOV;
				insn->admode = AM_01000;
				insn->pad = 1;
				insn->immv = rhs->X_add_number;
				return TRUE;
			}
			else if (O_add == rhs->X_op)
			{
				insn->opcode = OP_ADD;
				gas_assert(rhs->X_add_symbol != 0 && rhs->X_op_symbol != 0);

				expressionS *op1_expr = symbol_get_value_expression(rhs->X_add_symbol);
				expressionS *op2_expr = symbol_get_value_expression(rhs->X_op_symbol);

				if (op1_expr->X_op != O_register)
				{
					as_bad("operand expected to be a register");
					return FALSE;
				}

				insn->src0 = op1_expr->X_add_number;
				if (O_register == op2_expr->X_op)
				{
					insn->admode = AM_01011;
					insn->src1 = op2_expr->X_add_number;
					return TRUE;
				}
				else if (O_constant == op2_expr->X_op)
				{
					insn->admode = AM_01010;
					insn->immv = op2_expr->X_add_number;
					insn->pad = 1;
					return TRUE;
				}

				as_bad("unexpected operand type for 'add'");
			}
			else if (O_index == rhs->X_op)
			{
				expressionS *index_expr = symbol_get_value_expression(rhs->X_op_symbol);

				if (O_register == index_expr->X_op)
				{
					insn->opcode = OP_LOAD;
					insn->admode = AM_01001;
					insn->src0 = index_expr->X_add_number;
					return TRUE;
				}

				as_bad("unexpacted operand type for 'load'");
			}

			break;
		}

		case O_index:
		{
			if (O_register == rhs->X_op)
			{
				insn->dst = rhs->X_add_number;
				expressionS *index_expr = symbol_get_value_expression(lhs->X_op_symbol);

				if (O_register == index_expr->X_op)
				{
					insn->opcode = OP_STORE;
					insn->admode = AM_01001;
					insn->src0 = index_expr->X_add_number;
					return TRUE;
				}
			}
			as_bad("unexpacted operand type for 'store'");
			break;
		}

		case O_jump:
		case O_jrel:
		{
			gas_assert (lhs->X_add_symbol != 0);

			insn->opcode = O_jump == lhs->X_op ? OP_JUMP : OP_JUMP_REF;

			expressionS *target_expr = symbol_get_value_expression(lhs->X_add_symbol);

			if (O_register == target_expr->X_op)
			{
				insn->admode = AM_01001;
				insn->dst = target_expr->X_add_number;
				return TRUE;
			}
			else if (O_symbol == target_expr->X_op)
			{
				insn->admode = AM_01000;
				insn->pad = 1;
				return TRUE;
			}
			as_bad ("unexpected operand type for 'jump/jrel'");
			break;
		}
		
		default: break;
	}

	return FALSE;
}

static void amo_emit_insn(amo_slot_insn *insn,
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
}

/* main assembler hook, called once for each "instruction string" */
void md_assemble(char *insn_str)
{
	printf("Assembling instruction string: %s\n", insn_str);

	amo_slot_insn *insn = 0;
	expressionS lhs_info, rhs_info;
	int insn_done = 0;
	int assign_pos;
	char *assign;

	assign = strchr(insn_str, '=');
	input_line_pointer = insn_str;

	memset(&lhs_info, 0, sizeof(expressionS));
	expression(&lhs_info);

	insn = (amo_slot_insn *)malloc(sizeof(amo_slot_insn));

	if (assign)
	{
		assign_pos = assign - insn_str;
		input_line_pointer = ++assign;

		memset(&rhs_info, 0, sizeof(expressionS));
		expression(&rhs_info);

		insn_done = amo_build_insn(insn, &lhs_info, &rhs_info);
	}
	else
	{
		insn_done = amo_build_insn(insn, &lhs_info, 0);
	}

	if (!insn_done)
	{
		as_fatal(_("could not build instruction"));
	}
	else
	{
		amo_emit_insn(insn, &lhs_info, &rhs_info);
		printf(" written instruction\n");
	}
}

/* this gets invoked in case 'md_parse_name' has failed */
symbolS *md_undefined_symbol (char *name)
{
	as_bad("undefined symbol '%s'\n", name);
	return 0;
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
		if (BFD_RELOC_AMO_RELATIVE == fixp->fx_r_type)
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
