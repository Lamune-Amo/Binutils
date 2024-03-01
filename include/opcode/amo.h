#ifndef _AMO_OPCODES_H_
#define _AMO_OPCODES_H_

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

/* mask */
#define MASK_REGISTER 0x1F
#define MASK_IMM16 0xFFFF
#define MASK_IMM21 0x1FFFFF

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
		/* operation function */
		void (*f)(unsigned char opcode, int param);
		/* opcode */
		unsigned char opcode;
		/* operation parameter */
		int parameter;
	} operations[OPERATION_PER_INSTRUCTION_MAX];
} table_t;

extern table_t instab[];

/* the instruction executor can receive the following macros as parameter */
#define INSN_TYPE_IMMEDIATE 0
#define INSN_TYPE_REGISTER 1

/* used to define the dereference */
#define O_dereference O_md1

#endif
