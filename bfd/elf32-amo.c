/* amo-specific support for 32-bit ELF.
   Copyright (C) 2000-2023 Free Software Foundation, Inc.

   This file is part of BFD, the Binary File Descriptor library.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

#include "sysdep.h"
#include "bfd.h"
#include "libbfd.h"
#include "elf-bfd.h"

#include "elf/amo.h"

/* relocation map structure */
struct amo_relocation_map
{
   bfd_reloc_code_real_type bfd_reloc_val;
   unsigned char elf_reloc_val;
};

/* actual relocation map */

static const struct amo_relocation_map amo_reloc_map[] =
{
   { BFD_RELOC_NONE,           R_AMO_NONE },
   { BFD_RELOC_32,             R_AMO_32 },
  // { BFD_RELOC_AMO_RELATIVE,   R_AMO_RELATIVE },
};

/* prototype for custom handling of relative relocation */
static bfd_reloc_status_type amo_elf_relative_reloc(bfd *abfd,
                                                    arelent *r_entry,
                                                    asymbol *sym,
                                                    void *data,
                                                    asection *input_sec,
                                                    bfd *output_bfd,
                                                    char **msg);

static reloc_howto_type amo_elf_howto_table[] =
{
   HOWTO (R_AMO_NONE,         /* type */
          0,                  /* rightshift */
          3,                  /* size (byte:0 short:1 long:2)*/
          0,                  /* bit size */
          FALSE,              /* pc-relative */
          0,                  /* bit pos */
          complain_overflow_dont, /* complain on overflow */
          bfd_elf_generic_reloc,  /* special function */
          "R_AMO_NONE",       /* amo */
          FALSE,              /* partial inplace */
          0,                  /* src mask */
          0,                  /* dst mask */
          FALSE               /* pc-relative offset */
   ),
   HOWTO (R_AMO_32,           /* type */
          0,                  /* rightshift */
          2,                  /* size (byte:0 short:1 long:2)*/
          32,                  /* bit size */
          FALSE,              /* pc-relative */
          0,                  /* bit pos */
          complain_overflow_bitfield, /* complain on overflow */
          bfd_elf_generic_reloc,  /* special function */
          "R_AMO_32",       /* amo */
          FALSE,              /* partial inplace */
          0,                  /* src mask */
          0xffffffff,                  /* dst mask */
          FALSE               /* pc-relative offset */
   ),
   HOWTO (R_AMO_RELATIVE,           /* type */
          0,                  /* rightshift */
          2,                  /* size (byte:0 short:1 long:2)*/
          32,                  /* bit size */
          TRUE,              /* pc-relative */
          0,                  /* bit pos */
          complain_overflow_bitfield, /* complain on overflow */
          bfd_elf_generic_reloc,  /* special function */
          "R_LM32_RELATIVE",       /* amo */
          FALSE,              /* partial inplace */
          0xffffffff,                  /* src mask */
          0xffffffff,                  /* dst mask */
          TRUE              /* pc-relative offset */
   )
};

/* without following functinos defined, ld will crash when performing relocations */
static bfd_boolean amo_info_to_howto_rela(bfd *abfd,
                                          arelent *cache_ptr,
                                          Elf_Internal_Rela *dst)
{
   unsigned int r_type = ELF32_R_TYPE(dst->r_info);

   if (r_type >= (unsigned int) R_AMO_max)
   {
      _bfd_error_handler(
         _("%p: unsupported relocation type %d"), abfd, r_type);
      
      bfd_set_error(bfd_error_bad_value);
      return FALSE;
   }

   cache_ptr->howto = &amo_elf_howto_table[r_type];
   return TRUE;
}

/* look up a howto by relocation's type */
static reloc_howto_type*
amo_reloc_type_lookup(bfd *abfd,
					  bfd_reloc_code_real_type code)
{
   unsigned int i;
   unsigned int howto_index;

   printf("looking up a howto by relocatoin's type: %d\n", code);

   for (i = 0; i < R_AMO_max; ++i)
   {
      if (code == amo_reloc_map[i].bfd_reloc_val)
      {
         howto_index = amo_reloc_map[i].elf_reloc_val;
         return &amo_elf_howto_table[howto_index];
      }
   }

	return 0;
}

/* look up howto by relocation's name */
static reloc_howto_type*
amo_reloc_name_lookup(bfd *abfd,
					  const char *name)
{
   unsigned int i;

   printf("looking up a howto by relocatoin's name: %d\n", name);

   for (i = 0; i < R_AMO_max; ++i)
   {
      if (amo_elf_howto_table[i].name != 0 && strcasecmp(amo_elf_howto_table[i].name, name) == 0)
      {
         return &amo_elf_howto_table[i];
      }
   }

   return 0;
}

static bfd_reloc_status_type amo_elf_relative_reloc(bfd *abfd,
                                                    arelent *r_entry,
                                                    asymbol *sym,
                                                    void *data,
                                                    asection *input_sec,
                                                    bfd *output_bfd,
                                                    char **msg)
{
   /* just redirect */
   return bfd_elf_generic_reloc(
      abfd, r_entry, sym, data, input_sec, output_bfd, msg
   );
}


#define elf_info_to_howto              amo_info_to_howto_rela
#define elf_info_to_howto_rel          NULL
#define bfd_elf32_bfd_reloc_type_lookup	amo_reloc_type_lookup
#define bfd_elf32_bfd_reloc_name_lookup amo_reloc_name_lookup

/* these things we are expected to provied */
#define ELF_ARCH			bfd_arch_amo
#define ELF_MAXPAGESIZE		0x1000
#define ELF_MACHINE_CODE	EM_AMO

#undef TARGET_LITTLE_SYM
#define TARGET_LITTLE_SYM	amo_elf32_vec

#undef TARGET_LITTLE_NAME
#define TARGET_LITTLE_NAME	"elf32-amo"

/* this header has to be included at the end */
#include "elf32-target.h"
