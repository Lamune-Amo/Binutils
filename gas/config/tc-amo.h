/* tc-amo.h -- Assembler for the amo.
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

#ifndef TC_AMO_H
#define TC_AMO_H

#define TARGET_FORMAT "elf32-amo"

#define TARGET_BYTES_BIG_ENDIAN 0

#define TARGET_ARCH bfd_arch_amo
#define TARGET_MACH bfd_mach_amo

/* customizable handling of expression operands */
#define md_operand(x)

/* proper byte/char-wise emission of numeric data
   with respect to the endianness */
#define md_number_to_chars number_to_chars_littleendian
#define WORKING_DOT_WORD

#define NEED_INDEX_OPERATOR 1
#define TC_EQUAL_IN_INSN(C, NAME) 1
#define md_register_arithmetic 0
#define md_parse_name(str, expr, m, c) amo_parse_name(str, expr, c)

#define O_port O_md1
#define O_jump O_md2
#define O_jrel O_md3

#endif
