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

#ifndef x86_INSSET
#define x86_INSSET

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

// x86 (32-bit) Register Structure
typedef struct x86_Registers {
    // General Purpose Registers (32-bit)
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t esi;
    uint32_t edi;
    uint32_t ebp;
    uint32_t esp;

    // General Purpose Registers (16-bit) - lower 16 bits of above
    uint16_t ax;
    uint16_t bx;
    uint16_t cx;
    uint16_t dx;
    uint16_t si;
    uint16_t di;
    uint16_t bp;
    uint16_t sp;

    // General Purpose Registers (8-bit)
    uint8_t al;
    uint8_t bl;
    uint8_t cl;
    uint8_t dl;
    uint8_t ah;
    uint8_t bh;
    uint8_t ch;
    uint8_t dh;

    // Instruction Pointer
    uint32_t eip;

    // Flags Register
    uint32_t eflags;

    // Segment Registers
    uint16_t cs;
    uint16_t ds;
    uint16_t es;
    uint16_t fs;
    uint16_t gs;
    uint16_t ss;

    // Control Registers
    uint32_t cr0;
    uint32_t cr2;
    uint32_t cr3;
    uint32_t cr4;

    // Debug Registers
    uint32_t dr0;
    uint32_t dr1;
    uint32_t dr2;
    uint32_t dr3;
    uint32_t dr6;
    uint32_t dr7;

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

    // XMM Registers (128-bit for SSE/AVX) - 8 registers on 32-bit
    struct {
        uint64_t low;
        uint64_t high;
    } xmm[8];

    // MXCSR (SSE Control and Status Register)
    uint32_t mxcsr;

    // FS and GS Base Addresses (available in some modes)
    uint32_t fs_base;
    uint32_t gs_base;

    // Model Specific Registers (MSR) - common ones
    uint64_t msr_efer;
    uint64_t msr_star;
    uint64_t msr_lstar;
    uint64_t msr_cstar;
    uint64_t msr_sfmask;

} x86_Registers;

void x86_setupRegister(x86_Registers* regs, uintptr_t addr, uintptr_t* op, unsigned int* oplen) {
    uintptr_t offset = addr - (uintptr_t)regs;

    if (offset >= offsetof(x86_Registers, eax) && offset < offsetof(x86_Registers, ebx)) *oplen = sizeof(regs->eax), *op = (uintptr_t)&regs->eax;
    else if (offset >= offsetof(x86_Registers, ebx) && offset < offsetof(x86_Registers, ecx)) *oplen = sizeof(regs->ebx), *op = (uintptr_t)&regs->ebx;
    else if (offset >= offsetof(x86_Registers, ecx) && offset < offsetof(x86_Registers, edx)) *oplen = sizeof(regs->ecx), *op = (uintptr_t)&regs->ecx;
    else if (offset >= offsetof(x86_Registers, edx) && offset < offsetof(x86_Registers, esi)) *oplen = sizeof(regs->edx), *op = (uintptr_t)&regs->edx;
    else if (offset >= offsetof(x86_Registers, esi) && offset < offsetof(x86_Registers, edi)) *oplen = sizeof(regs->esi), *op = (uintptr_t)&regs->esi;
    else if (offset >= offsetof(x86_Registers, edi) && offset < offsetof(x86_Registers, ebp)) *oplen = sizeof(regs->edi), *op = (uintptr_t)&regs->edi;
    else if (offset >= offsetof(x86_Registers, ebp) && offset < offsetof(x86_Registers, esp)) *oplen = sizeof(regs->ebp), *op = (uintptr_t)&regs->ebp;
    else if (offset >= offsetof(x86_Registers, esp) && offset < offsetof(x86_Registers, ax)) *oplen = sizeof(regs->esp), *op = (uintptr_t)&regs->esp;
    else if (offset >= offsetof(x86_Registers, ax) && offset < offsetof(x86_Registers, bx)) *oplen = sizeof(regs->ax), *op = (uintptr_t)&regs->ax;
    else if (offset >= offsetof(x86_Registers, bx) && offset < offsetof(x86_Registers, cx)) *oplen = sizeof(regs->bx), *op = (uintptr_t)&regs->bx;
    else if (offset >= offsetof(x86_Registers, cx) && offset < offsetof(x86_Registers, dx)) *oplen = sizeof(regs->cx), *op = (uintptr_t)&regs->cx;
    else if (offset >= offsetof(x86_Registers, dx) && offset < offsetof(x86_Registers, si)) *oplen = sizeof(regs->dx), *op = (uintptr_t)&regs->dx;
    else if (offset >= offsetof(x86_Registers, si) && offset < offsetof(x86_Registers, di)) *oplen = sizeof(regs->si), *op = (uintptr_t)&regs->si;
    else if (offset >= offsetof(x86_Registers, di) && offset < offsetof(x86_Registers, bp)) *oplen = sizeof(regs->di), *op = (uintptr_t)&regs->di;
    else if (offset >= offsetof(x86_Registers, bp) && offset < offsetof(x86_Registers, sp)) *oplen = sizeof(regs->bp), *op = (uintptr_t)&regs->bp;
    else if (offset >= offsetof(x86_Registers, sp) && offset < offsetof(x86_Registers, al)) *oplen = sizeof(regs->sp), *op = (uintptr_t)&regs->sp;
    else if (offset >= offsetof(x86_Registers, al) && offset < offsetof(x86_Registers, bl)) *oplen = sizeof(regs->al), *op = (uintptr_t)&regs->al;
    else if (offset >= offsetof(x86_Registers, bl) && offset < offsetof(x86_Registers, cl)) *oplen = sizeof(regs->bl), *op = (uintptr_t)&regs->bl;
    else if (offset >= offsetof(x86_Registers, cl) && offset < offsetof(x86_Registers, dl)) *oplen = sizeof(regs->cl), *op = (uintptr_t)&regs->cl;
    else if (offset >= offsetof(x86_Registers, dl) && offset < offsetof(x86_Registers, ah)) *oplen = sizeof(regs->dl), *op = (uintptr_t)&regs->dl;
    else if (offset >= offsetof(x86_Registers, ah) && offset < offsetof(x86_Registers, bh)) *oplen = sizeof(regs->ah), *op = (uintptr_t)&regs->ah;
    else if (offset >= offsetof(x86_Registers, bh) && offset < offsetof(x86_Registers, ch)) *oplen = sizeof(regs->bh), *op = (uintptr_t)&regs->bh;
    else if (offset >= offsetof(x86_Registers, ch) && offset < offsetof(x86_Registers, dh)) *oplen = sizeof(regs->ch), *op = (uintptr_t)&regs->ch;
    else if (offset >= offsetof(x86_Registers, dh) && offset < offsetof(x86_Registers, eip)) *oplen = sizeof(regs->dh), *op = (uintptr_t)&regs->dh;
    else if (offset >= offsetof(x86_Registers, eip) && offset < offsetof(x86_Registers, eflags)) *oplen = sizeof(regs->eip), *op = (uintptr_t)&regs->eip;
    else if (offset >= offsetof(x86_Registers, eflags) && offset < offsetof(x86_Registers, cs)) *oplen = sizeof(regs->eflags), *op = (uintptr_t)&regs->eflags;
    else if (offset >= offsetof(x86_Registers, cs) && offset < offsetof(x86_Registers, ds)) *oplen = sizeof(regs->cs), *op = (uintptr_t)&regs->cs;
    else if (offset >= offsetof(x86_Registers, ds) && offset < offsetof(x86_Registers, es)) *oplen = sizeof(regs->ds), *op = (uintptr_t)&regs->ds;
    else if (offset >= offsetof(x86_Registers, es) && offset < offsetof(x86_Registers, fs)) *oplen = sizeof(regs->es), *op = (uintptr_t)&regs->es;
    else if (offset >= offsetof(x86_Registers, fs) && offset < offsetof(x86_Registers, gs)) *oplen = sizeof(regs->fs), *op = (uintptr_t)&regs->fs;
    else if (offset >= offsetof(x86_Registers, gs) && offset < offsetof(x86_Registers, ss)) *oplen = sizeof(regs->gs), *op = (uintptr_t)&regs->gs;
    else if (offset >= offsetof(x86_Registers, ss) && offset < offsetof(x86_Registers, cr0)) *oplen = sizeof(regs->ss), *op = (uintptr_t)&regs->ss;
    else if (offset >= offsetof(x86_Registers, cr0) && offset < offsetof(x86_Registers, cr2)) *oplen = sizeof(regs->cr0), *op = (uintptr_t)&regs->cr0;
    else if (offset >= offsetof(x86_Registers, cr2) && offset < offsetof(x86_Registers, cr3)) *oplen = sizeof(regs->cr2), *op = (uintptr_t)&regs->cr2;
    else if (offset >= offsetof(x86_Registers, cr3) && offset < offsetof(x86_Registers, cr4)) *oplen = sizeof(regs->cr3), *op = (uintptr_t)&regs->cr3;
    else if (offset >= offsetof(x86_Registers, cr4) && offset < offsetof(x86_Registers, dr0)) *oplen = sizeof(regs->cr4), *op = (uintptr_t)&regs->cr4;
    else if (offset >= offsetof(x86_Registers, dr0) && offset < offsetof(x86_Registers, dr1)) *oplen = sizeof(regs->dr0), *op = (uintptr_t)&regs->dr0;
    else if (offset >= offsetof(x86_Registers, dr1) && offset < offsetof(x86_Registers, dr2)) *oplen = sizeof(regs->dr1), *op = (uintptr_t)&regs->dr1;
    else if (offset >= offsetof(x86_Registers, dr2) && offset < offsetof(x86_Registers, dr3)) *oplen = sizeof(regs->dr2), *op = (uintptr_t)&regs->dr2;
    else if (offset >= offsetof(x86_Registers, dr3) && offset < offsetof(x86_Registers, dr6)) *oplen = sizeof(regs->dr3), *op = (uintptr_t)&regs->dr3;
    else if (offset >= offsetof(x86_Registers, dr6) && offset < offsetof(x86_Registers, dr7)) *oplen = sizeof(regs->dr6), *op = (uintptr_t)&regs->dr6;
    else if (offset >= offsetof(x86_Registers, dr7) && offset < offsetof(x86_Registers, st)) *oplen = sizeof(regs->dr7), *op = (uintptr_t)&regs->dr7;
    else if (offset >= offsetof(x86_Registers, st) && offset < offsetof(x86_Registers, fpu_cw)) *oplen = sizeof(regs->st), *op = (uintptr_t)&regs->st;
    else if (offset >= offsetof(x86_Registers, fpu_cw) && offset < offsetof(x86_Registers, fpu_sw)) *oplen = sizeof(regs->fpu_cw), *op = (uintptr_t)&regs->fpu_cw;
    else if (offset >= offsetof(x86_Registers, fpu_sw) && offset < offsetof(x86_Registers, fpu_tw)) *oplen = sizeof(regs->fpu_sw), *op = (uintptr_t)&regs->fpu_sw;
    else if (offset >= offsetof(x86_Registers, fpu_tw) && offset < offsetof(x86_Registers, fpu_ip)) *oplen = sizeof(regs->fpu_tw), *op = (uintptr_t)&regs->fpu_tw;
    else if (offset >= offsetof(x86_Registers, fpu_ip) && offset < offsetof(x86_Registers, fpu_dp)) *oplen = sizeof(regs->fpu_ip), *op = (uintptr_t)&regs->fpu_ip;
    else if (offset >= offsetof(x86_Registers, fpu_dp) && offset < offsetof(x86_Registers, mm0)) *oplen = sizeof(regs->fpu_dp), *op = (uintptr_t)&regs->fpu_dp;
    else if (offset >= offsetof(x86_Registers, mm0) && offset < offsetof(x86_Registers, mm1)) *oplen = sizeof(regs->mm0), *op = (uintptr_t)&regs->mm0;
    else if (offset >= offsetof(x86_Registers, mm1) && offset < offsetof(x86_Registers, mm2)) *oplen = sizeof(regs->mm1), *op = (uintptr_t)&regs->mm1;
    else if (offset >= offsetof(x86_Registers, mm2) && offset < offsetof(x86_Registers, mm3)) *oplen = sizeof(regs->mm2), *op = (uintptr_t)&regs->mm2;
    else if (offset >= offsetof(x86_Registers, mm3) && offset < offsetof(x86_Registers, mm4)) *oplen = sizeof(regs->mm3), *op = (uintptr_t)&regs->mm3;
    else if (offset >= offsetof(x86_Registers, mm4) && offset < offsetof(x86_Registers, mm5)) *oplen = sizeof(regs->mm4), *op = (uintptr_t)&regs->mm4;
    else if (offset >= offsetof(x86_Registers, mm5) && offset < offsetof(x86_Registers, mm6)) *oplen = sizeof(regs->mm5), *op = (uintptr_t)&regs->mm5;
    else if (offset >= offsetof(x86_Registers, mm6) && offset < offsetof(x86_Registers, mm7)) *oplen = sizeof(regs->mm6), *op = (uintptr_t)&regs->mm6;
    else if (offset >= offsetof(x86_Registers, mm7) && offset < offsetof(x86_Registers, xmm)) *oplen = sizeof(regs->mm7), *op = (uintptr_t)&regs->mm7;
    else if (offset >= offsetof(x86_Registers, xmm) && offset < offsetof(x86_Registers, mxcsr)) *oplen = sizeof(regs->xmm), *op = (uintptr_t)&regs->xmm;
    else if (offset >= offsetof(x86_Registers, mxcsr) && offset < offsetof(x86_Registers, fs_base)) *oplen = sizeof(regs->mxcsr), *op = (uintptr_t)&regs->mxcsr;
    else if (offset >= offsetof(x86_Registers, fs_base) && offset < offsetof(x86_Registers, gs_base)) *oplen = sizeof(regs->fs_base), *op = (uintptr_t)&regs->fs_base;
    else if (offset >= offsetof(x86_Registers, gs_base) && offset < offsetof(x86_Registers, msr_efer)) *oplen = sizeof(regs->gs_base), *op = (uintptr_t)&regs->gs_base;
    else if (offset >= offsetof(x86_Registers, msr_efer) && offset < offsetof(x86_Registers, msr_star)) *oplen = sizeof(regs->msr_efer), *op = (uintptr_t)&regs->msr_efer;
    else if (offset >= offsetof(x86_Registers, msr_star) && offset < offsetof(x86_Registers, msr_lstar)) *oplen = sizeof(regs->msr_star), *op = (uintptr_t)&regs->msr_star;
    else if (offset >= offsetof(x86_Registers, msr_lstar) && offset < offsetof(x86_Registers, msr_cstar)) *oplen = sizeof(regs->msr_lstar), *op = (uintptr_t)&regs->msr_lstar;
    else if (offset >= offsetof(x86_Registers, msr_cstar) && offset < offsetof(x86_Registers, msr_sfmask)) *oplen = sizeof(regs->msr_cstar), *op = (uintptr_t)&regs->msr_cstar;
    else if (offset >= offsetof(x86_Registers, msr_sfmask)) *oplen = sizeof(regs->msr_sfmask), *op = (uintptr_t)&regs->msr_sfmask;
    else {
        char buf[64];
        snprintf(buf, sizeof(buf), "Invalid Register From 0x%lx", (uintptr_t)regs);
        bugDetected(buf);
    }
}

#endif // x86_INSSET