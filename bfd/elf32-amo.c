/* amo-specific support for 32-bit ELF.
   Copyright (C) 2000-2024 Free Software Foundation, Inc.

   Author: Yechan Hong <yechan0815@naver.com>

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

struct amo_relocation_map
{
	bfd_reloc_code_real_type bfd_reloc_val;
	unsigned char elf_reloc_val;
};

static const struct amo_relocation_map amo_reloc_map[] =
{
	{ BFD_RELOC_NONE, R_AMO_NONE },
	{ BFD_RELOC_AMO_LITERAL, R_AMO_LITERAL },
	{ BFD_RELOC_AMO_PCREL, R_AMO_PCREL },
	{ BFD_RELOC_AMO_28, R_AMO_28 },
	{ BFD_RELOC_AMO_32, R_AMO_32 },
    { BFD_RELOC_32, R_AMO_32 }
};

static bfd_reloc_status_type
amo_elf_relative_reloc (bfd *abfd, arelent *r_entry, asymbol *sym,
                        void *data, asection *input_sec, bfd *output_bfd,
                        char **msg);

static reloc_howto_type amo_elf_howto_table[] =
{
	/* This reloc does nothing. */
	HOWTO (R_AMO_NONE,         /* type */
		   0,                  /* right shift */
           3,                  /* size (0 = byte, 1 = short, 2 = long) */
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
    /* A 21-bit pc-relative relocation. */
	HOWTO (R_AMO_LITERAL,      /* type */
           0,                  /* right shift */
           2,                  /* size (0 = byte, 1 = short, 2 = long) */
           21,                 /* bit size */
           TRUE,               /* pc-relative */
           0,				   /* bit pos */
           complain_overflow_bitfield, /* complain on overflow */
           bfd_elf_generic_reloc, /* special function */
           "R_AMO_LITERAL",    /* amo */
           FALSE,              /* partial inplace */
           0x001fffff,         /* src mask */
           0x001fffff,         /* dst mask */
           TRUE                /* pc-relative offset */
	),
    /* A 16-bit pc-relative relocation. */
	HOWTO (R_AMO_PCREL,        /* type */
           2,                  /* right shift */
           1,                  /* size (0 = byte, 1 = short, 2 = long) */
           16,                 /* bit size */
           TRUE,               /* pc-relative */
           0,				   /* bit pos */
           complain_overflow_bitfield, /* complain on overflow */
           bfd_elf_generic_reloc, /* special function */
           "R_AMO_PCREL",      /* amo */
           FALSE,              /* partial inplace */
           0x0000ffff,         /* src mask */
           0x0000ffff,         /* dst mask */
           TRUE                /* pc-relative offset */
	),
	/* A 28-bit absolute relocation. */
	HOWTO (R_AMO_28,           /* type */
           2,                  /* right shift */
           2,                  /* size (0 = byte, 1 = short, 2 = long) */
           26,                 /* bit size */
           FALSE,              /* pc-relative */
           0,                  /* bit pos */
           complain_overflow_bitfield, /* complain on overflow */
           bfd_elf_generic_reloc, /* special function */
           "R_AMO_28",         /* amo */
           FALSE,              /* partial inplace */
           0x03ffffff,         /* src mask */
           0x03ffffff,         /* dst mask */
           FALSE               /* pc-relative offset */
	),
    /* A 32-bit absolute relocation. */
	HOWTO (R_AMO_32,           /* type */
           0,                  /* right shift */
           2,                  /* size (0 = byte, 1 = short, 2 = long) */
           32,                 /* bit size */
           FALSE,              /* pc-relative */
           0,                  /* bit pos */
           complain_overflow_bitfield, /* complain on overflow */
           bfd_elf_generic_reloc, /* special function */
           "R_AMO_32",         /* amo */
           FALSE,              /* partial inplace */
           0xffffffff,         /* src mask */
           0xffffffff,         /* dst mask */
           FALSE               /* pc-relative offset */
	)
};

static bfd_boolean
amo_info_to_howto_rela (bfd *abfd, arelent *cache_ptr, Elf_Internal_Rela *dst)
{
	unsigned int r_type = ELF32_R_TYPE(dst->r_info);

	if (r_type >= (unsigned int) R_AMO_max)
	{
		_bfd_error_handler(_("%p: unsupported relocation type %d"), abfd, r_type);
		bfd_set_error(bfd_error_bad_value);

		return FALSE;
	}
	
	cache_ptr->howto = &amo_elf_howto_table[r_type];
	return TRUE;
}

static reloc_howto_type *
amo_reloc_type_lookup (bfd *abfd, bfd_reloc_code_real_type code)
{
	unsigned int i;

	for (i = 0; i < R_AMO_max; ++i)
	{
		if (amo_reloc_map[i].bfd_reloc_val == code)
			return &amo_elf_howto_table[amo_reloc_map[i].elf_reloc_val];
	}
	
	return NULL;
}

static reloc_howto_type *
amo_reloc_name_lookup (bfd *abfd, const char *name)
{
	unsigned int i;
	
	for (i = 0; i < R_AMO_max; ++i)
	{
		if (amo_elf_howto_table[i].name && !strcasecmp(amo_elf_howto_table[i].name, name))
		{
			return &amo_elf_howto_table[i];
		}
	}

	return NULL;
}

static bfd_reloc_status_type
amo_elf_relative_reloc(bfd *abfd, arelent *r_entry, asymbol *sym,
                       void *data, asection *input_sec, bfd *output_bfd,
                       char **msg)
{
	/* redirect */
	return bfd_elf_generic_reloc(
		abfd, r_entry, sym, data, input_sec, output_bfd, msg
	);
}

#define ELF_ARCH bfd_arch_amo
#define ELF_MACHINE_CODE EM_AMO

#define ELF_MAXPAGESIZE 0x1000

#undef TARGET_LITTLE_SYM
#define TARGET_LITTLE_SYM amo_elf32_vec

#undef TARGET_LITTLE_NAME
#define TARGET_LITTLE_NAME "elf32-amo"

#define elf_info_to_howto amo_info_to_howto_rela
#define elf_info_to_howto_rel NULL
#define bfd_elf32_bfd_reloc_type_lookup	amo_reloc_type_lookup
#define bfd_elf32_bfd_reloc_name_lookup amo_reloc_name_lookup

/* this header has to be included at the end */
#include "elf32-target.h"
