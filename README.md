This repository contains a **non-official port**. Original notices are kept in `README.upstream/` and `COPYING*`.

[**English**](README.md) · [한국어](README_ko.md)

# AMO Binutils (port)

GNU binutils port for the **AMO architecture** (provides `as`, `ld`, `objdump`, `objcopy`, …).

Target triple: `amo-linux` (`amo-unknown-linux`)

### Target & ABI

**Word size**: 32-bit

**Endianness**: Little-endian

**Alignment:** word-aligned preferred; unaligned loads are **hardware-supported by the SoC** (one CPU cycle; handled by a 2-beat merge in the memory subsystem).

**Calling convention**:

| Register   | Special | Width  | Notes                                |
|------------|---------|--------|--------------------------------------|
| R0         |         | 32-bit | Caller-saved/Result register         |
| R1-R15     |         | 32-bit | Caller-saved registers               |
| R16-R25    |         | 32-bit | Callee-saved registers               |
| R26-R27    |         | 32-bit | Interrupt registers                  |
| R28        |         | 32-bit | Argument register                    |
| R29        | FP      | 32-bit | Frame pointer                        |
| R30        | SP      | 32-bit | Stack pointer                        |
| R31        | LR      | 32-bit | Link register                        |
| PC         |         | 32-bit | Program Counter                      |
| CPSR       |         | 4-bit  | Current Program Status Register      |
| IDTR       |         | 32-bit | Interrupt Descriptor Table Register  |

`PC`, `CPSR` are non-allocatable.

### Instruction Set Architecture

Please refer to [ISA.pdf](https://github.com/Lamune-Amo/Document/blob/master/ISA.pdf) for the detailed ISA specification.

## Relocations

| Reloc                  | Meaning                                     |
|------------------------|---------------------------------------------|
| `R_AMO_LITERAL`        | 21-bit PC-relative (literal pool)           |
| `R_AMO_PCREL`          | 16-bit PC-relative                          |
| `R_AMO_28`             | 28-bit Absolute                             |
| `R_AMO_32`             | 32-bit Absolute                             |

## Assembler

```asm
.global start
.extern kernel_stack
.section .inittext
; entry for kernel
start:
	; stack
	mov sp, kernel_stack
	add sp, sp, $4096
	mov fp, sp

	sub sp, sp, $4
	str [sp], lr

	; jump to c-based kernel
	jal kernel_init

	ldr lr, [sp]
	add sp, sp, $0x4

	; jump to bios section
	jmp lr

.ltorg
```

```
amo-linux-as boot.s -o boot.o
```

You can assemble the snippet above with `as` and inspect the output with `objdump` as shown below.

```
amo-linux-objdump -D boot.o
```

```
boot/boot.o:     file format elf32-amo


Disassembly of section .inittext:

00000000 <start>:
   0:	ldr  sp, [$0x20]                          20 00 c0 5b 	
   4:	add  sp, sp, $0x1000 (4096)               00 10 de 03 	
   8:	mov  fp, sp                               00 00 be 57 	
   c:	sub  sp, sp, $0x4 (4)                     04 00 de 13 	
  10:	str  [sp], lr                             00 00 df 67 	
  14:	jal  $0x0                                 00 00 00 80 	
  18:	ldr  lr, [sp]                             00 00 df 5f 	
  1c:	add  sp, sp, $0x4 (4)                     04 00 de 03 	
  20:	jmp  lr                                   00 00 e0 7f 	

00000024 <.__litpol_chunk_0_>:
  24:	add  r0, r0, $0x0 (0)                     00 00 00 00 	
```

## Linker

```
OUTPUT_FORMAT(elf32-amo)
OUTPUT_ARCH(amo)
ENTRY(start)

SECTIONS
{
	. = 0x2400;

	.text :
	{
		*(.inittext)
		*(.text)
	}
	.rodata :
	{
		*(.rodata)
	}
	.data :
	{
		*(.data)
	}
	.bss :
	{
		*(COMMON)
		*(.bss)
	}

	_kernel_end = .;
}
```

```
amo-linux-ld -T linker/setup.ld boot.o -o boot
```

Flexible linking is supported with `ld`.

## Build

```
configure --target=amo-linux --prefix=/path/install
make all -j8
make install
```
