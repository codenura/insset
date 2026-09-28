/*
 * baregear - A programming language compiler
 * Copyright (C) 2026 Abdullah Al Nahian Raiyan <abdullahal3829@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef COMINSSET_H
#define COMINSSET_H

#include <stdint.h>
#include <dynvar.h>

// LLVM supported architectures enum
typedef enum {
    x86,           // 32-bit x86
    x86_64,        // 64-bit x86 (AMD64/Intel64)
    i8086,
    arm,           // 32-bit ARM
    aarch64,       // 64-bit ARM (ARM64)
    mips,          // 32-bit MIPS
    mips64,        // 64-bit MIPS
    mipsel,        // 32-bit MIPS little-endian
    mips64el,      // 64-bit MIPS little-endian
    powerpc,       // 32-bit PowerPC
    powerpc64,     // 64-bit PowerPC
    powerpc64le,   // 64-bit PowerPC little-endian
    sparc,         // SPARC
    sparcv9,       // SPARC V9 (64-bit)
    systemz,       // IBM SystemZ (s390x)
    riscv,         // RISC-V 32-bit
    riscv64,       // RISC-V 64-bit
    wasm32,        // WebAssembly 32-bit
    wasm64,        // WebAssembly 64-bit
    avr,           // Atmel AVR 8-bit
    hexagon,       // Qualcomm Hexagon
    lanai,         // Lanai
    msp430,        // TI MSP430 16-bit
    nvptx,         // NVIDIA PTX
    amdgpu,        // AMD GPU
    bpf,           // Berkeley Packet Filter
    bpfeb,         // BPF big-endian
    bpfel,         // BPF little-endian
    xcore,         // XMOS XCore
    m68k,          // Motorola 68000
    ve,            // NEC SX-Aurora VE
    csky,          // C-SKY
    loongarch,     // LoongArch 32-bit
    loongarch64,   // LoongArch 64-bit
    xtensa,        // Tensilica Xtensa
    arc,           // ARC
    amdgcn,        // AMDGCN (AMD GPU Code Object)
    hip,           // HIP (AMD GPU)
    spir,          // SPIR (Standard Portable Intermediate Representation)
    spir64,        // SPIR 64-bit
    spirv,         // SPIR-V
    kalimba,       // Qualcomm Kalimba
    shaders,       // DirectX shaders
    renderscript,  // Android RenderScript
    unknown        // Unknown architecture
} LLVMArch;

typedef enum {
    PROC_SUCCESS,
    PROC_BUFFER_OVERFLOW,
    PROC_BUFFER_UNDERFLOW,
    PROC_INTEGER_OVERFLOW,
    // PROC_INTEGER_UNDERFLOW,
    INVALID_INSTRUCTION,
    INVALID_REGISTER,
    INVALID_ADDRESS,
    PROC_DIVISION_BY_ZERO,
    PRIVILEGED_INSTRUCTION,
    PAGE_FAULT,
    PROC_SEGMENTATION_FAULT,
    ALIGNMENT_ERROR,
    FLOATING_POINT_ERROR,
    INTERRUPT,
    HALT,
    PROC_TIMEOUT,
    INVALID_OPERAND,
    PROC_STACK_OVERFLOW,
    PROC_STACK_UNDERFLOW,
    PROTECTION_FAULT,
    GENERAL_PROTECTION_FAULT,
    PROC_UNKNOWN_ERROR,
    PROC_NOT_FOUND,
    NOT_INITIALIZED
} ProcessorResult;

typedef enum {
    NOP,
    MOV,
    ADD,
    SUB,
    MUL,
    DIV,
    MOD,
    INC,
    DEC,
    AND,
    OR,
    XOR,
    NOT,
    SHL,
    SHR,
    CMP,
    TEST,
    JMP,
    JE,
    JNE,
    JG,
    JGE,
    JL,
    JLE,
    JA,
    JAE,
    JB,
    JBE,
    JO,
    JNO,
    JS,
    JNS,
    CALL,
    RET,
    PUSH,
    POP,
    LEA,
    XCHG,
    IMUL,
    IDIV,
    NEG,
    ROL,
    ROR,
    RCL,
    RCR,
    SAR,
    SHLD,
    SHRD,
    LOOP,
    LOOPE,
    LOOPNE,
    INT,
    SYSCALL,
    SYSENTER,
    SYSEXIT,
    CLD,
    STD,
    CLTD,
    CQO,
    CBW,
    CWDE,
    CDQE,
    LEAVE,
    ENTER,
    LDS,
    LES,
    LFS,
    LGS,
    LSS,
    MOVSB,
    MOVSW,
    MOVSD,
    MOVSQ,
    LODSB,
    LODSW,
    LODSD,
    LODSQ,
    STOSB,
    STOSW,
    STOSD,
    STOSQ,
    SCASB,
    SCASW,
    SCASD,
    SCASQ,
    CMPSB,
    CMPSW,
    CMPSD,
    CMPSQ,
    REP,
    REPE,
    REPNE,
    LOCK,
    XADD,
    CMPXCHG,
    UNKNOWN_OP
} CommonOperator;

#ifdef __cplusplus
extern "C" {
#endif

extern uintptr_t registers;
extern int reglen;

extern void cinit(LLVMArch arch);
extern ProcessorResult copy(uintptr_t left, unsigned int loplen, uintptr_t right, unsigned int roplen,
                            vector eops, vector eopslen);
extern ProcessorResult add(uintptr_t left, unsigned int loplen, uintptr_t right, unsigned int roplen,
                    vector eops, vector eopslen);
extern ProcessorResult subtract(uintptr_t left, unsigned int loplen, uintptr_t right, unsigned int roplen,
                    vector eops, vector eopslen);
extern ProcessorResult multiply(uintptr_t left, unsigned int loplen, uintptr_t right, unsigned int roplen,
                    vector eops, vector eopslen);
extern ProcessorResult divide(uintptr_t left, unsigned int loplen, uintptr_t right, unsigned int roplen,
                    vector eops, vector eopslen);
extern ProcessorResult compare(uintptr_t left, unsigned int loplen, uintptr_t right, unsigned int roplen,
                              vector eops, vector eopslen);
extern ProcessorResult increment(uintptr_t left, unsigned int loplen);
extern ProcessorResult decrement(uintptr_t left, unsigned int loplen);
extern ProcessorResult lgAnd(uintptr_t left, unsigned int loplen, uintptr_t right, unsigned int roplen,
                            vector eops, vector eopslen);
extern ProcessorResult lgOr(uintptr_t left, unsigned int loplen, uintptr_t right, unsigned int roplen,
                           vector eops, vector eopslen);
extern ProcessorResult lgXor(uintptr_t left, unsigned int loplen, uintptr_t right, unsigned int roplen,
                           vector eops, vector eopslen);
extern ProcessorResult lgNot(uintptr_t left, unsigned int loplen, uintptr_t right, unsigned int roplen,
                           vector eops, vector eopslen);
extern ProcessorResult shift_left(uintptr_t left, unsigned int loplen, uintptr_t right, unsigned int roplen);
extern ProcessorResult shift_right(uintptr_t left, unsigned int loplen, uintptr_t right, unsigned int roplen);
extern ProcessorResult push(uintptr_t left, unsigned int loplen);
extern ProcessorResult pop(uintptr_t left, unsigned int loplen);
extern ProcessorResult load_effective_address(uintptr_t left, unsigned int loplen, uintptr_t right, unsigned int roplen);
extern ProcessorResult test_and(uintptr_t left, unsigned int loplen, uintptr_t right, unsigned int roplen,
                               vector eops, vector eopslen);
#ifdef __cplusplus
}
#endif

#endif // COMINSSET_H