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

#ifndef x86_64_INSSET
#define x86_64_INSSET

#include <definations.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

// x86-64 Register Structure
typedef struct x86_64_Registers {
    // General Purpose Registers (64-bit)
    uint64_t rax;
    uint64_t rbx;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t rbp;
    uint64_t rsp;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;

    // General Purpose Registers (32-bit) - lower 32 bits of above
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t esi;
    uint32_t edi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t r8d;
    uint32_t r9d;
    uint32_t r10d;
    uint32_t r11d;
    uint32_t r12d;
    uint32_t r13d;
    uint32_t r14d;
    uint32_t r15d;

    // General Purpose Registers (16-bit) - lower 16 bits of above
    uint16_t ax;
    uint16_t bx;
    uint16_t cx;
    uint16_t dx;
    uint16_t si;
    uint16_t di;
    uint16_t bp;
    uint16_t sp;
    uint16_t r8w;
    uint16_t r9w;
    uint16_t r10w;
    uint16_t r11w;
    uint16_t r12w;
    uint16_t r13w;
    uint16_t r14w;
    uint16_t r15w;

    // General Purpose Registers (8-bit)
    uint8_t al;
    uint8_t bl;
    uint8_t cl;
    uint8_t dl;
    uint8_t ah;
    uint8_t bh;
    uint8_t ch;
    uint8_t dh;
    uint8_t sil;
    uint8_t dil;
    uint8_t bpl;
    uint8_t spl;
    uint8_t r8b;
    uint8_t r9b;
    uint8_t r10b;
    uint8_t r11b;
    uint8_t r12b;
    uint8_t r13b;
    uint8_t r14b;
    uint8_t r15b;

    // Instruction Pointer
    uint64_t rip;
    uint32_t eip;

    // Flags Register
    uint64_t rflags;
    uint32_t eflags;

    // Segment Registers
    uint16_t cs;
    uint16_t ds;
    uint16_t es;
    uint16_t fs;
    uint16_t gs;
    uint16_t ss;

    // Control Registers
    uint64_t cr0;
    uint64_t cr2;
    uint64_t cr3;
    uint64_t cr4;
    uint64_t cr8;

    // Debug Registers
    uint64_t dr0;
    uint64_t dr1;
    uint64_t dr2;
    uint64_t dr3;
    uint64_t dr6;
    uint64_t dr7;

    // FPU Registers (80-bit floating point)
    struct {
        uint64_t mantissa;
        uint16_t exponent;
    } st[8];

    // FPU Control Word
    uint16_t fpu_cw;
    uint16_t fpu_sw;
    uint16_t fpu_tw;
    uint16_t fpu_ip;
    uint16_t fpu_dp;

    // MMX Registers (64-bit)
    uint64_t mm0;
    uint64_t mm1;
    uint64_t mm2;
    uint64_t mm3;
    uint64_t mm4;
    uint64_t mm5;
    uint64_t mm6;
    uint64_t mm7;

    // XMM Registers (128-bit for SSE/AVX)
    struct {
        uint64_t low;
        uint64_t high;
    } xmm[16];

    // YMM Registers (256-bit for AVX)
    struct {
        uint64_t q0;
        uint64_t q1;
        uint64_t q2;
        uint64_t q3;
    } ymm[16];

    // ZMM Registers (512-bit for AVX-512)
    struct {
        uint64_t q0;
        uint64_t q1;
        uint64_t q2;
        uint64_t q3;
        uint64_t q4;
        uint64_t q5;
        uint64_t q6;
        uint64_t q7;
    } zmm[32];

    // MXCSR (SSE Control and Status Register)
    uint32_t mxcsr;

    // FS and GS Base Addresses
    uint64_t fs_base;
    uint64_t gs_base;

    // Model Specific Registers (MSR) - common ones
    uint64_t msr_efer;
    uint64_t msr_star;
    uint64_t msr_lstar;
    uint64_t msr_cstar;
    uint64_t msr_sfmask;
    uint64_t msr_kernel_gs_base;
} x86_64_Registers;

void x86_64_setupRegister(x86_64_Registers* regs, uintptr_t addr, uintptr_t* op, unsigned int* oplen) {
    uintptr_t offset = addr - (uintptr_t)regs;

    if (offset >= offsetof(x86_64_Registers, rax) && offset < offsetof(x86_64_Registers, rbx)) *oplen = sizeof(regs->rax), *op = (uintptr_t)&regs->rax;
    else if (offset >= offsetof(x86_64_Registers, rbx) && offset < offsetof(x86_64_Registers, rcx)) *oplen = sizeof(regs->rbx), *op = (uintptr_t)&regs->rbx;
    else if (offset >= offsetof(x86_64_Registers, rcx) && offset < offsetof(x86_64_Registers, rdx)) *oplen = sizeof(regs->rcx), *op = (uintptr_t)&regs->rcx;
    else if (offset >= offsetof(x86_64_Registers, rdx) && offset < offsetof(x86_64_Registers, rsi)) *oplen = sizeof(regs->rdx), *op = (uintptr_t)&regs->rdx;
    else if (offset >= offsetof(x86_64_Registers, rsi) && offset < offsetof(x86_64_Registers, rdi)) *oplen = sizeof(regs->rsi), *op = (uintptr_t)&regs->rsi;
    else if (offset >= offsetof(x86_64_Registers, rdi) && offset < offsetof(x86_64_Registers, rbp)) *oplen = sizeof(regs->rdi), *op = (uintptr_t)&regs->rdi;
    else if (offset >= offsetof(x86_64_Registers, rbp) && offset < offsetof(x86_64_Registers, rsp)) *oplen = sizeof(regs->rbp), *op = (uintptr_t)&regs->rbp;
    else if (offset >= offsetof(x86_64_Registers, rsp) && offset < offsetof(x86_64_Registers, r8)) *oplen = sizeof(regs->rsp), *op = (uintptr_t)&regs->rsp;
    else if (offset >= offsetof(x86_64_Registers, r8) && offset < offsetof(x86_64_Registers, r9)) *oplen = sizeof(regs->r8), *op = (uintptr_t)&regs->r8;
    else if (offset >= offsetof(x86_64_Registers, r9) && offset < offsetof(x86_64_Registers, r10)) *oplen = sizeof(regs->r9), *op = (uintptr_t)&regs->r9;
    else if (offset >= offsetof(x86_64_Registers, r10) && offset < offsetof(x86_64_Registers, r11)) *oplen = sizeof(regs->r10), *op = (uintptr_t)&regs->r10;
    else if (offset >= offsetof(x86_64_Registers, r11) && offset < offsetof(x86_64_Registers, r12)) *oplen = sizeof(regs->r11), *op = (uintptr_t)&regs->r11;
    else if (offset >= offsetof(x86_64_Registers, r12) && offset < offsetof(x86_64_Registers, r13)) *oplen = sizeof(regs->r12), *op = (uintptr_t)&regs->r12;
    else if (offset >= offsetof(x86_64_Registers, r13) && offset < offsetof(x86_64_Registers, r14)) *oplen = sizeof(regs->r13), *op = (uintptr_t)&regs->r13;
    else if (offset >= offsetof(x86_64_Registers, r14) && offset < offsetof(x86_64_Registers, r15)) *oplen = sizeof(regs->r14), *op = (uintptr_t)&regs->r14;
    else if (offset >= offsetof(x86_64_Registers, r15) && offset < offsetof(x86_64_Registers, eax)) *oplen = sizeof(regs->r15), *op = (uintptr_t)&regs->r15;
    else if (offset >= offsetof(x86_64_Registers, eax) && offset < offsetof(x86_64_Registers, ebx)) *oplen = sizeof(regs->eax), *op = (uintptr_t)&regs->eax;
    else if (offset >= offsetof(x86_64_Registers, ebx) && offset < offsetof(x86_64_Registers, ecx)) *oplen = sizeof(regs->ebx), *op = (uintptr_t)&regs->ebx;
    else if (offset >= offsetof(x86_64_Registers, ecx) && offset < offsetof(x86_64_Registers, edx)) *oplen = sizeof(regs->ecx), *op = (uintptr_t)&regs->ecx;
    else if (offset >= offsetof(x86_64_Registers, edx) && offset < offsetof(x86_64_Registers, esi)) *oplen = sizeof(regs->edx), *op = (uintptr_t)&regs->edx;
    else if (offset >= offsetof(x86_64_Registers, esi) && offset < offsetof(x86_64_Registers, edi)) *oplen = sizeof(regs->esi), *op = (uintptr_t)&regs->esi;
    else if (offset >= offsetof(x86_64_Registers, edi) && offset < offsetof(x86_64_Registers, ebp)) *oplen = sizeof(regs->edi), *op = (uintptr_t)&regs->edi;
    else if (offset >= offsetof(x86_64_Registers, ebp) && offset < offsetof(x86_64_Registers, esp)) *oplen = sizeof(regs->ebp), *op = (uintptr_t)&regs->ebp;
    else if (offset >= offsetof(x86_64_Registers, esp) && offset < offsetof(x86_64_Registers, r8d)) *oplen = sizeof(regs->esp), *op = (uintptr_t)&regs->esp;
    else if (offset >= offsetof(x86_64_Registers, r8d) && offset < offsetof(x86_64_Registers, r9d)) *oplen = sizeof(regs->r8d), *op = (uintptr_t)&regs->r8d;
    else if (offset >= offsetof(x86_64_Registers, r9d) && offset < offsetof(x86_64_Registers, r10d)) *oplen = sizeof(regs->r9d), *op = (uintptr_t)&regs->r9d;
    else if (offset >= offsetof(x86_64_Registers, r10d) && offset < offsetof(x86_64_Registers, r11d)) *oplen = sizeof(regs->r10d), *op = (uintptr_t)&regs->r10d;
    else if (offset >= offsetof(x86_64_Registers, r11d) && offset < offsetof(x86_64_Registers, r12d)) *oplen = sizeof(regs->r11d), *op = (uintptr_t)&regs->r11d;
    else if (offset >= offsetof(x86_64_Registers, r12d) && offset < offsetof(x86_64_Registers, r13d)) *oplen = sizeof(regs->r12d), *op = (uintptr_t)&regs->r12d;
    else if (offset >= offsetof(x86_64_Registers, r13d) && offset < offsetof(x86_64_Registers, r14d)) *oplen = sizeof(regs->r13d), *op = (uintptr_t)&regs->r13d;
    else if (offset >= offsetof(x86_64_Registers, r14d) && offset < offsetof(x86_64_Registers, r15d)) *oplen = sizeof(regs->r14d), *op = (uintptr_t)&regs->r14d;
    else if (offset >= offsetof(x86_64_Registers, r15d) && offset < offsetof(x86_64_Registers, ax)) *oplen = sizeof(regs->r15d), *op = (uintptr_t)&regs->r15d;
    else if (offset >= offsetof(x86_64_Registers, ax) && offset < offsetof(x86_64_Registers, bx)) *oplen = sizeof(regs->ax), *op = (uintptr_t)&regs->ax;
    else if (offset >= offsetof(x86_64_Registers, bx) && offset < offsetof(x86_64_Registers, cx)) *oplen = sizeof(regs->bx), *op = (uintptr_t)&regs->bx;
    else if (offset >= offsetof(x86_64_Registers, cx) && offset < offsetof(x86_64_Registers, dx)) *oplen = sizeof(regs->cx), *op = (uintptr_t)&regs->cx;
    else if (offset >= offsetof(x86_64_Registers, dx) && offset < offsetof(x86_64_Registers, si)) *oplen = sizeof(regs->dx), *op = (uintptr_t)&regs->dx;
    else if (offset >= offsetof(x86_64_Registers, si) && offset < offsetof(x86_64_Registers, di)) *oplen = sizeof(regs->si), *op = (uintptr_t)&regs->si;
    else if (offset >= offsetof(x86_64_Registers, di) && offset < offsetof(x86_64_Registers, bp)) *oplen = sizeof(regs->di), *op = (uintptr_t)&regs->di;
    else if (offset >= offsetof(x86_64_Registers, bp) && offset < offsetof(x86_64_Registers, sp)) *oplen = sizeof(regs->bp), *op = (uintptr_t)&regs->bp;
    else if (offset >= offsetof(x86_64_Registers, sp) && offset < offsetof(x86_64_Registers, r8w)) *oplen = sizeof(regs->sp), *op = (uintptr_t)&regs->sp;
    else if (offset >= offsetof(x86_64_Registers, r8w) && offset < offsetof(x86_64_Registers, r9w)) *oplen = sizeof(regs->r8w), *op = (uintptr_t)&regs->r8w;
    else if (offset >= offsetof(x86_64_Registers, r9w) && offset < offsetof(x86_64_Registers, r10w)) *oplen = sizeof(regs->r9w), *op = (uintptr_t)&regs->r9w;
    else if (offset >= offsetof(x86_64_Registers, r10w) && offset < offsetof(x86_64_Registers, r11w)) *oplen = sizeof(regs->r10w), *op = (uintptr_t)&regs->r10w;
    else if (offset >= offsetof(x86_64_Registers, r11w) && offset < offsetof(x86_64_Registers, r12w)) *oplen = sizeof(regs->r11w), *op = (uintptr_t)&regs->r11w;
    else if (offset >= offsetof(x86_64_Registers, r12w) && offset < offsetof(x86_64_Registers, r13w)) *oplen = sizeof(regs->r12w), *op = (uintptr_t)&regs->r12w;
    else if (offset >= offsetof(x86_64_Registers, r13w) && offset < offsetof(x86_64_Registers, r14w)) *oplen = sizeof(regs->r13w), *op = (uintptr_t)&regs->r13w;
    else if (offset >= offsetof(x86_64_Registers, r14w) && offset < offsetof(x86_64_Registers, r15w)) *oplen = sizeof(regs->r14w), *op = (uintptr_t)&regs->r14w;
    else if (offset >= offsetof(x86_64_Registers, r15w) && offset < offsetof(x86_64_Registers, al)) *oplen = sizeof(regs->r15w), *op = (uintptr_t)&regs->r15w;
    else if (offset >= offsetof(x86_64_Registers, al) && offset < offsetof(x86_64_Registers, bl)) *oplen = sizeof(regs->al), *op = (uintptr_t)&regs->al;
    else if (offset >= offsetof(x86_64_Registers, bl) && offset < offsetof(x86_64_Registers, cl)) *oplen = sizeof(regs->bl), *op = (uintptr_t)&regs->bl;
    else if (offset >= offsetof(x86_64_Registers, cl) && offset < offsetof(x86_64_Registers, dl)) *oplen = sizeof(regs->cl), *op = (uintptr_t)&regs->cl;
    else if (offset >= offsetof(x86_64_Registers, dl) && offset < offsetof(x86_64_Registers, ah)) *oplen = sizeof(regs->dl), *op = (uintptr_t)&regs->dl;
    else if (offset >= offsetof(x86_64_Registers, ah) && offset < offsetof(x86_64_Registers, bh)) *oplen = sizeof(regs->ah), *op = (uintptr_t)&regs->ah;
    else if (offset >= offsetof(x86_64_Registers, bh) && offset < offsetof(x86_64_Registers, ch)) *oplen = sizeof(regs->bh), *op = (uintptr_t)&regs->bh;
    else if (offset >= offsetof(x86_64_Registers, ch) && offset < offsetof(x86_64_Registers, dh)) *oplen = sizeof(regs->ch), *op = (uintptr_t)&regs->ch;
    else if (offset >= offsetof(x86_64_Registers, dh) && offset < offsetof(x86_64_Registers, sil)) *oplen = sizeof(regs->dh), *op = (uintptr_t)&regs->dh;
    else if (offset >= offsetof(x86_64_Registers, sil) && offset < offsetof(x86_64_Registers, dil)) *oplen = sizeof(regs->sil), *op = (uintptr_t)&regs->sil;
    else if (offset >= offsetof(x86_64_Registers, dil) && offset < offsetof(x86_64_Registers, bpl)) *oplen = sizeof(regs->dil), *op = (uintptr_t)&regs->dil;
    else if (offset >= offsetof(x86_64_Registers, bpl) && offset < offsetof(x86_64_Registers, spl)) *oplen = sizeof(regs->bpl), *op = (uintptr_t)&regs->bpl;
    else if (offset >= offsetof(x86_64_Registers, spl) && offset < offsetof(x86_64_Registers, r8b)) *oplen = sizeof(regs->spl), *op = (uintptr_t)&regs->spl;
    else if (offset >= offsetof(x86_64_Registers, r8b) && offset < offsetof(x86_64_Registers, r9b)) *oplen = sizeof(regs->r8b), *op = (uintptr_t)&regs->r8b;
    else if (offset >= offsetof(x86_64_Registers, r9b) && offset < offsetof(x86_64_Registers, r10b)) *oplen = sizeof(regs->r9b), *op = (uintptr_t)&regs->r9b;
    else if (offset >= offsetof(x86_64_Registers, r10b) && offset < offsetof(x86_64_Registers, r11b)) *oplen = sizeof(regs->r10b), *op = (uintptr_t)&regs->r10b;
    else if (offset >= offsetof(x86_64_Registers, r11b) && offset < offsetof(x86_64_Registers, r12b)) *oplen = sizeof(regs->r11b), *op = (uintptr_t)&regs->r11b;
    else if (offset >= offsetof(x86_64_Registers, r12b) && offset < offsetof(x86_64_Registers, r13b)) *oplen = sizeof(regs->r12b), *op = (uintptr_t)&regs->r12b;
    else if (offset >= offsetof(x86_64_Registers, r13b) && offset < offsetof(x86_64_Registers, r14b)) *oplen = sizeof(regs->r13b), *op = (uintptr_t)&regs->r13b;
    else if (offset >= offsetof(x86_64_Registers, r14b) && offset < offsetof(x86_64_Registers, r15b)) *oplen = sizeof(regs->r14b), *op = (uintptr_t)&regs->r14b;
    else if (offset >= offsetof(x86_64_Registers, r15b) && offset < offsetof(x86_64_Registers, rip)) *oplen = sizeof(regs->r15b), *op = (uintptr_t)&regs->r15b;
    else if (offset >= offsetof(x86_64_Registers, rip) && offset < offsetof(x86_64_Registers, eip)) *oplen = sizeof(regs->rip), *op = (uintptr_t)&regs->rip;
    else if (offset >= offsetof(x86_64_Registers, eip) && offset < offsetof(x86_64_Registers, rflags)) *oplen = sizeof(regs->eip), *op = (uintptr_t)&regs->eip;
    else if (offset >= offsetof(x86_64_Registers, rflags) && offset < offsetof(x86_64_Registers, eflags)) *oplen = sizeof(regs->rflags), *op = (uintptr_t)&regs->rflags;
    else if (offset >= offsetof(x86_64_Registers, eflags) && offset < offsetof(x86_64_Registers, cs)) *oplen = sizeof(regs->eflags), *op = (uintptr_t)&regs->eflags;
    else if (offset >= offsetof(x86_64_Registers, cs) && offset < offsetof(x86_64_Registers, ds)) *oplen = sizeof(regs->cs), *op = (uintptr_t)&regs->cs;
    else if (offset >= offsetof(x86_64_Registers, ds) && offset < offsetof(x86_64_Registers, es)) *oplen = sizeof(regs->ds), *op = (uintptr_t)&regs->ds;
    else if (offset >= offsetof(x86_64_Registers, es) && offset < offsetof(x86_64_Registers, fs)) *oplen = sizeof(regs->es), *op = (uintptr_t)&regs->es;
    else if (offset >= offsetof(x86_64_Registers, fs) && offset < offsetof(x86_64_Registers, gs)) *oplen = sizeof(regs->fs), *op = (uintptr_t)&regs->fs;
    else if (offset >= offsetof(x86_64_Registers, gs) && offset < offsetof(x86_64_Registers, ss)) *oplen = sizeof(regs->gs), *op = (uintptr_t)&regs->gs;
    else if (offset >= offsetof(x86_64_Registers, ss) && offset < offsetof(x86_64_Registers, cr0)) *oplen = sizeof(regs->ss), *op = (uintptr_t)&regs->ss;
    else if (offset >= offsetof(x86_64_Registers, cr0) && offset < offsetof(x86_64_Registers, cr2)) *oplen = sizeof(regs->cr0), *op = (uintptr_t)&regs->cr0;
    else if (offset >= offsetof(x86_64_Registers, cr2) && offset < offsetof(x86_64_Registers, cr3)) *oplen = sizeof(regs->cr2), *op = (uintptr_t)&regs->cr2;
    else if (offset >= offsetof(x86_64_Registers, cr3) && offset < offsetof(x86_64_Registers, cr4)) *oplen = sizeof(regs->cr3), *op = (uintptr_t)&regs->cr3;
    else if (offset >= offsetof(x86_64_Registers, cr4) && offset < offsetof(x86_64_Registers, cr8)) *oplen = sizeof(regs->cr4), *op = (uintptr_t)&regs->cr4;
    else if (offset >= offsetof(x86_64_Registers, cr8) && offset < offsetof(x86_64_Registers, dr0)) *oplen = sizeof(regs->cr8), *op = (uintptr_t)&regs->cr8;
    else if (offset >= offsetof(x86_64_Registers, dr0) && offset < offsetof(x86_64_Registers, dr1)) *oplen = sizeof(regs->dr0), *op = (uintptr_t)&regs->dr0;
    else if (offset >= offsetof(x86_64_Registers, dr1) && offset < offsetof(x86_64_Registers, dr2)) *oplen = sizeof(regs->dr1), *op = (uintptr_t)&regs->dr1;
    else if (offset >= offsetof(x86_64_Registers, dr2) && offset < offsetof(x86_64_Registers, dr3)) *oplen = sizeof(regs->dr2), *op = (uintptr_t)&regs->dr2;
    else if (offset >= offsetof(x86_64_Registers, dr3) && offset < offsetof(x86_64_Registers, dr6)) *oplen = sizeof(regs->dr3), *op = (uintptr_t)&regs->dr3;
    else if (offset >= offsetof(x86_64_Registers, dr6) && offset < offsetof(x86_64_Registers, dr7)) *oplen = sizeof(regs->dr6), *op = (uintptr_t)&regs->dr6;
    else if (offset >= offsetof(x86_64_Registers, dr7) && offset < offsetof(x86_64_Registers, st)) *oplen = sizeof(regs->dr7), *op = (uintptr_t)&regs->dr7;
    else if (offset >= offsetof(x86_64_Registers, st) && offset < offsetof(x86_64_Registers, fpu_cw)) *oplen = sizeof(regs->st), *op = (uintptr_t)&regs->st;
    else if (offset >= offsetof(x86_64_Registers, fpu_cw) && offset < offsetof(x86_64_Registers, fpu_sw)) *oplen = sizeof(regs->fpu_cw), *op = (uintptr_t)&regs->fpu_cw;
    else if (offset >= offsetof(x86_64_Registers, fpu_sw) && offset < offsetof(x86_64_Registers, fpu_tw)) *oplen = sizeof(regs->fpu_sw), *op = (uintptr_t)&regs->fpu_sw;
    else if (offset >= offsetof(x86_64_Registers, fpu_tw) && offset < offsetof(x86_64_Registers, fpu_ip)) *oplen = sizeof(regs->fpu_tw), *op = (uintptr_t)&regs->fpu_tw;
    else if (offset >= offsetof(x86_64_Registers, fpu_ip) && offset < offsetof(x86_64_Registers, fpu_dp)) *oplen = sizeof(regs->fpu_ip), *op = (uintptr_t)&regs->fpu_ip;
    else if (offset >= offsetof(x86_64_Registers, fpu_dp) && offset < offsetof(x86_64_Registers, mm0)) *oplen = sizeof(regs->fpu_dp), *op = (uintptr_t)&regs->fpu_dp;
    else if (offset >= offsetof(x86_64_Registers, mm0) && offset < offsetof(x86_64_Registers, mm1)) *oplen = sizeof(regs->mm0), *op = (uintptr_t)&regs->mm0;
    else if (offset >= offsetof(x86_64_Registers, mm1) && offset < offsetof(x86_64_Registers, mm2)) *oplen = sizeof(regs->mm1), *op = (uintptr_t)&regs->mm1;
    else if (offset >= offsetof(x86_64_Registers, mm2) && offset < offsetof(x86_64_Registers, mm3)) *oplen = sizeof(regs->mm2), *op = (uintptr_t)&regs->mm2;
    else if (offset >= offsetof(x86_64_Registers, mm3) && offset < offsetof(x86_64_Registers, mm4)) *oplen = sizeof(regs->mm3), *op = (uintptr_t)&regs->mm3;
    else if (offset >= offsetof(x86_64_Registers, mm4) && offset < offsetof(x86_64_Registers, mm5)) *oplen = sizeof(regs->mm4), *op = (uintptr_t)&regs->mm4;
    else if (offset >= offsetof(x86_64_Registers, mm5) && offset < offsetof(x86_64_Registers, mm6)) *oplen = sizeof(regs->mm5), *op = (uintptr_t)&regs->mm5;
    else if (offset >= offsetof(x86_64_Registers, mm6) && offset < offsetof(x86_64_Registers, mm7)) *oplen = sizeof(regs->mm6), *op = (uintptr_t)&regs->mm6;
    else if (offset >= offsetof(x86_64_Registers, mm7) && offset < offsetof(x86_64_Registers, xmm)) *oplen = sizeof(regs->mm7), *op = (uintptr_t)&regs->mm7;
    else if (offset >= offsetof(x86_64_Registers, xmm) && offset < offsetof(x86_64_Registers, ymm)) *oplen = sizeof(regs->xmm), *op = (uintptr_t)&regs->xmm;
    else if (offset >= offsetof(x86_64_Registers, ymm) && offset < offsetof(x86_64_Registers, zmm)) *oplen = sizeof(regs->ymm), *op = (uintptr_t)&regs->ymm;
    else if (offset >= offsetof(x86_64_Registers, zmm) && offset < offsetof(x86_64_Registers, mxcsr)) *oplen = sizeof(regs->zmm), *op = (uintptr_t)&regs->zmm;
    else if (offset >= offsetof(x86_64_Registers, mxcsr) && offset < offsetof(x86_64_Registers, fs_base)) *oplen = sizeof(regs->mxcsr), *op = (uintptr_t)&regs->mxcsr;
    else if (offset >= offsetof(x86_64_Registers, fs_base) && offset < offsetof(x86_64_Registers, gs_base)) *oplen = sizeof(regs->fs_base), *op = (uintptr_t)&regs->fs_base;
    else if (offset >= offsetof(x86_64_Registers, gs_base) && offset < offsetof(x86_64_Registers, msr_efer)) *oplen = sizeof(regs->gs_base), *op = (uintptr_t)&regs->gs_base;
    else if (offset >= offsetof(x86_64_Registers, msr_efer) && offset < offsetof(x86_64_Registers, msr_star)) *oplen = sizeof(regs->msr_efer), *op = (uintptr_t)&regs->msr_efer;
    else if (offset >= offsetof(x86_64_Registers, msr_star) && offset < offsetof(x86_64_Registers, msr_lstar)) *oplen = sizeof(regs->msr_star), *op = (uintptr_t)&regs->msr_star;
    else if (offset >= offsetof(x86_64_Registers, msr_lstar) && offset < offsetof(x86_64_Registers, msr_cstar)) *oplen = sizeof(regs->msr_lstar), *op = (uintptr_t)&regs->msr_lstar;
    else if (offset >= offsetof(x86_64_Registers, msr_cstar) && offset < offsetof(x86_64_Registers, msr_sfmask)) *oplen = sizeof(regs->msr_cstar), *op = (uintptr_t)&regs->msr_cstar;
    else if (offset >= offsetof(x86_64_Registers, msr_sfmask) && offset < offsetof(x86_64_Registers, msr_kernel_gs_base)) *oplen = sizeof(regs->msr_sfmask), *op = (uintptr_t)&regs->msr_sfmask;
    else if (offset >= offsetof(x86_64_Registers, msr_kernel_gs_base)) *oplen = sizeof(regs->msr_kernel_gs_base), *op = (uintptr_t)&regs->msr_kernel_gs_base;
    else {
        char buf[64];
        snprintf(buf, sizeof(buf), "Invalid Register From 0x%lx", (uintptr_t)regs);
        bugDetected(buf);
    }
}

#endif // x86_64_INSSET