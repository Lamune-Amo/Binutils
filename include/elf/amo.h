#ifndef _ELF_AMO_H_
#define _ELF_AMO_H_

#include "elf/reloc-macros.h"

START_RELOC_NUMBERS (elf_amo_reloc_type)
    RELOC_NUMBER (R_AMO_NONE,       0)
    RELOC_NUMBER (R_AMO_32,         1)
    RELOC_NUMBER (R_AMO_RELATIVE,   2)
END_RELOC_NUMBERS (R_AMO_max)

#endif