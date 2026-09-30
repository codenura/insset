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

#ifndef ANALYZER_H
#define ANALYZER_H

#ifdef __cplusplus
#include <utility>
#endif
#include <dynvar.h>
#include <insset/cominsset.h>

typedef enum {
    // Memory & Bounds Safety
    BUFFER_OVERFLOW,
    BUFFER_UNDERFLOW,
    OUT_OF_BOUNDS_READ,
    OUT_OF_BOUNDS_WRITE,
    USE_AFTER_FREE,
    DOUBLE_FREE,
    INVALID_FREE,
    NULL_POINTER_DEREFERENCE,
    MEMORY_LEAK,
    UNINITIALIZED_READ,
    DANGLING_POINTER_ACCESS,
    HEAP_CORRUPTION,

    // Concurrency & Threading Faults
    DATA_RACE,
    DEADLOCK,
    LIVELOCK,
    MUTEX_UNLOCK_ERROR,
    THREAD_CREATION_FAILED,
    RESOURCE_STARVATION,

    // Arithmetic & Type Safety
    DIVISION_BY_ZERO,
    MODULO_BY_ZERO,
    INTEGER_OVERFLOW,
    INTEGER_UNDERFLOW,
    FLOAT_INVALID_OPERATION,
    FLOAT_DIVISION_BY_ZERO,
    FLOAT_OVERFLOW,
    FLOAT_UNDERFLOW,
    TYPE_MISMATCH,
    INVALID_CAST,

    // Execution & Hardware Faults
    SEGMENTATION_FAULT,
    ILLEGAL_INSTRUCTION,
    STACK_OVERFLOW,
    STACK_UNDERFLOW,
    BUS_ERROR,
    PRIVILEGED_INSTRUCTION_VIOLATION,
    HARDWARE_TRAP,
    ALIGNMENT_FAULT,

    // Security & Sandbox Violations
    PERMISSION_DENIED,
    UNAUTHORIZED_SYSCALL,
    UNAUTHORIZED_FILE_ACCESS,
    UNAUTHORIZED_NETWORK_ACCESS,
    RESOURCE_LIMIT_EXCEEDED,
    SANDBOX_ESCAPE_ATTEMPT,
    MALFORMED_SYSCALL_ARGUMENT,
    POLICY_VIOLATION,
    PRIVILEGE_ESCALATION_ATTEMPT,

    // I/O & File System Faults
    FILE_NOT_FOUND,
    FILE_ACCESS_DENIED,
    EOF_REACHED_UNEXPECTEDLY,
    IO_OPERATION_FAILED,
    DISK_QUOTA_EXCEEDED,

    // General Status & Control Flow
    OKAY,
    TIMEOUT,
    CANCELLED,
    UNKNOWN_ERROR
} syscallResult;

typedef enum {
    VARIABLE,
    USEABLE_SPACE,
    CODE_SEGMENT,
    DATA_SEGMENT,
    STACK_SEGMENT,
    HEAP_SEGMENT,
    DANGEROUS
} MemoryRegionType;

typedef struct {
    dynvar name;
    uintptr_t address;
    long length;
    MemoryRegionType type;
} MemoryRegion;

#ifdef __cplusplus
struct AST {
    virtual ~AST() {}
};

struct InstructionNode : AST {
    CommonOperator ioperator;
    vector* operand;

    InstructionNode(CommonOperator op, vector* opr) : ioperator(std::move(op)),
                                                      operand(std::move(opr)) { }
};

struct ValueNode : AST {
    dynvar value;
    uintptr_t orgAddress;
    ValueNode(dynvar vl, uintptr_t address) : value(std::move(vl)), orgAddress(address) { }
};
#endif

typedef struct {
    vector value;
    vector behavior;
#ifdef __cplusplus
    AST* operation;
#endif
} bhResult;

typedef struct {
    dynvar value;
    uintptr_t addr;
} ValueAddressPair;

typedef struct {
    vector node;
    int64_t answer;
    unsigned char ready;
    unsigned short hours, minutes, seconds;
} cachedCalc;

#ifdef __cplusplus
static vector analyzeCode(std::vector<AST*> nodes);

extern "C" {
#endif

extern vector analyzeFunction(dynvar functionName, dynvar source);

#ifdef __cplusplus
}
#endif

#endif
