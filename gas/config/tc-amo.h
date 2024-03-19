/* tc-amo.h -- Assembler for the AMO
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

#ifndef TC_AMO_H
#define TC_AMO_H

#define TARGET_FORMAT "elf32-amo"

/* use little endian */
#define TARGET_BYTES_BIG_ENDIAN 0

#define TARGET_ARCH bfd_arch_amo
#define TARGET_MACH bfd_mach_amo

/* customize operand function called in expression (expr) */
#define md_operand(x)
#define md_end amo_md_end

#define md_number_to_chars number_to_chars_littleendian
#define WORKING_DOT_WORD

/* we dont need indexing */
#define NEED_INDEX_OPERATOR 0
#define TC_EQUAL_IN_INSN(C, NAME) 1

/* it dosen't make sense to be able to calculate register numbers arithmetically */
#define md_register_arithmetic 0

#endif
