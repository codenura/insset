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

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <analyzer.h>
#include <definations.h>
#include <dynvar.h>
#include <runtime.h>

#include <insset/cominsset.h>
#include <insset/x86_64.h>
#include <insset/x86.h>

/*
 * Agent: Codex
 * LLM: GPT-6
 */

bool isInit = false;
extern void expect(dynvar);
LLVMArch carch;
vector pushedItems;

typedef struct {
    uintptr_t address;
    vector backups;
} PushedItem;

extern vector simval;
extern vector memRegions;
void cinit(LLVMArch arch) {
    if (isInit)
        return;

    vectorInit(&pushedItems, sizeof(PushedItem));
    carch = arch;
    dynvar registers = {0, 0};
    if (arch == x86_64) {
        registers.address = (uintptr_t)falloc(NULL, sizeof(x86_64_Registers));
        registers.length = sizeof(x86_64_Registers);
    } else if (arch == x86) {
        registers.address = (uintptr_t)falloc(NULL, sizeof(x86_Registers));
        registers.length = sizeof(x86_Registers);
    }
    ValueAddressPair vap;
    vap.addr = (uintptr_t)0x00;
    vap.value = registers;
    lgr valueBuffer = {0};
    memcpy(valueBuffer, &vap, sizeof(vap));
    vectorAppend(&simval, valueBuffer);

    if (registers.address == 0) {
        fprintf(stderr, "Not Enough Memory Found, For %lu Bytes.\n", (unsigned long)registers.length);
        exit(1);
    }
    isInit = true;
}

ProcessorResult copy(uintptr_t left, unsigned int loplen, uintptr_t right, unsigned int roplen,
                     vector eops, vector eopslen) {
    if (!isInit)
        return NOT_INITIALIZED;

    if (((uintptr_t)left < (uintptr_t)simval.address) || ((uintptr_t)right < (uintptr_t)simval.address))
        return PROC_BUFFER_UNDERFLOW;
    else if (((uintptr_t)left > (uintptr_t)(simval.count * simval.sizePerBlks)) ||
             ((uintptr_t)right > (uintptr_t)(simval.count * simval.sizePerBlks)))
        return PROC_BUFFER_OVERFLOW;

    int valLen = 0;
    if (carch == x86_64 || carch == x86) {
        lgr bufr = {0};
        ValueAddressPair vap = {0};
        vectorGetValue(&simval, 0, &bufr);
        memcpy(&vap, bufr, sizeof(ValueAddressPair));
        valLen = vap.value.length;
        getValue(vap.value, &bufr);
        if (carch == x86_64) {
            x86_64_Registers* regs = (x86_64_Registers*)bufr;
            if (left >= (uintptr_t)registers && left <= ((uintptr_t)registers + sizeof(x86_64_Registers)))
                x86_64_setupRegister(regs, left, &left, &loplen);
            if (right >= (uintptr_t)registers && right <= ((uintptr_t)registers + sizeof(x86_64_Registers)))
                x86_64_setupRegister(regs, right, &right, &roplen);
        } else if (carch == x86) {
            x86_Registers* regs = (x86_Registers*)bufr;
            if (left >= (uintptr_t)registers && left <= ((uintptr_t)registers + sizeof(x86_Registers)))
                x86_setupRegister(regs, left, &left, &loplen);
            if (right >= (uintptr_t)registers && right <= ((uintptr_t)registers + sizeof(x86_Registers)))
                x86_setupRegister(regs, right, &right, &roplen);
        }
        if (loplen < roplen)
            return PROC_BUFFER_OVERFLOW;

        memset((void*)bufr, 0, valLen - 1);
        for (int i = 0; i < (memRegions.count - 1); i++) {
            vectorGetValue(&memRegions, i, &bufr);
            MemoryRegion mreg;
            memcpy(&mreg, bufr, sizeof(MemoryRegion));
            if (mreg.address == left) {
                if ((mreg.type != VARIABLE) || (mreg.type != USEABLE_SPACE))
                    return GENERAL_PROTECTION_FAULT;

                if (roplen > (mreg.address + mreg.length - left))
                    return PROC_BUFFER_OVERFLOW;

                memcpy((void*)left, (void*)right, roplen);
                memset((void*)left, roplen + 1, loplen);
                return PROC_SUCCESS;
            }
        }
        return INVALID_OPERAND;
    } else
        bugDetected("Unknown Archtecture");
    
    return PROC_SUCCESS;
}

ProcessorResult add(uintptr_t left, unsigned int loplen, uintptr_t right, unsigned int roplen,
                    vector eops, vector eopslen) {
    if (!isInit)
        return NOT_INITIALIZED;
    
    if (carch == x86_64) {
        lgr bufr;
        vectorGetValue(&simval, 0, &bufr);
        x86_64_Registers* regs = (x86_64_Registers*)bufr;
        if (left >= (uintptr_t)registers && left <= ((uintptr_t)registers + sizeof(x86_64_Registers)))
            x86_64_setupRegister(regs, left, &left, &loplen);
        if (right >= (uintptr_t)registers && right <= ((uintptr_t)registers + sizeof(x86_64_Registers)))
            x86_64_setupRegister(regs, right, &right, &roplen);
        
        // Perform addition based on operand size
        uint64_t result = 0;
        uint64_t src_val = 0;
        uint64_t dest_val = 0;
        
        if (loplen == 1) {
            uint8_t* d = (uint8_t*)left;
            uint8_t* s = (uint8_t*)right;
            dest_val = *d;
            src_val = *s;
            *d += *s;
            result = *d;
        } else if (loplen == 2) {
            uint16_t* d = (uint16_t*)left;
            uint16_t* s = (uint16_t*)right;
            dest_val = *d;
            src_val = *s;
            *d += *s;
            result = *d;
        } else if (loplen == 4) {
            uint32_t* d = (uint32_t*)left;
            uint32_t* s = (uint32_t*)right;
            dest_val = *d;
            src_val = *s;
            *d += *s;
            result = *d;
        } else if (loplen == 8) {
            uint64_t* d = (uint64_t*)left;
            uint64_t* s = (uint64_t*)right;
            dest_val = *d;
            src_val = *s;
            *d += *s;
            result = *d;
        }
        
        // Update CPU flags
        regs->rflags = 0;
        
        // Zero Flag (ZF)
        if (result == 0)
            regs->rflags |= 0x40;
        
        // Sign Flag (SF)
        if (loplen == 1 && (result & 0x80))
            regs->rflags |= 0x80;
        else if (loplen == 2 && (result & 0x8000))
            regs->rflags |= 0x80;
        else if (loplen == 4 && (result & 0x80000000))
            regs->rflags |= 0x80;
        else if (loplen == 8 && (result & 0x8000000000000000ULL))
            regs->rflags |= 0x80;
        
        // Carry Flag (CF)
        uint64_t max_val = (loplen == 1) ? 0xFF : (loplen == 2) ? 0xFFFF : (loplen == 4) ? 0xFFFFFFFF : 0xFFFFFFFFFFFFFFFFULL;
        if (dest_val > max_val - src_val)
            regs->rflags |= 0x1;
        
        // Overflow Flag (OF) - signed overflow
        if (loplen == 1) {
            int8_t d8 = (int8_t)dest_val;
            int8_t s8 = (int8_t)src_val;
            int8_t r8 = (int8_t)result;
            if ((d8 > 0 && s8 > 0 && r8 < 0) || (d8 < 0 && s8 < 0 && r8 > 0))
                regs->rflags |= 0x800;
        } else if (loplen == 2) {
            int16_t d16 = (int16_t)dest_val;
            int16_t s16 = (int16_t)src_val;
            int16_t r16 = (int16_t)result;
            if ((d16 > 0 && s16 > 0 && r16 < 0) || (d16 < 0 && s16 < 0 && r16 > 0))
                regs->rflags |= 0x800;
        } else if (loplen == 4) {
            int32_t d32 = (int32_t)dest_val;
            int32_t s32 = (int32_t)src_val;
            int32_t r32 = (int32_t)result;
            if ((d32 > 0 && s32 > 0 && r32 < 0) || (d32 < 0 && s32 < 0 && r32 > 0))
                regs->rflags |= 0x800;
        } else if (loplen == 8) {
            int64_t d64 = (int64_t)dest_val;
            int64_t s64 = (int64_t)src_val;
            int64_t r64 = (int64_t)result;
            if ((d64 > 0 && s64 > 0 && r64 < 0) || (d64 < 0 && s64 < 0 && r64 > 0))
                regs->rflags |= 0x800;
        }
        
        // Parity Flag (PF) - only for low 8 bits
        uint8_t low_byte = result & 0xFF;
        int parity = 0;
        for (int i = 0; i < 8; i++) {
            if (low_byte & (1 << i))
                parity++;
        }
        if (parity % 2 == 0)
            regs->rflags |= 0x4;
    }
    else if (carch == x86) {
        lgr bufr;
        vectorGetValue(&simval, 0, &bufr);
        x86_Registers* regs = (x86_Registers*)bufr;
        if (left >= (uintptr_t)registers && left <= ((uintptr_t)registers + sizeof(x86_Registers)))
            x86_setupRegister(regs, left, &left, &loplen);
        if (right >= (uintptr_t)registers && right <= ((uintptr_t)registers + sizeof(x86_Registers)))
            x86_setupRegister(regs, right, &right, &roplen);

        // Perform addition based on operand size
        uint64_t result = 0;
        uint64_t src_val = 0;
        uint64_t dest_val = 0;
        
        if (loplen == 1) {
            uint8_t* d = (uint8_t*)left;
            uint8_t* s = (uint8_t*)right;
            dest_val = *d;
            src_val = *s;
            *d += *s;
            result = *d;
        } else if (loplen == 2) {
            uint16_t* d = (uint16_t*)left;
            uint16_t* s = (uint16_t*)right;
            dest_val = *d;
            src_val = *s;
            *d += *s;
            result = *d;
        } else if (loplen == 4) {
            uint32_t* d = (uint32_t*)left;
            uint32_t* s = (uint32_t*)right;
            dest_val = *d;
            src_val = *s;
            *d += *s;
            result = *d;
        } else if (loplen == 8) {
            uint64_t* d = (uint64_t*)left;
            uint64_t* s = (uint64_t*)right;
            dest_val = *d;
            src_val = *s;
            *d += *s;
            result = *d;
        }
        
        // Update CPU flags
        regs->eflags = 0;
        
        // Zero Flag (ZF)
        if (result == 0)
            regs->eflags |= 0x40;
        
        // Sign Flag (SF)
        if (loplen == 1 && (result & 0x80))
            regs->eflags |= 0x80;
        else if (loplen == 2 && (result & 0x8000))
            regs->eflags |= 0x80;
        else if (loplen == 4 && (result & 0x80000000))
            regs->eflags |= 0x80;
        else if (loplen == 8 && (result & 0x8000000000000000ULL))
            regs->eflags |= 0x80;
        
        // Carry Flag (CF)
        uint64_t max_val = (loplen == 1) ? 0xFF : (loplen == 2) ? 0xFFFF : (loplen == 4) ? 0xFFFFFFFF : 0xFFFFFFFFFFFFFFFFULL;
        if (dest_val > max_val - src_val)
            regs->eflags |= 0x1;
        
        // Overflow Flag (OF) - signed overflow
        if (loplen == 1) {
            int8_t d8 = (int8_t)dest_val;
            int8_t s8 = (int8_t)src_val;
            int8_t r8 = (int8_t)result;
            if ((d8 > 0 && s8 > 0 && r8 < 0) || (d8 < 0 && s8 < 0 && r8 > 0))
                regs->eflags |= 0x800;
        } else if (loplen == 2) {
            int16_t d16 = (int16_t)dest_val;
            int16_t s16 = (int16_t)src_val;
            int16_t r16 = (int16_t)result;
            if ((d16 > 0 && s16 > 0 && r16 < 0) || (d16 < 0 && s16 < 0 && r16 > 0))
                regs->eflags |= 0x800;
        } else if (loplen == 4) {
            int32_t d32 = (int32_t)dest_val;
            int32_t s32 = (int32_t)src_val;
            int32_t r32 = (int32_t)result;
            if ((d32 > 0 && s32 > 0 && r32 < 0) || (d32 < 0 && s32 < 0 && r32 > 0))
                regs->eflags |= 0x800;
        } else if (loplen == 8) {
            int64_t d64 = (int64_t)dest_val;
            int64_t s64 = (int64_t)src_val;
            int64_t r64 = (int64_t)result;
            if ((d64 > 0 && s64 > 0 && r64 < 0) || (d64 < 0 && s64 < 0 && r64 > 0))
                regs->eflags |= 0x800;
        }
        
        // Parity Flag (PF) - only for low 8 bits
        uint8_t low_byte = result & 0xFF;
        int parity = 0;
        for (int i = 0; i < 8; i++) {
            if (low_byte & (1 << i))
                parity++;
        }
        if (parity % 2 == 0)
            regs->eflags |= 0x4;
    } else
        bugDetected("Unknown Archtecture");
    
    return PROC_SUCCESS;
}

ProcessorResult subtract(uintptr_t left, unsigned int loplen, uintptr_t right, unsigned int roplen,
                    vector eops, vector eopslen) {
    if (!isInit)
        return NOT_INITIALIZED;
    
    if (carch == x86_64) {
        lgr bufr;
        vectorGetValue(&simval, 0, &bufr);
        x86_64_Registers* regs = (x86_64_Registers*)bufr;
        if (left >= (uintptr_t)registers && left <= ((uintptr_t)registers + sizeof(x86_64_Registers)))
            x86_64_setupRegister(regs, left, &left, &loplen);
        if (right >= (uintptr_t)registers && right <= ((uintptr_t)registers + sizeof(x86_64_Registers)))
            x86_64_setupRegister(regs, right, &right, &roplen);
        
        // Perform subtraction based on operand size
        uint64_t result = 0;
        uint64_t src_val = 0;
        uint64_t dest_val = 0;
        
        if (loplen == 1) {
            uint8_t* d = (uint8_t*)left;
            uint8_t* s = (uint8_t*)right;
            dest_val = *d;
            src_val = *s;
            *d -= *s;
            result = *d;
        } else if (loplen == 2) {
            uint16_t* d = (uint16_t*)left;
            uint16_t* s = (uint16_t*)right;
            dest_val = *d;
            src_val = *s;
            *d -= *s;
            result = *d;
        } else if (loplen == 4) {
            uint32_t* d = (uint32_t*)left;
            uint32_t* s = (uint32_t*)right;
            dest_val = *d;
            src_val = *s;
            *d -= *s;
            result = *d;
        } else if (loplen == 8) {
            uint64_t* d = (uint64_t*)left;
            uint64_t* s = (uint64_t*)right;
            dest_val = *d;
            src_val = *s;
            *d -= *s;
            result = *d;
        }
        
        // Update CPU flags
        regs->rflags = 0;
        
        // Zero Flag (ZF)
        if (result == 0)
            regs->rflags |= 0x40;
        
        // Sign Flag (SF)
        if (loplen == 1 && (result & 0x80))
            regs->rflags |= 0x80;
        else if (loplen == 2 && (result & 0x8000))
            regs->rflags |= 0x80;
        else if (loplen == 4 && (result & 0x80000000))
            regs->rflags |= 0x80;
        else if (loplen == 8 && (result & 0x8000000000000000ULL))
            regs->rflags |= 0x80;
        
        // Carry Flag (CF) - set if borrow occurred
        if (dest_val < src_val)
            regs->rflags |= 0x1;
        
        // Overflow Flag (OF) - signed overflow
        if (loplen == 1) {
            int8_t d8 = (int8_t)dest_val;
            int8_t s8 = (int8_t)src_val;
            int8_t r8 = (int8_t)result;
            if ((d8 > 0 && s8 < 0 && r8 < 0) || (d8 < 0 && s8 > 0 && r8 > 0))
                regs->rflags |= 0x800;
        } else if (loplen == 2) {
            int16_t d16 = (int16_t)dest_val;
            int16_t s16 = (int16_t)src_val;
            int16_t r16 = (int16_t)result;
            if ((d16 > 0 && s16 < 0 && r16 < 0) || (d16 < 0 && s16 > 0 && r16 > 0))
                regs->rflags |= 0x800;
        } else if (loplen == 4) {
            int32_t d32 = (int32_t)dest_val;
            int32_t s32 = (int32_t)src_val;
            int32_t r32 = (int32_t)result;
            if ((d32 > 0 && s32 < 0 && r32 < 0) || (d32 < 0 && s32 > 0 && r32 > 0))
                regs->rflags |= 0x800;
        } else if (loplen == 8) {
            int64_t d64 = (int64_t)dest_val;
            int64_t s64 = (int64_t)src_val;
            int64_t r64 = (int64_t)result;
            if ((d64 > 0 && s64 < 0 && r64 < 0) || (d64 < 0 && s64 > 0 && r64 > 0))
                regs->rflags |= 0x800;
        }
        
        // Parity Flag (PF) - only for low 8 bits
        uint8_t low_byte = result & 0xFF;
        int parity = 0;
        for (int i = 0; i < 8; i++) {
            if (low_byte & (1 << i))
                parity++;
        }
        if (parity % 2 == 0)
            regs->rflags |= 0x4;
    }
    else if (carch == x86) {
        lgr bufr;
        vectorGetValue(&simval, 0, &bufr);
        x86_Registers* regs = (x86_Registers*)bufr;
        if (left >= (uintptr_t)registers && left <= ((uintptr_t)registers + sizeof(x86_Registers)))
            x86_setupRegister(regs, left, &left, &loplen);
        if (right >= (uintptr_t)registers && right <= ((uintptr_t)registers + sizeof(x86_Registers)))
            x86_setupRegister(regs, right, &right, &roplen);

        // Perform subtraction based on operand size
        uint64_t result = 0;
        uint64_t src_val = 0;
        uint64_t dest_val = 0;
        
        if (loplen == 1) {
            uint8_t* d = (uint8_t*)left;
            uint8_t* s = (uint8_t*)right;
            dest_val = *d;
            src_val = *s;
            *d -= *s;
            result = *d;
        } else if (loplen == 2) {
            uint16_t* d = (uint16_t*)left;
            uint16_t* s = (uint16_t*)right;
            dest_val = *d;
            src_val = *s;
            *d -= *s;
            result = *d;
        } else if (loplen == 4) {
            uint32_t* d = (uint32_t*)left;
            uint32_t* s = (uint32_t*)right;
            dest_val = *d;
            src_val = *s;
            *d -= *s;
            result = *d;
        } else if (loplen == 8) {
            uint64_t* d = (uint64_t*)left;
            uint64_t* s = (uint64_t*)right;
            dest_val = *d;
            src_val = *s;
            *d -= *s;
            result = *d;
        }
        
        // Update CPU flags
        regs->eflags = 0;
        
        // Zero Flag (ZF)
        if (result == 0)
            regs->eflags |= 0x40;
        
        // Sign Flag (SF)
        if (loplen == 1 && (result & 0x80))
            regs->eflags |= 0x80;
        else if (loplen == 2 && (result & 0x8000))
            regs->eflags |= 0x80;
        else if (loplen == 4 && (result & 0x80000000))
            regs->eflags |= 0x80;
        else if (loplen == 8 && (result & 0x8000000000000000ULL))
            regs->eflags |= 0x80;
        
        // Carry Flag (CF) - set if borrow occurred
        if (dest_val < src_val)
            regs->eflags |= 0x1;
        
        // Overflow Flag (OF) - signed overflow
        if (loplen == 1) {
            int8_t d8 = (int8_t)dest_val;
            int8_t s8 = (int8_t)src_val;
            int8_t r8 = (int8_t)result;
            if ((d8 > 0 && s8 < 0 && r8 < 0) || (d8 < 0 && s8 > 0 && r8 > 0))
                regs->eflags |= 0x800;
        } else if (loplen == 2) {
            int16_t d16 = (int16_t)dest_val;
            int16_t s16 = (int16_t)src_val;
            int16_t r16 = (int16_t)result;
            if ((d16 > 0 && s16 < 0 && r16 < 0) || (d16 < 0 && s16 > 0 && r16 > 0))
                regs->eflags |= 0x800;
        } else if (loplen == 4) {
            int32_t d32 = (int32_t)dest_val;
            int32_t s32 = (int32_t)src_val;
            int32_t r32 = (int32_t)result;
            if ((d32 > 0 && s32 < 0 && r32 < 0) || (d32 < 0 && s32 > 0 && r32 > 0))
                regs->eflags |= 0x800;
        } else if (loplen == 8) {
            int64_t d64 = (int64_t)dest_val;
            int64_t s64 = (int64_t)src_val;
            int64_t r64 = (int64_t)result;
            if ((d64 > 0 && s64 < 0 && r64 < 0) || (d64 < 0 && s64 > 0 && r64 > 0))
                regs->eflags |= 0x800;
        }
        
        // Parity Flag (PF) - only for low 8 bits
        uint8_t low_byte = result & 0xFF;
        int parity = 0;
        for (int i = 0; i < 8; i++) {
            if (low_byte & (1 << i))
                parity++;
        }
        if (parity % 2 == 0)
            regs->eflags |= 0x4;
    } else
        bugDetected("Unknown Archtecture");
    
    return PROC_SUCCESS;
}

ProcessorResult multiply(uintptr_t left, unsigned int loplen, uintptr_t right, unsigned int roplen,
                    vector eops, vector eopslen) {
    if (!isInit)
        return NOT_INITIALIZED;
    
    if (carch == x86_64) {
        lgr bufr;
        vectorGetValue(&simval, 0, &bufr);
        x86_64_Registers* regs = (x86_64_Registers*)bufr;
        if (left >= (uintptr_t)registers && left <= ((uintptr_t)registers + sizeof(x86_64_Registers)))
            x86_64_setupRegister(regs, left, &left, &loplen);
        if (right >= (uintptr_t)registers && right <= ((uintptr_t)registers + sizeof(x86_64_Registers)))
            x86_64_setupRegister(regs, right, &right, &roplen);
        
        // Perform multiplication based on operand size
        uint64_t result = 0;
        uint64_t src_val = 0;
        uint64_t dest_val = 0;
        
        if (loplen == 1) {
            uint8_t* d = (uint8_t*)left;
            uint8_t* s = (uint8_t*)right;
            dest_val = *d;
            src_val = *s;
            *d *= *s;
            result = *d;
        } else if (loplen == 2) {
            uint16_t* d = (uint16_t*)left;
            uint16_t* s = (uint16_t*)right;
            dest_val = *d;
            src_val = *s;
            *d *= *s;
            result = *d;
        } else if (loplen == 4) {
            uint32_t* d = (uint32_t*)left;
            uint32_t* s = (uint32_t*)right;
            dest_val = *d;
            src_val = *s;
            *d *= *s;
            result = *d;
        } else if (loplen == 8) {
            uint64_t* d = (uint64_t*)left;
            uint64_t* s = (uint64_t*)right;
            dest_val = *d;
            src_val = *s;
            *d *= *s;
            result = *d;
        }
        
        // Update CPU flags
        regs->rflags = 0;
        
        // Zero Flag (ZF)
        if (result == 0)
            regs->rflags |= 0x40;
        
        // Sign Flag (SF)
        if (loplen == 1 && (result & 0x80))
            regs->rflags |= 0x80;
        else if (loplen == 2 && (result & 0x8000))
            regs->rflags |= 0x80;
        else if (loplen == 4 && (result & 0x80000000))
            regs->rflags |= 0x80;
        else if (loplen == 8 && (result & 0x8000000000000000ULL))
            regs->rflags |= 0x80;
        
        // Carry Flag (CF) - set if high bits of result are non-zero
        uint64_t max_val = (loplen == 1) ? 0xFF : (loplen == 2) ? 0xFFFF : (loplen == 4) ? 0xFFFFFFFF : 0xFFFFFFFFFFFFFFFFULL;
        if (dest_val != 0 && result / dest_val != src_val)
            regs->rflags |= 0x1;
        
        // Overflow Flag (OF) - signed overflow
        if (loplen == 1) {
            int8_t d8 = (int8_t)dest_val;
            int8_t s8 = (int8_t)src_val;
            int8_t r8 = (int8_t)result;
            if (d8 != 0 && r8 / d8 != s8)
                regs->rflags |= 0x800;
        } else if (loplen == 2) {
            int16_t d16 = (int16_t)dest_val;
            int16_t s16 = (int16_t)src_val;
            int16_t r16 = (int16_t)result;
            if (d16 != 0 && r16 / d16 != s16)
                regs->rflags |= 0x800;
        } else if (loplen == 4) {
            int32_t d32 = (int32_t)dest_val;
            int32_t s32 = (int32_t)src_val;
            int32_t r32 = (int32_t)result;
            if (d32 != 0 && r32 / d32 != s32)
                regs->rflags |= 0x800;
        } else if (loplen == 8) {
            int64_t d64 = (int64_t)dest_val;
            int64_t s64 = (int64_t)src_val;
            int64_t r64 = (int64_t)result;
            if (d64 != 0 && r64 / d64 != s64)
                regs->rflags |= 0x800;
        }
        
        // Parity Flag (PF) - only for low 8 bits
        uint8_t low_byte = result & 0xFF;
        int parity = 0;
        for (int i = 0; i < 8; i++) {
            if (low_byte & (1 << i))
                parity++;
        }
        if (parity % 2 == 0)
            regs->rflags |= 0x4;
    }
    else if (carch == x86) {
        lgr bufr;
        vectorGetValue(&simval, 0, &bufr);
        x86_Registers* regs = (x86_Registers*)bufr;
        if (left >= (uintptr_t)registers && left <= ((uintptr_t)registers + sizeof(x86_Registers)))
            x86_setupRegister(regs, left, &left, &loplen);
        if (right >= (uintptr_t)registers && right <= ((uintptr_t)registers + sizeof(x86_Registers)))
            x86_setupRegister(regs, right, &right, &roplen);

        // Perform multiplication based on operand size
        uint64_t result = 0;
        uint64_t src_val = 0;
        uint64_t dest_val = 0;
        
        if (loplen == 1) {
            uint8_t* d = (uint8_t*)left;
            uint8_t* s = (uint8_t*)right;
            dest_val = *d;
            src_val = *s;
            *d *= *s;
            result = *d;
        } else if (loplen == 2) {
            uint16_t* d = (uint16_t*)left;
            uint16_t* s = (uint16_t*)right;
            dest_val = *d;
            src_val = *s;
            *d *= *s;
            result = *d;
        } else if (loplen == 4) {
            uint32_t* d = (uint32_t*)left;
            uint32_t* s = (uint32_t*)right;
            dest_val = *d;
            src_val = *s;
            *d *= *s;
            result = *d;
        } else if (loplen == 8) {
            uint64_t* d = (uint64_t*)left;
            uint64_t* s = (uint64_t*)right;
            dest_val = *d;
            src_val = *s;
            *d *= *s;
            result = *d;
        }
        
        // Update CPU flags
        regs->eflags = 0;
        
        // Zero Flag (ZF)
        if (result == 0)
            regs->eflags |= 0x40;
        
        // Sign Flag (SF)
        if (loplen == 1 && (result & 0x80))
            regs->eflags |= 0x80;
        else if (loplen == 2 && (result & 0x8000))
            regs->eflags |= 0x80;
        else if (loplen == 4 && (result & 0x80000000))
            regs->eflags |= 0x80;
        else if (loplen == 8 && (result & 0x8000000000000000ULL))
            regs->eflags |= 0x80;
        
        // Carry Flag (CF) - set if high bits of result are non-zero
        uint64_t max_val = (loplen == 1) ? 0xFF : (loplen == 2) ? 0xFFFF : (loplen == 4) ? 0xFFFFFFFF : 0xFFFFFFFFFFFFFFFFULL;
        if (dest_val != 0 && result / dest_val != src_val)
            regs->eflags |= 0x1;
        
        // Overflow Flag (OF) - signed overflow
        if (loplen == 1) {
            int8_t d8 = (int8_t)dest_val;
            int8_t s8 = (int8_t)src_val;
            int8_t r8 = (int8_t)result;
            if (d8 != 0 && r8 / d8 != s8)
                regs->eflags |= 0x800;
        } else if (loplen == 2) {
            int16_t d16 = (int16_t)dest_val;
            int16_t s16 = (int16_t)src_val;
            int16_t r16 = (int16_t)result;
            if (d16 != 0 && r16 / d16 != s16)
                regs->eflags |= 0x800;
        } else if (loplen == 4) {
            int32_t d32 = (int32_t)dest_val;
            int32_t s32 = (int32_t)src_val;
            int32_t r32 = (int32_t)result;
            if (d32 != 0 && r32 / d32 != s32)
                regs->eflags |= 0x800;
        } else if (loplen == 8) {
            int64_t d64 = (int64_t)dest_val;
            int64_t s64 = (int64_t)src_val;
            int64_t r64 = (int64_t)result;
            if (d64 != 0 && r64 / d64 != s64)
                regs->eflags |= 0x800;
        }
        
        // Parity Flag (PF) - only for low 8 bits
        uint8_t low_byte = result & 0xFF;
        int parity = 0;
        for (int i = 0; i < 8; i++) {
            if (low_byte & (1 << i))
                parity++;
        }
        if (parity % 2 == 0)
            regs->eflags |= 0x4;
    } else
        bugDetected("Unknown Archtecture");
    
    return PROC_SUCCESS;
}

ProcessorResult divide(uintptr_t left, unsigned int loplen, uintptr_t right, unsigned int roplen,
                    vector eops, vector eopslen) {
    if (!isInit)
        return NOT_INITIALIZED;
    
    if (carch == x86_64) {
        lgr bufr;
        vectorGetValue(&simval, 0, &bufr);
        x86_64_Registers* regs = (x86_64_Registers*)bufr;
        if (left >= (uintptr_t)registers && left <= ((uintptr_t)registers + sizeof(x86_64_Registers)))
            x86_64_setupRegister(regs, left, &left, &loplen);
        if (right >= (uintptr_t)registers && right <= ((uintptr_t)registers + sizeof(x86_64_Registers)))
            x86_64_setupRegister(regs, right, &right, &roplen);
        
        // Check for division by zero
        if ((roplen == 1 && *(uint8_t*)right == 0) || (loplen == 1 && *(uint8_t*)left == 0))
            return DIVISION_BY_ZERO;
        else if ((roplen == 2 && *(uint16_t*)right == 0) || (loplen == 1 && *(uint16_t*)left == 0))
            return DIVISION_BY_ZERO;
        else if ((roplen == 4 && *(uint32_t*)right == 0) || (loplen == 1 && *(uint32_t*)left == 0))
            return DIVISION_BY_ZERO;
        else if ((roplen == 8 && *(uint64_t*)right == 0) || (loplen == 1 && *(uint64_t*)left == 0))
            return DIVISION_BY_ZERO;
        
        // Perform division based on operand size
        uint64_t result = 0;
        uint64_t src_val = 0;
        uint64_t dest_val = 0;
        
        if (loplen == 1) {
            uint8_t* d = (uint8_t*)left;
            uint8_t* s = (uint8_t*)right;
            dest_val = *d;
            src_val = *s;
            *d /= *s;
            result = *d;
        } else if (loplen == 2) {
            uint16_t* d = (uint16_t*)left;
            uint16_t* s = (uint16_t*)right;
            dest_val = *d;
            src_val = *s;
            *d /= *s;
            result = *d;
        } else if (loplen == 4) {
            uint32_t* d = (uint32_t*)left;
            uint32_t* s = (uint32_t*)right;
            dest_val = *d;
            src_val = *s;
            *d /= *s;
            result = *d;
        } else if (loplen == 8) {
            uint64_t* d = (uint64_t*)left;
            uint64_t* s = (uint64_t*)right;
            dest_val = *d;
            src_val = *s;
            *d /= *s;
            result = *d;
        }
        
        // Update CPU flags
        regs->rflags = 0;
        
        // Zero Flag (ZF)
        if (result == 0)
            regs->rflags |= 0x40;
        
        // Sign Flag (SF)
        if (loplen == 1 && (result & 0x80))
            regs->rflags |= 0x80;
        else if (loplen == 2 && (result & 0x8000))
            regs->rflags |= 0x80;
        else if (loplen == 4 && (result & 0x80000000))
            regs->rflags |= 0x80;
        else if (loplen == 8 && (result & 0x8000000000000000ULL))
            regs->rflags |= 0x80;
        
        // Parity Flag (PF) - only for low 8 bits
        uint8_t low_byte = result & 0xFF;
        int parity = 0;
        for (int i = 0; i < 8; i++) {
            if (low_byte & (1 << i))
                parity++;
        }
        if (parity % 2 == 0)
            regs->rflags |= 0x4;
    }
    else if (carch == x86) {
        lgr bufr;
        vectorGetValue(&simval, 0, &bufr);
        x86_Registers* regs = (x86_Registers*)bufr;
        if (left >= (uintptr_t)registers && left <= ((uintptr_t)registers + sizeof(x86_Registers)))
            x86_setupRegister(regs, left, &left, &loplen);
        if (right >= (uintptr_t)registers && right <= ((uintptr_t)registers + sizeof(x86_Registers)))
            x86_setupRegister(regs, right, &right, &roplen);

        // Check for division by zero
        if ((roplen == 1 && *(uint8_t*)right == 0) || (loplen == 1 && *(uint8_t*)left == 0))
            return DIVISION_BY_ZERO;
        else if ((roplen == 2 && *(uint16_t*)right == 0) || (loplen == 1 && *(uint16_t*)left == 0))
            return DIVISION_BY_ZERO;
        else if ((roplen == 4 && *(uint32_t*)right == 0) || (loplen == 1 && *(uint32_t*)left == 0))
            return DIVISION_BY_ZERO;
        else if ((roplen == 8 && *(uint64_t*)right == 0) || (loplen == 1 && *(uint64_t*)left == 0))
            return DIVISION_BY_ZERO;

        // Perform division based on operand size
        uint64_t result = 0;
        uint64_t src_val = 0;
        uint64_t dest_val = 0;
        
        if (loplen == 1) {
            uint8_t* d = (uint8_t*)left;
            uint8_t* s = (uint8_t*)right;
            dest_val = *d;
            src_val = *s;
            *d /= *s;
            result = *d;
        } else if (loplen == 2) {
            uint16_t* d = (uint16_t*)left;
            uint16_t* s = (uint16_t*)right;
            dest_val = *d;
            src_val = *s;
            *d /= *s;
            result = *d;
        } else if (loplen == 4) {
            uint32_t* d = (uint32_t*)left;
            uint32_t* s = (uint32_t*)right;
            dest_val = *d;
            src_val = *s;
            *d /= *s;
            result = *d;
        } else if (loplen == 8) {
            uint64_t* d = (uint64_t*)left;
            uint64_t* s = (uint64_t*)right;
            dest_val = *d;
            src_val = *s;
            *d /= *s;
            result = *d;
        }
        
        // Update CPU flags
        regs->eflags = 0;
        
        // Zero Flag (ZF)
        if (result == 0)
            regs->eflags |= 0x40;
        
        // Sign Flag (SF)
        if (loplen == 1 && (result & 0x80))
            regs->eflags |= 0x80;
        else if (loplen == 2 && (result & 0x8000))
            regs->eflags |= 0x80;
        else if (loplen == 4 && (result & 0x80000000))
            regs->eflags |= 0x80;
        else if (loplen == 8 && (result & 0x8000000000000000ULL))
            regs->eflags |= 0x80;
        
        // Parity Flag (PF) - only for low 8 bits
        uint8_t low_byte = result & 0xFF;
        int parity = 0;
        for (int i = 0; i < 8; i++) {
            if (low_byte & (1 << i))
                parity++;
        }
        if (parity % 2 == 0)
            regs->eflags |= 0x4;
    } else
        bugDetected("Unknown Archtecture");
    
    return PROC_SUCCESS;
}

ProcessorResult compare(uintptr_t left, unsigned int loplen, uintptr_t right, unsigned int roplen,
                        vector eops, vector eopslen) {
    if (!isInit)
        return NOT_INITIALIZED;
    
    if (carch == x86_64) {
        lgr bufr;
        vectorGetValue(&simval, 0, &bufr);
        x86_64_Registers* regs = (x86_64_Registers*)bufr;
        if (left >= (uintptr_t)registers && left <= ((uintptr_t)registers + sizeof(x86_64_Registers)))
            x86_64_setupRegister(regs, left, &left, &loplen);
        if (right >= (uintptr_t)registers && right <= ((uintptr_t)registers + sizeof(x86_64_Registers)))
            x86_64_setupRegister(regs, right, &right, &roplen);
        
        // Perform comparison based on operand size (subtraction without storing result)
        uint64_t result = 0;
        uint64_t src_val = 0;
        uint64_t dest_val = 0;
        
        if (loplen == 1) {
            uint8_t* d = (uint8_t*)left;
            uint8_t* s = (uint8_t*)right;
            dest_val = *d;
            src_val = *s;
            result = dest_val - src_val;
        } else if (loplen == 2) {
            uint16_t* d = (uint16_t*)left;
            uint16_t* s = (uint16_t*)right;
            dest_val = *d;
            src_val = *s;
            result = dest_val - src_val;
        } else if (loplen == 4) {
            uint32_t* d = (uint32_t*)left;
            uint32_t* s = (uint32_t*)right;
            dest_val = *d;
            src_val = *s;
            result = dest_val - src_val;
        } else if (loplen == 8) {
            uint64_t* d = (uint64_t*)left;
            uint64_t* s = (uint64_t*)right;
            dest_val = *d;
            src_val = *s;
            result = dest_val - src_val;
        }
        
        // Update CPU flags
        regs->rflags = 0;
        
        // Zero Flag (ZF)
        if (result == 0)
            regs->rflags |= 0x40;
        
        // Sign Flag (SF)
        if (loplen == 1 && (result & 0x80))
            regs->rflags |= 0x80;
        else if (loplen == 2 && (result & 0x8000))
            regs->rflags |= 0x80;
        else if (loplen == 4 && (result & 0x80000000))
            regs->rflags |= 0x80;
        else if (loplen == 8 && (result & 0x8000000000000000ULL))
            regs->rflags |= 0x80;
        
        // Carry Flag (CF) - set if borrow occurred
        if (dest_val < src_val)
            regs->rflags |= 0x1;
        
        // Overflow Flag (OF) - signed overflow
        if (loplen == 1) {
            int8_t d8 = (int8_t)dest_val;
            int8_t s8 = (int8_t)src_val;
            int8_t r8 = (int8_t)result;
            if ((d8 > 0 && s8 < 0 && r8 < 0) || (d8 < 0 && s8 > 0 && r8 > 0))
                regs->rflags |= 0x800;
        } else if (loplen == 2) {
            int16_t d16 = (int16_t)dest_val;
            int16_t s16 = (int16_t)src_val;
            int16_t r16 = (int16_t)result;
            if ((d16 > 0 && s16 < 0 && r16 < 0) || (d16 < 0 && s16 > 0 && r16 > 0))
                regs->rflags |= 0x800;
        } else if (loplen == 4) {
            int32_t d32 = (int32_t)dest_val;
            int32_t s32 = (int32_t)src_val;
            int32_t r32 = (int32_t)result;
            if ((d32 > 0 && s32 < 0 && r32 < 0) || (d32 < 0 && s32 > 0 && r32 > 0))
                regs->rflags |= 0x800;
        } else if (loplen == 8) {
            int64_t d64 = (int64_t)dest_val;
            int64_t s64 = (int64_t)src_val;
            int64_t r64 = (int64_t)result;
            if ((d64 > 0 && s64 < 0 && r64 < 0) || (d64 < 0 && s64 > 0 && r64 > 0))
                regs->rflags |= 0x800;
        }
        
        // Parity Flag (PF) - only for low 8 bits
        uint8_t low_byte = result & 0xFF;
        int parity = 0;
        for (int i = 0; i < 8; i++) {
            if (low_byte & (1 << i))
                parity++;
        }
        if (parity % 2 == 0)
            regs->rflags |= 0x4;
    }
    else if (carch == x86) {
        lgr bufr;
        vectorGetValue(&simval, 0, &bufr);
        x86_Registers* regs = (x86_Registers*)bufr;
        if (left >= (uintptr_t)registers && left <= ((uintptr_t)registers + sizeof(x86_Registers)))
            x86_setupRegister(regs, left, &left, &loplen);
        if (right >= (uintptr_t)registers && right <= ((uintptr_t)registers + sizeof(x86_Registers)))
            x86_setupRegister(regs, right, &right, &roplen);

        // Perform comparison based on operand size (subtraction without storing result)
        uint64_t result = 0;
        uint64_t src_val = 0;
        uint64_t dest_val = 0;
        
        if (loplen == 1) {
            uint8_t* d = (uint8_t*)left;
            uint8_t* s = (uint8_t*)right;
            dest_val = *d;
            src_val = *s;
            result = dest_val - src_val;
        } else if (loplen == 2) {
            uint16_t* d = (uint16_t*)left;
            uint16_t* s = (uint16_t*)right;
            dest_val = *d;
            src_val = *s;
            result = dest_val - src_val;
        } else if (loplen == 4) {
            uint32_t* d = (uint32_t*)left;
            uint32_t* s = (uint32_t*)right;
            dest_val = *d;
            src_val = *s;
            result = dest_val - src_val;
        } else if (loplen == 8) {
            uint64_t* d = (uint64_t*)left;
            uint64_t* s = (uint64_t*)right;
            dest_val = *d;
            src_val = *s;
            result = dest_val - src_val;
        }
        
        // Update CPU flags
        regs->eflags = 0;
        
        // Zero Flag (ZF)
        if (result == 0)
            regs->eflags |= 0x40;
        
        // Sign Flag (SF)
        if (loplen == 1 && (result & 0x80))
            regs->eflags |= 0x80;
        else if (loplen == 2 && (result & 0x8000))
            regs->eflags |= 0x80;
        else if (loplen == 4 && (result & 0x80000000))
            regs->eflags |= 0x80;
        else if (loplen == 8 && (result & 0x8000000000000000ULL))
            regs->eflags |= 0x80;
        
        // Carry Flag (CF) - set if borrow occurred
        if (dest_val < src_val)
            regs->eflags |= 0x1;
        
        // Overflow Flag (OF) - signed overflow
        if (loplen == 1) {
            int8_t d8 = (int8_t)dest_val;
            int8_t s8 = (int8_t)src_val;
            int8_t r8 = (int8_t)result;
            if ((d8 > 0 && s8 < 0 && r8 < 0) || (d8 < 0 && s8 > 0 && r8 > 0))
                regs->eflags |= 0x800;
        } else if (loplen == 2) {
            int16_t d16 = (int16_t)dest_val;
            int16_t s16 = (int16_t)src_val;
            int16_t r16 = (int16_t)result;
            if ((d16 > 0 && s16 < 0 && r16 < 0) || (d16 < 0 && s16 > 0 && r16 > 0))
                regs->eflags |= 0x800;
        } else if (loplen == 4) {
            int32_t d32 = (int32_t)dest_val;
            int32_t s32 = (int32_t)src_val;
            int32_t r32 = (int32_t)result;
            if ((d32 > 0 && s32 < 0 && r32 < 0) || (d32 < 0 && s32 > 0 && r32 > 0))
                regs->eflags |= 0x800;
        } else if (loplen == 8) {
            int64_t d64 = (int64_t)dest_val;
            int64_t s64 = (int64_t)src_val;
            int64_t r64 = (int64_t)result;
            if ((d64 > 0 && s64 < 0 && r64 < 0) || (d64 < 0 && s64 > 0 && r64 > 0))
                regs->eflags |= 0x800;
        }
        
        // Parity Flag (PF) - only for low 8 bits
        uint8_t low_byte = result & 0xFF;
        int parity = 0;
        for (int i = 0; i < 8; i++) {
            if (low_byte & (1 << i))
                parity++;
        }
        if (parity % 2 == 0)
            regs->eflags |= 0x4;
    } else
        bugDetected("Unknown Archtecture");
    
    return PROC_SUCCESS;
}

ProcessorResult increment(uintptr_t left, unsigned int loplen) {
    if (!isInit)
        return NOT_INITIALIZED;
    
    if (carch == x86_64) {
        lgr bufr;
        vectorGetValue(&simval, 0, &bufr);
        x86_64_Registers* regs = (x86_64_Registers*)bufr;
        if (left >= (uintptr_t)registers && left <= ((uintptr_t)registers + sizeof(x86_64_Registers)))
            x86_64_setupRegister(regs, left, &left, &loplen);
        
        // Perform increment based on operand size
        uint64_t result = 0;
        uint64_t dest_val = 0;
        
        if (loplen == 1) {
            uint8_t* d = (uint8_t*)left;
            dest_val = *d;
            (*d)++;
            result = *d;
        } else if (loplen == 2) {
            uint16_t* d = (uint16_t*)left;
            dest_val = *d;
            (*d)++;
            result = *d;
        } else if (loplen == 4) {
            uint32_t* d = (uint32_t*)left;
            dest_val = *d;
            (*d)++;
            result = *d;
        } else if (loplen == 8) {
            uint64_t* d = (uint64_t*)left;
            dest_val = *d;
            (*d)++;
            result = *d;
        }
        
        // Update CPU flags
        regs->rflags = 0;
        
        // Zero Flag (ZF)
        if (result == 0)
            regs->rflags |= 0x40;
        
        // Sign Flag (SF)
        if (loplen == 1 && (result & 0x80))
            regs->rflags |= 0x80;
        else if (loplen == 2 && (result & 0x8000))
            regs->rflags |= 0x80;
        else if (loplen == 4 && (result & 0x80000000))
            regs->rflags |= 0x80;
        else if (loplen == 8 && (result & 0x8000000000000000ULL))
            regs->rflags |= 0x80;
        
        // Overflow Flag (OF) - signed overflow (increment can overflow from max to min)
        if (loplen == 1) {
            int8_t d8 = (int8_t)dest_val;
            int8_t r8 = (int8_t)result;
            if (d8 > 0 && r8 < 0)
                regs->rflags |= 0x800;
        } else if (loplen == 2) {
            int16_t d16 = (int16_t)dest_val;
            int16_t r16 = (int16_t)result;
            if (d16 > 0 && r16 < 0)
                regs->rflags |= 0x800;
        } else if (loplen == 4) {
            int32_t d32 = (int32_t)dest_val;
            int32_t r32 = (int32_t)result;
            if (d32 > 0 && r32 < 0)
                regs->rflags |= 0x800;
        } else if (loplen == 8) {
            int64_t d64 = (int64_t)dest_val;
            int64_t r64 = (int64_t)result;
            if (d64 > 0 && r64 < 0)
                regs->rflags |= 0x800;
        }
        
        // Parity Flag (PF) - only for low 8 bits
        uint8_t low_byte = result & 0xFF;
        int parity = 0;
        for (int i = 0; i < 8; i++) {
            if (low_byte & (1 << i))
                parity++;
        }
        if (parity % 2 == 0)
            regs->rflags |= 0x4;
    }
    else if (carch == x86) {
        lgr bufr;
        vectorGetValue(&simval, 0, &bufr);
        x86_Registers* regs = (x86_Registers*)bufr;
        if (left >= (uintptr_t)registers && left <= ((uintptr_t)registers + sizeof(x86_Registers)))
            x86_setupRegister(regs, left, &left, &loplen);

        // Perform increment based on operand size
        uint64_t result = 0;
        uint64_t dest_val = 0;
        
        if (loplen == 1) {
            uint8_t* d = (uint8_t*)left;
            dest_val = *d;
            (*d)++;
            result = *d;
        } else if (loplen == 2) {
            uint16_t* d = (uint16_t*)left;
            dest_val = *d;
            (*d)++;
            result = *d;
        } else if (loplen == 4) {
            uint32_t* d = (uint32_t*)left;
            dest_val = *d;
            (*d)++;
            result = *d;
        } else if (loplen == 8) {
            uint64_t* d = (uint64_t*)left;
            dest_val = *d;
            (*d)++;
            result = *d;
        }
        
        // Update CPU flags
        regs->eflags = 0;
        
        // Zero Flag (ZF)
        if (result == 0)
            regs->eflags |= 0x40;
        
        // Sign Flag (SF)
        if (loplen == 1 && (result & 0x80))
            regs->eflags |= 0x80;
        else if (loplen == 2 && (result & 0x8000))
            regs->eflags |= 0x80;
        else if (loplen == 4 && (result & 0x80000000))
            regs->eflags |= 0x80;
        else if (loplen == 8 && (result & 0x8000000000000000ULL))
            regs->eflags |= 0x80;
        
        // Overflow Flag (OF) - signed overflow
        if (loplen == 1) {
            int8_t d8 = (int8_t)dest_val;
            int8_t r8 = (int8_t)result;
            if (d8 > 0 && r8 < 0)
                regs->eflags |= 0x800;
        } else if (loplen == 2) {
            int16_t d16 = (int16_t)dest_val;
            int16_t r16 = (int16_t)result;
            if (d16 > 0 && r16 < 0)
                regs->eflags |= 0x800;
        } else if (loplen == 4) {
            int32_t d32 = (int32_t)dest_val;
            int32_t r32 = (int32_t)result;
            if (d32 > 0 && r32 < 0)
                regs->eflags |= 0x800;
        } else if (loplen == 8) {
            int64_t d64 = (int64_t)dest_val;
            int64_t r64 = (int64_t)result;
            if (d64 > 0 && r64 < 0)
                regs->eflags |= 0x800;
        }
        
        // Parity Flag (PF) - only for low 8 bits
        uint8_t low_byte = result & 0xFF;
        int parity = 0;
        for (int i = 0; i < 8; i++) {
            if (low_byte & (1 << i))
                parity++;
        }
        if (parity % 2 == 0)
            regs->eflags |= 0x4;
    } else
        bugDetected("Unknown Archtecture");
    
    return PROC_SUCCESS;
}

ProcessorResult decrement(uintptr_t left, unsigned int loplen) {
    if (!isInit)
        return NOT_INITIALIZED;
    
    if (carch == x86_64) {
        lgr bufr;
        vectorGetValue(&simval, 0, &bufr);
        x86_64_Registers* regs = (x86_64_Registers*)bufr;
        if (left >= (uintptr_t)registers && left <= ((uintptr_t)registers + sizeof(x86_64_Registers)))
            x86_64_setupRegister(regs, left, &left, &loplen);
        
        // Perform decrement based on operand size
        uint64_t result = 0;
        uint64_t dest_val = 0;
        
        if (loplen == 1) {
            uint8_t* d = (uint8_t*)left;
            dest_val = *d;
            (*d)--;
            result = *d;
        } else if (loplen == 2) {
            uint16_t* d = (uint16_t*)left;
            dest_val = *d;
            (*d)--;
            result = *d;
        } else if (loplen == 4) {
            uint32_t* d = (uint32_t*)left;
            dest_val = *d;
            (*d)--;
            result = *d;
        } else if (loplen == 8) {
            uint64_t* d = (uint64_t*)left;
            dest_val = *d;
            (*d)--;
            result = *d;
        }
        
        // Update CPU flags
        regs->rflags = 0;
        
        // Zero Flag (ZF)
        if (result == 0)
            regs->rflags |= 0x40;
        
        // Sign Flag (SF)
        if (loplen == 1 && (result & 0x80))
            regs->rflags |= 0x80;
        else if (loplen == 2 && (result & 0x8000))
            regs->rflags |= 0x80;
        else if (loplen == 4 && (result & 0x80000000))
            regs->rflags |= 0x80;
        else if (loplen == 8 && (result & 0x8000000000000000ULL))
            regs->rflags |= 0x80;
        
        // Overflow Flag (OF) - signed overflow (decrement can overflow from min to max)
        if (loplen == 1) {
            int8_t d8 = (int8_t)dest_val;
            int8_t r8 = (int8_t)result;
            if (d8 < 0 && r8 > 0)
                regs->rflags |= 0x800;
        } else if (loplen == 2) {
            int16_t d16 = (int16_t)dest_val;
            int16_t r16 = (int16_t)result;
            if (d16 < 0 && r16 > 0)
                regs->rflags |= 0x800;
        } else if (loplen == 4) {
            int32_t d32 = (int32_t)dest_val;
            int32_t r32 = (int32_t)result;
            if (d32 < 0 && r32 > 0)
                regs->rflags |= 0x800;
        } else if (loplen == 8) {
            int64_t d64 = (int64_t)dest_val;
            int64_t r64 = (int64_t)result;
            if (d64 < 0 && r64 > 0)
                regs->rflags |= 0x800;
        }
        
        // Parity Flag (PF) - only for low 8 bits
        uint8_t low_byte = result & 0xFF;
        int parity = 0;
        for (int i = 0; i < 8; i++) {
            if (low_byte & (1 << i))
                parity++;
        }
        if (parity % 2 == 0)
            regs->rflags |= 0x4;
    }
    else if (carch == x86) {
        lgr bufr;
        vectorGetValue(&simval, 0, &bufr);
        x86_Registers* regs = (x86_Registers*)bufr;
        if (left >= (uintptr_t)registers && left <= ((uintptr_t)registers + sizeof(x86_Registers)))
            x86_setupRegister(regs, left, &left, &loplen);

        // Perform decrement based on operand size
        uint64_t result = 0;
        uint64_t dest_val = 0;
        
        if (loplen == 1) {
            uint8_t* d = (uint8_t*)left;
            dest_val = *d;
            (*d)--;
            result = *d;
        } else if (loplen == 2) {
            uint16_t* d = (uint16_t*)left;
            dest_val = *d;
            (*d)--;
            result = *d;
        } else if (loplen == 4) {
            uint32_t* d = (uint32_t*)left;
            dest_val = *d;
            (*d)--;
            result = *d;
        } else if (loplen == 8) {
            uint64_t* d = (uint64_t*)left;
            dest_val = *d;
            (*d)--;
            result = *d;
        }
        
        // Update CPU flags
        regs->eflags = 0;
        
        // Zero Flag (ZF)
        if (result == 0)
            regs->eflags |= 0x40;
        
        // Sign Flag (SF)
        if (loplen == 1 && (result & 0x80))
            regs->eflags |= 0x80;
        else if (loplen == 2 && (result & 0x8000))
            regs->eflags |= 0x80;
        else if (loplen == 4 && (result & 0x80000000))
            regs->eflags |= 0x80;
        else if (loplen == 8 && (result & 0x8000000000000000ULL))
            regs->eflags |= 0x80;
        
        // Overflow Flag (OF) - signed overflow
        if (loplen == 1) {
            int8_t d8 = (int8_t)dest_val;
            int8_t r8 = (int8_t)result;
            if (d8 < 0 && r8 > 0)
                regs->eflags |= 0x800;
        } else if (loplen == 2) {
            int16_t d16 = (int16_t)dest_val;
            int16_t r16 = (int16_t)result;
            if (d16 < 0 && r16 > 0)
                regs->eflags |= 0x800;
        } else if (loplen == 4) {
            int32_t d32 = (int32_t)dest_val;
            int32_t r32 = (int32_t)result;
            if (d32 < 0 && r32 > 0)
                regs->eflags |= 0x800;
        } else if (loplen == 8) {
            int64_t d64 = (int64_t)dest_val;
            int64_t r64 = (int64_t)result;
            if (d64 < 0 && r64 > 0)
                regs->eflags |= 0x800;
        }
        
        // Parity Flag (PF) - only for low 8 bits
        uint8_t low_byte = result & 0xFF;
        int parity = 0;
        for (int i = 0; i < 8; i++) {
            if (low_byte & (1 << i))
                parity++;
        }
        if (parity % 2 == 0)
            regs->eflags |= 0x4;
    } else
        bugDetected("Unknown Archtecture");
    
    return PROC_SUCCESS;
}

ProcessorResult lgAnd(uintptr_t left, unsigned int loplen, uintptr_t right, unsigned int roplen,
                           vector eops, vector eopslen) {
    if (!isInit)
        return NOT_INITIALIZED;
    
    if (carch == x86_64) {
        lgr bufr;
        vectorGetValue(&simval, 0, &bufr);
        x86_64_Registers* regs = (x86_64_Registers*)bufr;
        if (left >= (uintptr_t)registers && left <= ((uintptr_t)registers + sizeof(x86_64_Registers)))
            x86_64_setupRegister(regs, left, &left, &loplen);
        if (right >= (uintptr_t)registers && right <= ((uintptr_t)registers + sizeof(x86_64_Registers)))
            x86_64_setupRegister(regs, right, &right, &roplen);
        
        // Perform AND based on operand size
        uint64_t result = 0;
        
        if (loplen == 1) {
            uint8_t* d = (uint8_t*)left;
            uint8_t* s = (uint8_t*)right;
            *d &= *s;
            result = *d;
        } else if (loplen == 2) {
            uint16_t* d = (uint16_t*)left;
            uint16_t* s = (uint16_t*)right;
            *d &= *s;
            result = *d;
        } else if (loplen == 4) {
            uint32_t* d = (uint32_t*)left;
            uint32_t* s = (uint32_t*)right;
            *d &= *s;
            result = *d;
        } else if (loplen == 8) {
            uint64_t* d = (uint64_t*)left;
            uint64_t* s = (uint64_t*)right;
            *d &= *s;
            result = *d;
        }
        
        // Update CPU flags
        regs->rflags = 0;
        
        // Zero Flag (ZF)
        if (result == 0)
            regs->rflags |= 0x40;
        
        // Sign Flag (SF)
        if (loplen == 1 && (result & 0x80))
            regs->rflags |= 0x80;
        else if (loplen == 2 && (result & 0x8000))
            regs->rflags |= 0x80;
        else if (loplen == 4 && (result & 0x80000000))
            regs->rflags |= 0x80;
        else if (loplen == 8 && (result & 0x8000000000000000ULL))
            regs->rflags |= 0x80;
        
        // Carry Flag (CF) - always cleared for AND
        regs->rflags &= ~0x1;
        
        // Overflow Flag (OF) - always cleared for AND
        regs->rflags &= ~0x800;
        
        // Parity Flag (PF) - only for low 8 bits
        uint8_t low_byte = result & 0xFF;
        int parity = 0;
        for (int i = 0; i < 8; i++) {
            if (low_byte & (1 << i))
                parity++;
        }
        if (parity % 2 == 0)
            regs->rflags |= 0x4;
    }
    else if (carch == x86) {
        lgr bufr;
        vectorGetValue(&simval, 0, &bufr);
        x86_Registers* regs = (x86_Registers*)bufr;
        if (left >= (uintptr_t)registers && left <= ((uintptr_t)registers + sizeof(x86_Registers)))
            x86_setupRegister(regs, left, &left, &loplen);
        if (right >= (uintptr_t)registers && right <= ((uintptr_t)registers + sizeof(x86_Registers)))
            x86_setupRegister(regs, right, &right, &roplen);

        // Perform AND based on operand size
        uint64_t result = 0;
        
        if (loplen == 1) {
            uint8_t* d = (uint8_t*)left;
            uint8_t* s = (uint8_t*)right;
            *d &= *s;
            result = *d;
        } else if (loplen == 2) {
            uint16_t* d = (uint16_t*)left;
            uint16_t* s = (uint16_t*)right;
            *d &= *s;
            result = *d;
        } else if (loplen == 4) {
            uint32_t* d = (uint32_t*)left;
            uint32_t* s = (uint32_t*)right;
            *d &= *s;
            result = *d;
        } else if (loplen == 8) {
            uint64_t* d = (uint64_t*)left;
            uint64_t* s = (uint64_t*)right;
            *d &= *s;
            result = *d;
        }
        
        // Update CPU flags
        regs->eflags = 0;
        
        // Zero Flag (ZF)
        if (result == 0)
            regs->eflags |= 0x40;
        
        // Sign Flag (SF)
        if (loplen == 1 && (result & 0x80))
            regs->eflags |= 0x80;
        else if (loplen == 2 && (result & 0x8000))
            regs->eflags |= 0x80;
        else if (loplen == 4 && (result & 0x80000000))
            regs->eflags |= 0x80;
        else if (loplen == 8 && (result & 0x8000000000000000ULL))
            regs->eflags |= 0x80;
        
        // Carry Flag (CF) - always cleared for AND
        regs->eflags &= ~0x1;
        
        // Overflow Flag (OF) - always cleared for AND
        regs->eflags &= ~0x800;
        
        // Parity Flag (PF) - only for low 8 bits
        uint8_t low_byte = result & 0xFF;
        int parity = 0;
        for (int i = 0; i < 8; i++) {
            if (low_byte & (1 << i))
                parity++;
        }
        if (parity % 2 == 0)
            regs->eflags |= 0x4;
    } else
        bugDetected("Unknown Archtecture");
    
    return PROC_SUCCESS;
}

ProcessorResult lgOr(uintptr_t left, unsigned int loplen, uintptr_t right, unsigned int roplen,
                          vector eops, vector eopslen) {
    if (!isInit)
        return NOT_INITIALIZED;
    
    if (carch == x86_64) {
        lgr bufr;
        vectorGetValue(&simval, 0, &bufr);
        x86_64_Registers* regs = (x86_64_Registers*)bufr;
        if (left >= (uintptr_t)registers && left <= ((uintptr_t)registers + sizeof(x86_64_Registers)))
            x86_64_setupRegister(regs, left, &left, &loplen);
        if (right >= (uintptr_t)registers && right <= ((uintptr_t)registers + sizeof(x86_64_Registers)))
            x86_64_setupRegister(regs, right, &right, &roplen);
        
        // Perform OR based on operand size
        uint64_t result = 0;
        
        if (loplen == 1) {
            uint8_t* d = (uint8_t*)left;
            uint8_t* s = (uint8_t*)right;
            *d |= *s;
            result = *d;
        } else if (loplen == 2) {
            uint16_t* d = (uint16_t*)left;
            uint16_t* s = (uint16_t*)right;
            *d |= *s;
            result = *d;
        } else if (loplen == 4) {
            uint32_t* d = (uint32_t*)left;
            uint32_t* s = (uint32_t*)right;
            *d |= *s;
            result = *d;
        } else if (loplen == 8) {
            uint64_t* d = (uint64_t*)left;
            uint64_t* s = (uint64_t*)right;
            *d |= *s;
            result = *d;
        }
        
        // Update CPU flags
        regs->rflags = 0;
        
        // Zero Flag (ZF)
        if (result == 0)
            regs->rflags |= 0x40;
        
        // Sign Flag (SF)
        if (loplen == 1 && (result & 0x80))
            regs->rflags |= 0x80;
        else if (loplen == 2 && (result & 0x8000))
            regs->rflags |= 0x80;
        else if (loplen == 4 && (result & 0x80000000))
            regs->rflags |= 0x80;
        else if (loplen == 8 && (result & 0x8000000000000000ULL))
            regs->rflags |= 0x80;
        
        // Carry Flag (CF) - always cleared for OR
        regs->rflags &= ~0x1;
        
        // Overflow Flag (OF) - always cleared for OR
        regs->rflags &= ~0x800;
        
        // Parity Flag (PF) - only for low 8 bits
        uint8_t low_byte = result & 0xFF;
        int parity = 0;
        for (int i = 0; i < 8; i++) {
            if (low_byte & (1 << i))
                parity++;
        }
        if (parity % 2 == 0)
            regs->rflags |= 0x4;
    }
    else if (carch == x86) {
        lgr bufr;
        vectorGetValue(&simval, 0, &bufr);
        x86_Registers* regs = (x86_Registers*)bufr;
        if (left >= (uintptr_t)registers && left <= ((uintptr_t)registers + sizeof(x86_Registers)))
            x86_setupRegister(regs, left, &left, &loplen);
        if (right >= (uintptr_t)registers && right <= ((uintptr_t)registers + sizeof(x86_Registers)))
            x86_setupRegister(regs, right, &right, &roplen);

        // Perform OR based on operand size
        uint64_t result = 0;
        
        if (loplen == 1) {
            uint8_t* d = (uint8_t*)left;
            uint8_t* s = (uint8_t*)right;
            *d |= *s;
            result = *d;
        } else if (loplen == 2) {
            uint16_t* d = (uint16_t*)left;
            uint16_t* s = (uint16_t*)right;
            *d |= *s;
            result = *d;
        } else if (loplen == 4) {
            uint32_t* d = (uint32_t*)left;
            uint32_t* s = (uint32_t*)right;
            *d |= *s;
            result = *d;
        } else if (loplen == 8) {
            uint64_t* d = (uint64_t*)left;
            uint64_t* s = (uint64_t*)right;
            *d |= *s;
            result = *d;
        }
        
        // Update CPU flags
        regs->eflags = 0;
        
        // Zero Flag (ZF)
        if (result == 0)
            regs->eflags |= 0x40;
        
        // Sign Flag (SF)
        if (loplen == 1 && (result & 0x80))
            regs->eflags |= 0x80;
        else if (loplen == 2 && (result & 0x8000))
            regs->eflags |= 0x80;
        else if (loplen == 4 && (result & 0x80000000))
            regs->eflags |= 0x80;
        else if (loplen == 8 && (result & 0x8000000000000000ULL))
            regs->eflags |= 0x80;
        
        // Carry Flag (CF) - always cleared for OR
        regs->eflags &= ~0x1;
        
        // Overflow Flag (OF) - always cleared for OR
        regs->eflags &= ~0x800;
        
        // Parity Flag (PF) - only for low 8 bits
        uint8_t low_byte = result & 0xFF;
        int parity = 0;
        for (int i = 0; i < 8; i++) {
            if (low_byte & (1 << i))
                parity++;
        }
        if (parity % 2 == 0)
            regs->eflags |= 0x4;
    } else
        bugDetected("Unknown Archtecture");
    
    return PROC_SUCCESS;
}

ProcessorResult lgXor(uintptr_t left, unsigned int loplen, uintptr_t right, unsigned int roplen,
                           vector eops, vector eopslen) {
    if (!isInit)
        return NOT_INITIALIZED;
    
    if (carch == x86_64) {
        lgr bufr;
        vectorGetValue(&simval, 0, &bufr);
        x86_64_Registers* regs = (x86_64_Registers*)bufr;
        if (left >= (uintptr_t)registers && left <= ((uintptr_t)registers + sizeof(x86_64_Registers)))
            x86_64_setupRegister(regs, left, &left, &loplen);
        if (right >= (uintptr_t)registers && right <= ((uintptr_t)registers + sizeof(x86_64_Registers)))
            x86_64_setupRegister(regs, right, &right, &roplen);
        
        // Perform XOR based on operand size
        uint64_t result = 0;
        
        if (loplen == 1) {
            uint8_t* d = (uint8_t*)left;
            uint8_t* s = (uint8_t*)right;
            *d ^= *s;
            result = *d;
        } else if (loplen == 2) {
            uint16_t* d = (uint16_t*)left;
            uint16_t* s = (uint16_t*)right;
            *d ^= *s;
            result = *d;
        } else if (loplen == 4) {
            uint32_t* d = (uint32_t*)left;
            uint32_t* s = (uint32_t*)right;
            *d ^= *s;
            result = *d;
        } else if (loplen == 8) {
            uint64_t* d = (uint64_t*)left;
            uint64_t* s = (uint64_t*)right;
            *d ^= *s;
            result = *d;
        }
        
        // Update CPU flags
        regs->rflags = 0;
        
        // Zero Flag (ZF)
        if (result == 0)
            regs->rflags |= 0x40;
        
        // Sign Flag (SF)
        if (loplen == 1 && (result & 0x80))
            regs->rflags |= 0x80;
        else if (loplen == 2 && (result & 0x8000))
            regs->rflags |= 0x80;
        else if (loplen == 4 && (result & 0x80000000))
            regs->rflags |= 0x80;
        else if (loplen == 8 && (result & 0x8000000000000000ULL))
            regs->rflags |= 0x80;
        
        // Carry Flag (CF) - always cleared for XOR
        regs->rflags &= ~0x1;
        
        // Overflow Flag (OF) - always cleared for XOR
        regs->rflags &= ~0x800;
        
        // Parity Flag (PF) - only for low 8 bits
        uint8_t low_byte = result & 0xFF;
        int parity = 0;
        for (int i = 0; i < 8; i++) {
            if (low_byte & (1 << i))
                parity++;
        }
        if (parity % 2 == 0)
            regs->rflags |= 0x4;
    }
    else if (carch == x86) {
        lgr bufr;
        vectorGetValue(&simval, 0, &bufr);
        x86_Registers* regs = (x86_Registers*)bufr;
        if (left >= (uintptr_t)registers && left <= ((uintptr_t)registers + sizeof(x86_Registers)))
            x86_setupRegister(regs, left, &left, &loplen);
        if (right >= (uintptr_t)registers && right <= ((uintptr_t)registers + sizeof(x86_Registers)))
            x86_setupRegister(regs, right, &right, &roplen);

        // Perform XOR based on operand size
        uint64_t result = 0;
        
        if (loplen == 1) {
            uint8_t* d = (uint8_t*)left;
            uint8_t* s = (uint8_t*)right;
            *d ^= *s;
            result = *d;
        } else if (loplen == 2) {
            uint16_t* d = (uint16_t*)left;
            uint16_t* s = (uint16_t*)right;
            *d ^= *s;
            result = *d;
        } else if (loplen == 4) {
            uint32_t* d = (uint32_t*)left;
            uint32_t* s = (uint32_t*)right;
            *d ^= *s;
            result = *d;
        } else if (loplen == 8) {
            uint64_t* d = (uint64_t*)left;
            uint64_t* s = (uint64_t*)right;
            *d ^= *s;
            result = *d;
        }
        
        // Update CPU flags
        regs->eflags = 0;
        
        // Zero Flag (ZF)
        if (result == 0)
            regs->eflags |= 0x40;
        
        // Sign Flag (SF)
        if (loplen == 1 && (result & 0x80))
            regs->eflags |= 0x80;
        else if (loplen == 2 && (result & 0x8000))
            regs->eflags |= 0x80;
        else if (loplen == 4 && (result & 0x80000000))
            regs->eflags |= 0x80;
        else if (loplen == 8 && (result & 0x8000000000000000ULL))
            regs->eflags |= 0x80;
        
        // Carry Flag (CF) - always cleared for XOR
        regs->eflags &= ~0x1;
        
        // Overflow Flag (OF) - always cleared for XOR
        regs->eflags &= ~0x800;
        
        // Parity Flag (PF) - only for low 8 bits
        uint8_t low_byte = result & 0xFF;
        int parity = 0;
        for (int i = 0; i < 8; i++) {
            if (low_byte & (1 << i))
                parity++;
        }
        if (parity % 2 == 0)
            regs->eflags |= 0x4;
    } else
        bugDetected("Unknown Archtecture");
    
    return PROC_SUCCESS;
}

ProcessorResult lgNot(uintptr_t left, unsigned int loplen, uintptr_t right, unsigned int roplen,
                           vector eops, vector eopslen) {
    if (!isInit)
        return NOT_INITIALIZED;
    
    if (carch == x86_64) {
        lgr bufr;
        vectorGetValue(&simval, 0, &bufr);
        x86_64_Registers* regs = (x86_64_Registers*)bufr;
        if (left >= (uintptr_t)registers && left <= ((uintptr_t)registers + sizeof(x86_64_Registers)))
            x86_64_setupRegister(regs, left, &left, &loplen);
        
        // Perform NOT based on operand size
        uint64_t result = 0;
        
        if (loplen == 1) {
            uint8_t* d = (uint8_t*)left;
            *d = ~(*d);
            result = *d;
        } else if (loplen == 2) {
            uint16_t* d = (uint16_t*)left;
            *d = ~(*d);
            result = *d;
        } else if (loplen == 4) {
            uint32_t* d = (uint32_t*)left;
            *d = ~(*d);
            result = *d;
        } else if (loplen == 8) {
            uint64_t* d = (uint64_t*)left;
            *d = ~(*d);
            result = *d;
        }
        
        // Update CPU flags
        regs->rflags = 0;
        
        // Zero Flag (ZF)
        if (result == 0)
            regs->rflags |= 0x40;
        
        // Sign Flag (SF)
        if (loplen == 1 && (result & 0x80))
            regs->rflags |= 0x80;
        else if (loplen == 2 && (result & 0x8000))
            regs->rflags |= 0x80;
        else if (loplen == 4 && (result & 0x80000000))
            regs->rflags |= 0x80;
        else if (loplen == 8 && (result & 0x8000000000000000ULL))
            regs->rflags |= 0x80;
        
        // Carry Flag (CF) - always cleared for NOT
        regs->rflags &= ~0x1;
        
        // Overflow Flag (OF) - always cleared for NOT
        regs->rflags &= ~0x800;
        
        // Parity Flag (PF) - only for low 8 bits
        uint8_t low_byte = result & 0xFF;
        int parity = 0;
        for (int i = 0; i < 8; i++) {
            if (low_byte & (1 << i))
                parity++;
        }
        if (parity % 2 == 0)
            regs->rflags |= 0x4;
    }
    else if (carch == x86) {
        lgr bufr;
        vectorGetValue(&simval, 0, &bufr);
        x86_Registers* regs = (x86_Registers*)bufr;
        if (left >= (uintptr_t)registers && left <= ((uintptr_t)registers + sizeof(x86_Registers)))
            x86_setupRegister(regs, left, &left, &loplen);

        // Perform NOT based on operand size
        uint64_t result = 0;
        
        if (loplen == 1) {
            uint8_t* d = (uint8_t*)left;
            *d = ~(*d);
            result = *d;
        } else if (loplen == 2) {
            uint16_t* d = (uint16_t*)left;
            *d = ~(*d);
            result = *d;
        } else if (loplen == 4) {
            uint32_t* d = (uint32_t*)left;
            *d = ~(*d);
            result = *d;
        } else if (loplen == 8) {
            uint64_t* d = (uint64_t*)left;
            *d = ~(*d);
            result = *d;
        }
        
        // Update CPU flags
        regs->eflags = 0;
        
        // Zero Flag (ZF)
        if (result == 0)
            regs->eflags |= 0x40;
        
        // Sign Flag (SF)
        if (loplen == 1 && (result & 0x80))
            regs->eflags |= 0x80;
        else if (loplen == 2 && (result & 0x8000))
            regs->eflags |= 0x80;
        else if (loplen == 4 && (result & 0x80000000))
            regs->eflags |= 0x80;
        else if (loplen == 8 && (result & 0x8000000000000000ULL))
            regs->eflags |= 0x80;
        
        // Carry Flag (CF) - always cleared for NOT
        regs->eflags &= ~0x1;
        
        // Overflow Flag (OF) - always cleared for NOT
        regs->eflags &= ~0x800;
        
        // Parity Flag (PF) - only for low 8 bits
        uint8_t low_byte = result & 0xFF;
        int parity = 0;
        for (int i = 0; i < 8; i++) {
            if (low_byte & (1 << i))
                parity++;
        }
        if (parity % 2 == 0)
            regs->eflags |= 0x4;
    } else
        bugDetected("Unknown Archtecture");
    
    return PROC_SUCCESS;
}

ProcessorResult shift_left(uintptr_t left, unsigned int loplen, uintptr_t right, unsigned int roplen) {
    if (!isInit)
        return NOT_INITIALIZED;
    
    if (carch == x86_64) {
        lgr bufr;
        vectorGetValue(&simval, 0, &bufr);
        x86_64_Registers* regs = (x86_64_Registers*)bufr;
        if (left >= (uintptr_t)registers && left <= ((uintptr_t)registers + sizeof(x86_64_Registers)))
            x86_64_setupRegister(regs, left, &left, &loplen);
        if (right >= (uintptr_t)registers && right <= ((uintptr_t)registers + sizeof(x86_64_Registers)))
            x86_64_setupRegister(regs, right, &right, &roplen);
        
        // Get shift count (mask to operand size)
        uint64_t shift_count = 0;
        if (roplen == 1)
            shift_count = *(uint8_t*)right & 0x3F;
        else if (roplen == 2)
            shift_count = *(uint16_t*)right & 0x3F;
        else if (roplen == 4)
            shift_count = *(uint32_t*)right & 0x3F;
        else if (roplen == 8)
            shift_count = *(uint64_t*)right & 0x3F;
        
        // Perform shift based on operand size
        uint64_t result = 0;
        uint64_t dest_val = 0;
        
        if (loplen == 1) {
            uint8_t* d = (uint8_t*)left;
            dest_val = *d;
            if (shift_count < 8) {
                *d <<= shift_count;
                result = *d;
            } else {
                *d = 0;
                result = 0;
            }
        } else if (loplen == 2) {
            uint16_t* d = (uint16_t*)left;
            dest_val = *d;
            if (shift_count < 16) {
                *d <<= shift_count;
                result = *d;
            } else {
                *d = 0;
                result = 0;
            }
        } else if (loplen == 4) {
            uint32_t* d = (uint32_t*)left;
            dest_val = *d;
            if (shift_count < 32) {
                *d <<= shift_count;
                result = *d;
            } else {
                *d = 0;
                result = 0;
            }
        } else if (loplen == 8) {
            uint64_t* d = (uint64_t*)left;
            dest_val = *d;
            if (shift_count < 64) {
                *d <<= shift_count;
                result = *d;
            } else {
                *d = 0;
                result = 0;
            }
        }
        
        // Update CPU flags
        regs->rflags = 0;
        
        // Zero Flag (ZF)
        if (result == 0)
            regs->rflags |= 0x40;
        
        // Sign Flag (SF)
        if (loplen == 1 && (result & 0x80))
            regs->rflags |= 0x80;
        else if (loplen == 2 && (result & 0x8000))
            regs->rflags |= 0x80;
        else if (loplen == 4 && (result & 0x80000000))
            regs->rflags |= 0x80;
        else if (loplen == 8 && (result & 0x8000000000000000ULL))
            regs->rflags |= 0x80;
        
        // Carry Flag (CF) - last bit shifted out
        if (shift_count > 0 && shift_count <= loplen * 8) {
            if ((dest_val >> (loplen * 8 - shift_count)) & 1)
                regs->rflags |= 0x1;
        }
        
        // Overflow Flag (OF) - only defined for single-bit shifts
        if (shift_count == 1) {
            if (loplen == 1) {
                if ((dest_val & 0x40) != ((result & 0x80) >> 1))
                    regs->rflags |= 0x800;
            } else if (loplen == 2) {
                if ((dest_val & 0x4000) != ((result & 0x8000) >> 1))
                    regs->rflags |= 0x800;
            } else if (loplen == 4) {
                if ((dest_val & 0x40000000) != ((result & 0x80000000) >> 1))
                    regs->rflags |= 0x800;
            } else if (loplen == 8) {
                if ((dest_val & 0x4000000000000000ULL) != ((result & 0x8000000000000000ULL) >> 1))
                    regs->rflags |= 0x800;
            }
        }
        
        // Parity Flag (PF) - only for low 8 bits
        uint8_t low_byte = result & 0xFF;
        int parity = 0;
        for (int i = 0; i < 8; i++) {
            if (low_byte & (1 << i))
                parity++;
        }
        if (parity % 2 == 0)
            regs->rflags |= 0x4;
    }
    else if (carch == x86) {
        lgr bufr;
        vectorGetValue(&simval, 0, &bufr);
        x86_Registers* regs = (x86_Registers*)bufr;
        if (left >= (uintptr_t)registers && left <= ((uintptr_t)registers + sizeof(x86_Registers)))
            x86_setupRegister(regs, left, &left, &loplen);
        if (right >= (uintptr_t)registers && right <= ((uintptr_t)registers + sizeof(x86_Registers)))
            x86_setupRegister(regs, right, &right, &roplen);

        // Get shift count (mask to operand size)
        uint64_t shift_count = 0;
        if (roplen == 1)
            shift_count = *(uint8_t*)right & 0x3F;
        else if (roplen == 2)
            shift_count = *(uint16_t*)right & 0x3F;
        else if (roplen == 4)
            shift_count = *(uint32_t*)right & 0x3F;
        else if (roplen == 8)
            shift_count = *(uint64_t*)right & 0x3F;
        
        // Perform shift based on operand size
        uint64_t result = 0;
        uint64_t dest_val = 0;
        
        if (loplen == 1) {
            uint8_t* d = (uint8_t*)left;
            dest_val = *d;
            if (shift_count < 8) {
                *d <<= shift_count;
                result = *d;
            } else {
                *d = 0;
                result = 0;
            }
        } else if (loplen == 2) {
            uint16_t* d = (uint16_t*)left;
            dest_val = *d;
            if (shift_count < 16) {
                *d <<= shift_count;
                result = *d;
            } else {
                *d = 0;
                result = 0;
            }
        } else if (loplen == 4) {
            uint32_t* d = (uint32_t*)left;
            dest_val = *d;
            if (shift_count < 32) {
                *d <<= shift_count;
                result = *d;
            } else {
                *d = 0;
                result = 0;
            }
        } else if (loplen == 8) {
            uint64_t* d = (uint64_t*)left;
            dest_val = *d;
            if (shift_count < 64) {
                *d <<= shift_count;
                result = *d;
            } else {
                *d = 0;
                result = 0;
            }
        }
        
        // Update CPU flags
        regs->eflags = 0;
        
        // Zero Flag (ZF)
        if (result == 0)
            regs->eflags |= 0x40;
        
        // Sign Flag (SF)
        if (loplen == 1 && (result & 0x80))
            regs->eflags |= 0x80;
        else if (loplen == 2 && (result & 0x8000))
            regs->eflags |= 0x80;
        else if (loplen == 4 && (result & 0x80000000))
            regs->eflags |= 0x80;
        else if (loplen == 8 && (result & 0x8000000000000000ULL))
            regs->eflags |= 0x80;
        
        // Carry Flag (CF) - last bit shifted out
        if (shift_count > 0 && shift_count <= loplen * 8)
            if ((dest_val >> (loplen * 8 - shift_count)) & 1)
                regs->eflags |= 0x1;
        
        // Overflow Flag (OF) - only defined for single-bit shifts
        if (shift_count == 1) {
            if (loplen == 1) {
                if ((dest_val & 0x40) != ((result & 0x80) >> 1))
                    regs->eflags |= 0x800;
            } else if (loplen == 2) {
                if ((dest_val & 0x4000) != ((result & 0x8000) >> 1))
                    regs->eflags |= 0x800;
            } else if (loplen == 4) {
                if ((dest_val & 0x40000000) != ((result & 0x80000000) >> 1))
                    regs->eflags |= 0x800;
            } else if (loplen == 8) {
                if ((dest_val & 0x4000000000000000ULL) != ((result & 0x8000000000000000ULL) >> 1))
                    regs->eflags |= 0x800;
            }
        }
        
        // Parity Flag (PF) - only for low 8 bits
        uint8_t low_byte = result & 0xFF;
        int parity = 0;
        for (int i = 0; i < 8; i++) {
            if (low_byte & (1 << i))
                parity++;
        }
        if (parity % 2 == 0)
            regs->eflags |= 0x4;
    } else
        bugDetected("Unknown Archtecture");
    
    return PROC_SUCCESS;
}

ProcessorResult shift_right(uintptr_t left, unsigned int loplen, uintptr_t right, unsigned int roplen) {
    if (!isInit)
        return NOT_INITIALIZED;
    
    if (carch == x86_64) {
        lgr bufr;
        vectorGetValue(&simval, 0, &bufr);
        x86_64_Registers* regs = (x86_64_Registers*)bufr;
        if (left >= (uintptr_t)registers && left <= ((uintptr_t)registers + sizeof(x86_64_Registers)))
            x86_64_setupRegister(regs, left, &left, &loplen);
        if (right >= (uintptr_t)registers && right <= ((uintptr_t)registers + sizeof(x86_64_Registers)))
            x86_64_setupRegister(regs, right, &right, &roplen);
        
        // Get shift count (mask to operand size)
        uint64_t shift_count = 0;
        if (roplen == 1)
            shift_count = *(uint8_t*)right & 0x3F;
        else if (roplen == 2)
            shift_count = *(uint16_t*)right & 0x3F;
        else if (roplen == 4)
            shift_count = *(uint32_t*)right & 0x3F;
        else if (roplen == 8)
            shift_count = *(uint64_t*)right & 0x3F;
        
        // Perform logical shift right based on operand size
        uint64_t result = 0;
        uint64_t dest_val = 0;
        
        if (loplen == 1) {
            uint8_t* d = (uint8_t*)left;
            dest_val = *d;
            if (shift_count < 8) {
                *d >>= shift_count;
                result = *d;
            } else {
                *d = 0;
                result = 0;
            }
        } else if (loplen == 2) {
            uint16_t* d = (uint16_t*)left;
            dest_val = *d;
            if (shift_count < 16) {
                *d >>= shift_count;
                result = *d;
            } else {
                *d = 0;
                result = 0;
            }
        } else if (loplen == 4) {
            uint32_t* d = (uint32_t*)left;
            dest_val = *d;
            if (shift_count < 32) {
                *d >>= shift_count;
                result = *d;
            } else {
                *d = 0;
                result = 0;
            }
        } else if (loplen == 8) {
            uint64_t* d = (uint64_t*)left;
            dest_val = *d;
            if (shift_count < 64) {
                *d >>= shift_count;
                result = *d;
            } else {
                *d = 0;
                result = 0;
            }
        }
        
        // Update CPU flags
        regs->rflags = 0;
        
        // Zero Flag (ZF)
        if (result == 0)
            regs->rflags |= 0x40;
        
        // Sign Flag (SF)
        if (loplen == 1 && (result & 0x80))
            regs->rflags |= 0x80;
        else if (loplen == 2 && (result & 0x8000))
            regs->rflags |= 0x80;
        else if (loplen == 4 && (result & 0x80000000))
            regs->rflags |= 0x80;
        else if (loplen == 8 && (result & 0x8000000000000000ULL))
            regs->rflags |= 0x80;
        
        // Carry Flag (CF) - last bit shifted out
        if (shift_count > 0 && shift_count <= loplen * 8) {
            if ((dest_val >> (shift_count - 1)) & 1)
                regs->rflags |= 0x1;
        }
        
        // Overflow Flag (OF) - only defined for single-bit shifts
        if (shift_count == 1) {
            if (loplen == 1) {
                if (dest_val & 0x80)
                    regs->rflags |= 0x800;
            } else if (loplen == 2) {
                if (dest_val & 0x8000)
                    regs->rflags |= 0x800;
            } else if (loplen == 4) {
                if (dest_val & 0x80000000)
                    regs->rflags |= 0x800;
            } else if (loplen == 8) {
                if (dest_val & 0x8000000000000000ULL)
                    regs->rflags |= 0x800;
            }
        }
        
        // Parity Flag (PF) - only for low 8 bits
        uint8_t low_byte = result & 0xFF;
        int parity = 0;
        for (int i = 0; i < 8; i++)
            if (low_byte & (1 << i))
                parity++;
        if (parity % 2 == 0)
            regs->rflags |= 0x4;
    }
    else if (carch == x86) {
        lgr bufr;
        vectorGetValue(&simval, 0, &bufr);
        x86_Registers* regs = (x86_Registers*)bufr;
        if (left >= (uintptr_t)registers && left <= ((uintptr_t)registers + sizeof(x86_Registers)))
            x86_setupRegister(regs, left, &left, &loplen);
        if (right >= (uintptr_t)registers && right <= ((uintptr_t)registers + sizeof(x86_Registers)))
            x86_setupRegister(regs, right, &right, &roplen);

        // Get shift count (mask to operand size)
        uint64_t shift_count = 0;
        if (roplen == 1)
            shift_count = *(uint8_t*)right & 0x3F;
        else if (roplen == 2)
            shift_count = *(uint16_t*)right & 0x3F;
        else if (roplen == 4)
            shift_count = *(uint32_t*)right & 0x3F;
        else if (roplen == 8)
            shift_count = *(uint64_t*)right & 0x3F;
        
        // Perform logical shift right based on operand size
        uint64_t result = 0;
        uint64_t dest_val = 0;
        
        if (loplen == 1) {
            uint8_t* d = (uint8_t*)left;
            dest_val = *d;
            if (shift_count < 8) {
                *d >>= shift_count;
                result = *d;
            } else {
                *d = 0;
                result = 0;
            }
        } else if (loplen == 2) {
            uint16_t* d = (uint16_t*)left;
            dest_val = *d;
            if (shift_count < 16) {
                *d >>= shift_count;
                result = *d;
            } else {
                *d = 0;
                result = 0;
            }
        } else if (loplen == 4) {
            uint32_t* d = (uint32_t*)left;
            dest_val = *d;
            if (shift_count < 32) {
                *d >>= shift_count;
                result = *d;
            } else {
                *d = 0;
                result = 0;
            }
        } else if (loplen == 8) {
            uint64_t* d = (uint64_t*)left;
            dest_val = *d;
            if (shift_count < 64) {
                *d >>= shift_count;
                result = *d;
            } else {
                *d = 0;
                result = 0;
            }
        }
        
        // Update CPU flags
        regs->eflags = 0;
        
        // Zero Flag (ZF)
        if (result == 0)
            regs->eflags |= 0x40;
        
        // Sign Flag (SF)
        if (loplen == 1 && (result & 0x80))
            regs->eflags |= 0x80;
        else if (loplen == 2 && (result & 0x8000))
            regs->eflags |= 0x80;
        else if (loplen == 4 && (result & 0x80000000))
            regs->eflags |= 0x80;
        else if (loplen == 8 && (result & 0x8000000000000000ULL))
            regs->eflags |= 0x80;
        
        // Carry Flag (CF) - last bit shifted out
        if (shift_count > 0 && shift_count <= loplen * 8)
            if ((dest_val >> (shift_count - 1)) & 1)
                regs->eflags |= 0x1;
        
        // Overflow Flag (OF) - only defined for single-bit shifts
        if (shift_count == 1) {
            if (loplen == 1) {
                if (dest_val & 0x80)
                    regs->eflags |= 0x800;
            } else if (loplen == 2) {
                if (dest_val & 0x8000)
                    regs->eflags |= 0x800;
            } else if (loplen == 4) {
                if (dest_val & 0x80000000)
                    regs->eflags |= 0x800;
            } else if (loplen == 8) {
                if (dest_val & 0x8000000000000000ULL)
                    regs->eflags |= 0x800;
            }
        }
        
        // Parity Flag (PF) - only for low 8 bits
        uint8_t low_byte = result & 0xFF;
        int parity = 0;
        for (int i = 0; i < 8; i++) {
            if (low_byte & (1 << i))
                parity++;
        }
        if (parity % 2 == 0)
            regs->eflags |= 0x4;
    } else
        bugDetected("Unknown Archtecture");
    
    return PROC_SUCCESS;
}

ProcessorResult push(uintptr_t left, unsigned int loplen) {
    if (!isInit)
        return NOT_INITIALIZED;
    
    if (carch == x86_64) {
        lgr bufr;
        vectorGetValue(&simval, 0, &bufr);
        x86_64_Registers* regs = (x86_64_Registers*)bufr;
        if (left >= (uintptr_t)registers && left <= ((uintptr_t)registers + sizeof(x86_64_Registers)))
            x86_64_setupRegister(regs, left, &left, &loplen);
        
        // Get value to push
        uint64_t value = 0;
        if (loplen == 1)
            value = *(uint8_t*)left;
        else if (loplen == 2)
            value = *(uint16_t*)left;
        else if (loplen == 4)
            value = *(uint32_t*)left;
        else if (loplen == 8)
            value = *(uint64_t*)left;
        
        // Decrement stack pointer and push value (push is always 8 bytes in 64-bit mode)
        regs->rsp -= 8;
        *(uint64_t*)(regs->rsp) = value;
        
        // PUSH does not affect flags in x86
    }
    else if (carch == x86) {
        lgr bufr;
        vectorGetValue(&simval, 0, &bufr);
        x86_Registers* regs = (x86_Registers*)bufr;
        if (left >= (uintptr_t)registers && left <= ((uintptr_t)registers + sizeof(x86_Registers)))
            x86_setupRegister(regs, left, &left, &loplen);

        // Get value to push
        uint64_t value = 0;
        if (loplen == 1)
            value = *(uint8_t*)left;
        else if (loplen == 2)
            value = *(uint16_t*)left;
        else if (loplen == 4)
            value = *(uint32_t*)left;
        else if (loplen == 8)
            value = *(uint64_t*)left;
        
        // Decrement stack pointer and push value (push size depends on operand size in 32-bit mode)
        if (loplen == 2) {
            regs->esp -= 2;
            *(uint16_t*)(regs->esp) = (uint16_t)value;
        } else {
            // Default to 4 bytes for 32-bit mode
            regs->esp -= 4;
            *(uint32_t*)(regs->esp) = (uint32_t)value;
        }
        
        // PUSH does not affect flags in x86
    } else
        bugDetected("Unknown Archtecture");
    
    return PROC_SUCCESS;
}

ProcessorResult pop(uintptr_t left, unsigned int loplen) {
    if (!isInit)
        return NOT_INITIALIZED;
    
    if (carch == x86_64) {
        lgr bufr;
        vectorGetValue(&simval, 0, &bufr);
        x86_64_Registers* regs = (x86_64_Registers*)bufr;
        if (left >= (uintptr_t)registers && left <= ((uintptr_t)registers + sizeof(x86_64_Registers)))
            x86_64_setupRegister(regs, left, &left, &loplen);
        
        // Pop value from stack (pop is always 8 bytes in 64-bit mode)
        uint64_t value = *(uint64_t*)(regs->rsp);
        regs->rsp += 8;
        
        // Store value in destination
        if (loplen == 1)
            *(uint8_t*)left = (uint8_t)value;
        else if (loplen == 2)
            *(uint16_t*)left = (uint16_t)value;
        else if (loplen == 4)
            *(uint32_t*)left = (uint32_t)value;
        else if (loplen == 8)
            *(uint64_t*)left = value;
        
        // POP does not affect flags in x86
    }
    else if (carch == x86) {
        lgr bufr;
        vectorGetValue(&simval, 0, &bufr);
        x86_Registers* regs = (x86_Registers*)bufr;
        if (left >= (uintptr_t)registers && left <= ((uintptr_t)registers + sizeof(x86_Registers)))
            x86_setupRegister(regs, left, &left, &loplen);

        // Pop value from stack (pop size depends on operand size in 32-bit mode)
        uint64_t value = 0;
        if (loplen == 2) {
            value = *(uint16_t*)(regs->esp);
            regs->esp += 2;
        } else {
            // Default to 4 bytes for 32-bit mode
            value = *(uint32_t*)(regs->esp);
            regs->esp += 4;
        }
        
        // Store value in destination
        if (loplen == 1)
            *(uint8_t*)left = (uint8_t)value;
        else if (loplen == 2)
            *(uint16_t*)left = (uint16_t)value;
        else if (loplen == 4)
            *(uint32_t*)left = (uint32_t)value;
        else if (loplen == 8)
            *(uint64_t*)left = value;
        
        // POP does not affect flags in x86
    } else
        bugDetected("Unknown Archtecture");
    
    return PROC_SUCCESS;
}

ProcessorResult load_effective_address(uintptr_t left, unsigned int loplen, uintptr_t right, unsigned int roplen) {
    if (!isInit)
        return NOT_INITIALIZED;
    
    if (carch == x86_64) {
        lgr bufr;
        vectorGetValue(&simval, 0, &bufr);
        x86_64_Registers* regs = (x86_64_Registers*)bufr;
        if (left >= (uintptr_t)registers && left <= ((uintptr_t)registers + sizeof(x86_64_Registers)))
            x86_64_setupRegister(regs, left, &left, &loplen);
        
        // LEA stores the address (right) into the destination (left)
        // For simplicity, we treat right as the effective address to load
        if (loplen == 8)
            *(uint64_t*)left = right;
        else if (loplen == 4)
            *(uint32_t*)left = (uint32_t)right;
        
        // LEA does not affect flags in x86
    }
    else if (carch == x86) {
        lgr bufr;
        vectorGetValue(&simval, 0, &bufr);
        x86_Registers* regs = (x86_Registers*)bufr;
        if (left >= (uintptr_t)registers && left <= ((uintptr_t)registers + sizeof(x86_Registers)))
            x86_setupRegister(regs, left, &left, &loplen);

        // LEA stores the address (right) into the destination (left)
        if (loplen == 4)
            *(uint32_t*)left = (uint32_t)right;
        else if (loplen == 2)
            *(uint16_t*)left = (uint16_t)right;
        
        // LEA does not affect flags in x86
    } else
        bugDetected("Unknown Archtecture");
    
    return PROC_SUCCESS;
}

ProcessorResult test_and(uintptr_t left, unsigned int loplen, uintptr_t right, unsigned int roplen,
                         vector eops, vector eopslen) {
    if (!isInit)
        return NOT_INITIALIZED;
    
    if (carch == x86_64) {
        lgr bufr;
        vectorGetValue(&simval, 0, &bufr);
        x86_64_Registers* regs = (x86_64_Registers*)bufr;
        if (left >= (uintptr_t)registers && left <= ((uintptr_t)registers + sizeof(x86_64_Registers)))
            x86_64_setupRegister(regs, left, &left, &loplen);
        if (right >= (uintptr_t)registers && right <= ((uintptr_t)registers + sizeof(x86_64_Registers)))
            x86_64_setupRegister(regs, right, &right, &roplen);
        
        // Perform AND operation but don't store result (TEST instruction)
        uint64_t result = 0;
        
        if (loplen == 1) {
            uint8_t* d = (uint8_t*)left;
            uint8_t* s = (uint8_t*)right;
            result = *d & *s;
        } else if (loplen == 2) {
            uint16_t* d = (uint16_t*)left;
            uint16_t* s = (uint16_t*)right;
            result = *d & *s;
        } else if (loplen == 4) {
            uint32_t* d = (uint32_t*)left;
            uint32_t* s = (uint32_t*)right;
            result = *d & *s;
        } else if (loplen == 8) {
            uint64_t* d = (uint64_t*)left;
            uint64_t* s = (uint64_t*)right;
            result = *d & *s;
        }
        
        // Update CPU flags
        regs->rflags = 0;
        
        // Zero Flag (ZF)
        if (result == 0)
            regs->rflags |= 0x40;
        
        // Sign Flag (SF)
        if (loplen == 1 && (result & 0x80))
            regs->rflags |= 0x80;
        else if (loplen == 2 && (result & 0x8000))
            regs->rflags |= 0x80;
        else if (loplen == 4 && (result & 0x80000000))
            regs->rflags |= 0x80;
        else if (loplen == 8 && (result & 0x8000000000000000ULL))
            regs->rflags |= 0x80;
        
        // Carry Flag (CF) - always cleared for TEST
        regs->rflags &= ~0x1;
        
        // Overflow Flag (OF) - always cleared for TEST
        regs->rflags &= ~0x800;
        
        // Parity Flag (PF) - only for low 8 bits
        uint8_t low_byte = result & 0xFF;
        int parity = 0;
        for (int i = 0; i < 8; i++) {
            if (low_byte & (1 << i))
                parity++;
        }
        if (parity % 2 == 0)
            regs->rflags |= 0x4;
    }
    else if (carch == x86) {
        lgr bufr;
        vectorGetValue(&simval, 0, &bufr);
        x86_Registers* regs = (x86_Registers*)bufr;
        if (left >= (uintptr_t)registers && left <= ((uintptr_t)registers + sizeof(x86_Registers)))
            x86_setupRegister(regs, left, &left, &loplen);
        if (right >= (uintptr_t)registers && right <= ((uintptr_t)registers + sizeof(x86_Registers)))
            x86_setupRegister(regs, right, &right, &roplen);

        // Perform AND operation but don't store result (TEST instruction)
        uint64_t result = 0;
        
        if (loplen == 1) {
            uint8_t* d = (uint8_t*)left;
            uint8_t* s = (uint8_t*)right;
            result = *d & *s;
        } else if (loplen == 2) {
            uint16_t* d = (uint16_t*)left;
            uint16_t* s = (uint16_t*)right;
            result = *d & *s;
        } else if (loplen == 4) {
            uint32_t* d = (uint32_t*)left;
            uint32_t* s = (uint32_t*)right;
            result = *d & *s;
        } else if (loplen == 8) {
            uint64_t* d = (uint64_t*)left;
            uint64_t* s = (uint64_t*)right;
            result = *d & *s;
        }
        
        // Update CPU flags
        regs->eflags = 0;
        
        // Zero Flag (ZF)
        if (result == 0)
            regs->eflags |= 0x40;
        
        // Sign Flag (SF)
        if (loplen == 1 && (result & 0x80))
            regs->eflags |= 0x80;
        else if (loplen == 2 && (result & 0x8000))
            regs->eflags |= 0x80;
        else if (loplen == 4 && (result & 0x80000000))
            regs->eflags |= 0x80;
        else if (loplen == 8 && (result & 0x8000000000000000ULL))
            regs->eflags |= 0x80;
        
        // Carry Flag (CF) - always cleared for TEST
        regs->eflags &= ~0x1;
        
        // Overflow Flag (OF) - always cleared for TEST
        regs->eflags &= ~0x800;
        
        // Parity Flag (PF) - only for low 8 bits
        uint8_t low_byte = result & 0xFF;
        int parity = 0;
        for (int i = 0; i < 8; i++) {
            if (low_byte & (1 << i))
                parity++;
        }
        if (parity % 2 == 0)
            regs->eflags |= 0x4;
    } else
        bugDetected("Unknown Archtecture");
    
    return PROC_SUCCESS;
}