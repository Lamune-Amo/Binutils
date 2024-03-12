#ifndef _AMO_OPCODE_H_
#define _AMO_OPCODE_H_

/* number of bytes required for one instruction */
#define BYTES_PER_INSTRUCTION 4

/* max number of operands used for one instruction */
#define OPERAND_PER_INSTRUCTION_MAX 4

/* max number of operations that one instruction can perform */
#define OPERATION_PER_INSTRUCTION_MAX 4

/* total number of instructions */
#define INSTRUCTION_NUMBER_MAX 10

/* mnemonic */
#define MNEMONIC_LENGTH_MAX 10
#define DISASSEMBLE_DECODE_LENGTH 42

/* literal pool */
#define LITERAL_POOL_SIZE_DEFAULT 16
#define LITERAL_POOL_NAME_DEFAULT ".__litpol_chunk_%x_"
#define LITERAL_POOL_NAME_LENGTH 42

/* pseudo opcode */
/* these opcodes must be eight-bits and have 1 in their MSB */
#define PSEUDO_NOP 0b10000000
#define PSEUDO_MOV 0b10000001

/* system */
#define IS_BIG_ENDIAN (!(union { uint16_t u16; unsigned char u8; }){ .u16 = 1 }.u8)

#define OPCODESX(name) amo_opcode_t name##_opcodes[] = {
#define ENDOPCODESX };
#define OPX(mnemonic) { #mnemonic, {
#define ENDOPX } },

#define OPFUNCS(name) amo_opfunc_t name##_opfuncs[] = {
#define ENDOPFUNCS };
#define FUNC(mnemonic) { {
#define ENDFUNC } },

#define DECFUNCS(name) amo_decfunc_t name##_decfuncs[] = {
#define ENDDECFUNCS };
#define DECFUNC(mnemonic) { {
#define ENDDECFUNC } },

/* the instruction executor can receive the following macros as parameter */
#define TYPE_NONE -1
#define TYPE_IMM 0
#define TYPE_SYM 1
#define TYPE_REG 2
#define TYPE_DEREF 3
#define TYPE_DREL 4

/* mask */
#define MASK_REGISTER 0x1F
#define MASK_IMM8 0xFF
#define MASK_IMM16 0xFFFF
#define MASK_IMM21 0x1FFFFF
#define MASK_IMM26 0x3FFFFFF

/* used to define the dereference */
#define O_dereference O_md1
#define O_dereference_rel O_md2

typedef struct
{
	/* total number of operation */
	unsigned char number;

	/* each type (operatorT) */
	unsigned char types[OPERAND_PER_INSTRUCTION_MAX];
	
} operand_cond_t;

typedef struct
{
	/* mnemonic */
	const char *name;

	/* different operations represented by the same mnemonic */
	struct
	{	
		/* condition */
		operand_cond_t condition;
		/* opcode */
		unsigned char opcode;
	} operations[OPERATION_PER_INSTRUCTION_MAX];
} amo_opcode_t;

extern amo_opcode_t amo_opcodes[];
extern unsigned int amo_opcodes_size;

#endif
