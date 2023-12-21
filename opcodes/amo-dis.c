/* Disassembler interface for targets using CGEN. -*- C -*-
   CGEN: Cpu tools GENerator

   THIS FILE IS MACHINE GENERATED WITH CGEN.
   - the resultant file is machine generated, cgen-dis.in isn't

   Copyright (C) 1996-2023 Free Software Foundation, Inc.

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
   along with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA.  */

#include "sysdep.h"
#include "opcode/amo.h"
#include "disassemble.h"
#include "bfd.h"

#include <stdio.h>
#include <assert.h>

#define MAX_INSN_STRING_LENGTH 42
#define HEX_DATA_STRING_POS 46

static void amo_decode_insn(char insn_buf[AMO_BYTES_SLOT_INSTRUCTION],
                            amo_slot_insn *insn)
{
   assert(insn != 0);

   insn->dst = insn_buf[0] & 0x3f;
   insn->src1 = ((insn_buf[1] << 2) & 0x3c) | ((insn_buf[0] >> 6) & 0x3);
   insn->src0 = ((insn_buf[2] << 4) & 0x30) | ((insn_buf[1] >> 4) & 0xf);
   insn->admode = (insn_buf[2] >> 2) & 0x3f;
   insn->opcode = insn_buf[3] & 0x7f;
   insn->pad = (insn_buf[3] >> 7) & 0x1;
}

static void amo_decode_immv(char imm_buf[AMO_BYTES_SLOT_IMMEDIATE],
                            amo_slot_insn *insn)
{
   assert(4 == AMO_BYTES_SLOT_IMMEDIATE);

   int i;
   char *c = (char *)(&insn->immv);

   for (i = 0; i < AMO_BYTES_SLOT_IMMEDIATE; ++i)
   {
      *(c++) = imm_buf[i];
   }
}

static void amo_pad_string(char str[MAX_INSN_STRING_LENGTH],
                           const unsigned size)
{
   int i, len = strlen(str);

   if (len < size)
   {
      for (i = len; i < size; ++i)
      {
         str[i] = ' ';
      }

      str[size] = 0;
   }
}

static void amo_print_insn(amo_slot_insn *insn)
{
   assert(insn != 0);

   char insn_buf[MAX_INSN_STRING_LENGTH];

   switch (insn->opcode)
   {
      case OP_MOV:
      {
         if (AM_01001 == insn->admode)
         {
            sprintf(insn_buf, "r%d = r%d", insn->dst, insn->src0);
         }
         else if (AM_01000 == insn->admode)
         {
            sprintf(insn_buf, "r%d = r%d", insn->dst, insn->immv);
         }
         else sprintf(insn_buf, "Bad addressing mode for 'mov'");
         break;
      }

      case OP_ADD:
      {
         if (AM_01011 == insn->admode)
         {
            sprintf(insn_buf, "r%d = r%d + r%d", insn->dst, insn->src0, insn->src1);
         }
         else if (AM_01010 == insn->admode)
         {
            sprintf(insn_buf, "r%d = r%d + %d", insn->dst, insn->src0, insn->immv);
         }
         else sprintf(insn_buf, "Bad addressing mode for 'add'");
 
         break;
      }

      case OP_LOAD:
      {
         if (AM_01001 == insn->admode)
         {
            sprintf(insn_buf, "r%d = port32[r%d]", insn->dst, insn->src0);
         }
         else sprintf(insn_buf, "Bad addressing mode for 'load'");

         break;
      }

      case OP_STORE:
      {
         if (AM_01001 == insn->admode)
         {
            sprintf(insn_buf, "port32[r%d] = r%d", insn->src0, insn->dst);
         }
         else sprintf(insn_buf, "Bad addressing mode for 'store'");

         break;
      }

      case OP_JUMP:
      {
         if (AM_01001 == insn->admode)
         {
            sprintf(insn_buf, "jump (r%d)", insn->dst);
         }
         else if (AM_01000 == insn->admode)
         {
            sprintf(insn_buf, "jump (%d)", insn->immv);
         }
         else sprintf(insn_buf, "Bad addressing mode for 'jump'");
         break;
      }

      case OP_JUMP_REF:
      {
         if (AM_01001 == insn->admode)
         {
            sprintf(insn_buf, "jrel (r%d)", insn->dst);
         }
         else if (AM_01000 == insn->admode)
         {
            sprintf(insn_buf, "jrel (%d)", insn->immv);
         }
         else sprintf(insn_buf, "Bad addressing mode for 'jrel'");
         break;
      }

      default:
         sprintf(insn_buf, "Unsupported opcode %d", insn->opcode);
         break;
   }

   amo_pad_string(insn_buf, HEX_DATA_STRING_POS);
   printf("%s", insn_buf);
}

int print_insn_amo(bfd_vma addr, disassemble_info *info)
{
   int bytes_read = 0;

   char insn_buf[AMO_BYTES_SLOT_INSTRUCTION];
   char imm_buf[AMO_BYTES_SLOT_IMMEDIATE];

   amo_slot_insn insn;

   memset(&insn, 0, sizeof(amo_slot_insn));

   if ((*info->read_memory_func)(addr, insn_buf, AMO_BYTES_SLOT_INSTRUCTION, info))
   {
      (*info->fprintf_func)(info->stream, "Error: attept to read more instruction bytes (%d) than available (%d)\n",
      AMO_BYTES_SLOT_INSTRUCTION, info->buffer_length);
      return -1;
   }

   bytes_read = AMO_BYTES_SLOT_INSTRUCTION;

   amo_decode_insn(insn_buf, &insn);

   if (insn.pad)
   {
      addr += AMO_BYTES_SLOT_INSTRUCTION;

      if ((*info->read_memory_func)(addr, imm_buf, AMO_BYTES_SLOT_IMMEDIATE, info))
      {
         (*info->fprintf_func)(info->stream, "Error: attept to read more immeditate bytes (%d) than available (%d)\n",
         AMO_BYTES_SLOT_INSTRUCTION, info->buffer_length);
         return -1;
      }
      amo_decode_immv(imm_buf, &insn);

      bytes_read += AMO_BYTES_SLOT_IMMEDIATE;
   }

   amo_print_insn(&insn);

   return bytes_read;
}