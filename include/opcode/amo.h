#ifndef _AMO_OPCODES_H_
#define _AMO_OPCODES_H_

#define AMO_NUM_REGISTERS 32

/* number of bytes required for one slot instruction */
#ifndef AMO_BYTES_SLOT_INSTRUCTION
#define AMO_BYTES_SLOT_INSTRUCTION 4
#endif

#ifndef AMO_BYTES_SLOT_IMMEDIATE
#define AMO_BYTES_SLOT_IMMEDIATE 4
#endif

/* we are going to use this when dealing with gas and objdump
   the value encode the actual machine opcodes, thus, if not dense
   list explicitly */
enum amo_opcode
{
	OP_NOP = 0,
	OP_JUMP = 1,

	/* to demonstrate a pc-relative relocation */
	OP_JUMP_REF = 2,

	OP_STORE = 17,
	OP_LOAD = 25,

	OP_MOV = 94,
	OP_ADD = 95
};

/* additionally and optionally, provide addressing mode enums */
enum amo_admode
{
	AM_01000 = 8,  /* reg = imm */
	AM_01001 = 9,  /* reg = op (reg) */
	AM_01010 = 10, /* reg = op (reg, imm) */
	AM_01011 = 11 /* reg = op (reg, reg) */
};

/* the structure of a single slot instruction, let's not bother with
   fancy stuff such as bundling and simd yet. again, record the 
   immediate value (if in use) in the struct, but emit separately
   from the instruction */
typedef struct
{
	int pad; /* immediate present, 1 bit */
	int opcode; /* opcode, occupies 7 bit */
	int admode; /* address mode, 6 bit */
	int src0; /* source 0 register, 6 bit */
	int src1; /* source 1 register, 6 bit */
	int dst; /* dest. register 6 bit */
	int immv; /* immediate, 32 bit only? */
} amo_slot_insn;

#endif
