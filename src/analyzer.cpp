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

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <fstream>
#include <iomanip>
#include <map>
#include <algorithm>
#include <threading.h>
#include <unistd.h>
#include <cstdint>

#include <llvm/Support/TargetSelect.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/MC/MCAsmInfo.h>
#include <llvm/MC/MCContext.h>
#include <llvm/MC/MCDisassembler/MCDisassembler.h>
#include <llvm/MC/MCInstPrinter.h>
#include <llvm/MC/MCInstrInfo.h>
#include <llvm/MC/MCRegisterInfo.h>
#include <llvm/MC/MCSubtargetInfo.h>
#include <llvm/MC/MCTargetOptions.h>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/TargetParser/Triple.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/Object/ObjectFile.h>
#include <llvm/Object/ELFObjectFile.h>
#include <llvm/Support/MemoryBuffer.h>

#include <dynvar.h>
#include <runtime.h>
#include <analyzer.h>
#include <definations.h>
#include <insset/cominsset.h>

/*
 * Agent: GitHub Copilot
 * LLM: GPT-5.6 Luna
 */

static AST* getOperandNode(vector* operands, int index) {
    lgr operandBuffer;
    vectorGetValue(operands, index, &operandBuffer);
    AST* node = nullptr;
    memcpy(&node, operandBuffer, sizeof(AST*));
    return node;
}

static void assignValueNode(ValueNode* destination, ValueNode* source) {
    lgr valueBuffer;
    getValue(source->value, &valueBuffer);
    setValue(&destination->value, valueBuffer);
}

typedef struct {
    ProcessorResult res;
    AST* operation;
} OperationResult;

typedef struct {
    InstructionNode* node;
    AST* targvar;
    int thrid;
} InstructionQueue;

static vector instructionQueues;
static vector executionTasks;
std::map<AST*, std::pair<bool, int>> cachedSteps;
static int lockout = -1;
static LLVMArch carch;
static vector processorResults;
static vector cachedCalcs;
static int ccLockout = -1;
static int simvalLockout = -1;

static vector analyzeCode(std::vector<AST*> nodes);

extern "C" {
    vector simval;
    vector memRegions;
}

/*
 * Agent: GitHub Copilot
 * LLM: GPT-5.6 Luna
 */

static void appendSimValue(ValueNode* node) {
    if (!node)
        return;

    ValueAddressPair pair;
    pair.addr = node->orgAddress;
    pair.value = node->value;

    lgr valueBuffer = {0};
    memcpy(valueBuffer, &pair, sizeof(ValueAddressPair));
    vectorAppend(&simval, valueBuffer);
}

static void appendSimValue(ValueNode* node, lgr valueBuffer) {
    if (!node)
        return;

    ValueAddressPair pair;
    pair.addr = node->orgAddress;
    pair.value = node->value;
    memcpy(valueBuffer, &pair, sizeof(ValueAddressPair));
    vectorAppend(&simval, valueBuffer);
}

static ValueAddressPair* findSimValue(uintptr_t address, ValueAddressPair* result) {
    for (unsigned int index = 0; index < simval.count; ++index) {
        lgr valueBuffer;
        vectorGetValue(&simval, index, &valueBuffer);
        ValueAddressPair pair;
        memcpy(&pair, valueBuffer, sizeof(ValueAddressPair));
        if ((uintptr_t)pair.addr == (uintptr_t)address) {
            *result = pair;
            return result;
        }
    }
    return nullptr;
}

static cachedCalc* findCachedCalc(AST* node, cachedCalc* result) {
    for (unsigned int index = 0; index < cachedCalcs.count; ++index) {
        lgr cacheBuffer;
        vectorGetValue(&cachedCalcs, index, &cacheBuffer);
        cachedCalc cache;
        memcpy(&cache, cacheBuffer, sizeof(cachedCalc));
        if (cache.node.count == 0)
            continue;

        lgr nodeBuffer;
        vectorGetValue(&cache.node, 0, &nodeBuffer);
        InstructionNode* cachedNode = nullptr;
        memcpy(&cachedNode, nodeBuffer, sizeof(InstructionNode*));
        if (cachedNode == node) {
            *result = cache;
            return result;
        }
    }
    return nullptr;
}

static void cacheCalcResult(AST* node, dynvar value) {
    cachedCalc cache = {0};
    if (!findCachedCalc(node, &cache)) {
        vectorInit(&cache.node, sizeof(InstructionNode*));
        lgr nodeBuffer = {0};
        memcpy(nodeBuffer, &node, sizeof(node));
        vectorAppend(&cache.node, nodeBuffer);
        cache.ready = false;

        lgr cacheBuffer = {0};
        memcpy(cacheBuffer, &cache, sizeof(cachedCalc));
        vectorAppend(&cachedCalcs, cacheBuffer);
    }

    lgr valueBuffer = {0};
    getValue(value, &valueBuffer);
    memcpy(&cache.answer, valueBuffer,
           value.length < sizeof(cache.answer) ? value.length : sizeof(cache.answer));
    cache.ready = true;

    for (unsigned int index = 0; index < cachedCalcs.count; ++index) {
        lgr cacheBuffer;
        vectorGetValue(&cachedCalcs, index, &cacheBuffer);
        cachedCalc current;
        memcpy(&current, cacheBuffer, sizeof(cachedCalc));
        if (current.node.address == cache.node.address) {
            memcpy(cacheBuffer, &cache, sizeof(cachedCalc));
            return;
        }
    }
}

dynvar evalResult(AST* node, bool cachingMode) {
    dynvar result;
    result.address = 0;
    result.length = 0;

    cachedCalc cached;
    if (dynamic_cast<InstructionNode*>(node) && findCachedCalc(node, &cached) && cached.ready) {
        lgr cachedValue = {0};
        memcpy(cachedValue, &cached.answer, sizeof(cached.answer));
        setValue(&result, cachedValue);
        return result;
    }

    if (auto instr = dynamic_cast<InstructionNode*>(node)) {
        if (instr->ioperator == ADD) {
            ProcessorResult pres;
            int64_t leftOp, rightOp;
            int loplen, roplen;
            dynvar var;
            switch (carch) {
                case x86_64:
                case x86:
                case i8086:
                    if (instr->operand->count != 2) {
                        std::cerr << "Ilegal Instruction." << std::endl;
                        exit(1);
                    }
                    const int64_t maxCalcValue = carch == x86_64 ? 9223372036854775807 :
                                  (carch == x86 ? 2147483647 : 32767);
                    
                    dynvar left = evalResult(getOperandNode(instr->operand, 0), cachingMode);
                    var = left;
                    loplen = var.length;
                    if (loplen > sizeof(int64_t)) {
                        pres = PROC_INTEGER_OVERFLOW;
                        goto add_end;
                    }
                    memcpy((void*)leftOp, (void*)var.address, loplen);
                    
                    var = evalResult(getOperandNode(instr->operand, 1), cachingMode);
                    roplen = var.length;
                    if (roplen > sizeof(int64_t)) {
                        pres = PROC_INTEGER_OVERFLOW;
                        goto add_end;
                    }
                    memcpy((void*)rightOp, (void*)var.address, roplen);

                    if ((leftOp > maxCalcValue || rightOp > maxCalcValue) ||
                        (leftOp < ((maxCalcValue * -1) -1) || rightOp < ((maxCalcValue * -1) -1))) {
                        pres = PROC_INTEGER_OVERFLOW;
                        goto add_end;
                    }

                    if (cachingMode) {
                        result = left;
                        pres = add(result.address, loplen, (uintptr_t)rightOp,
                                                     roplen, emvec, emvec);
                        goto add_end;
                        
                    } else
                        pres = add((uintptr_t)leftOp, loplen, (uintptr_t)rightOp,
                                                        roplen, emvec, emvec);
                    
                    if (leftOp > maxCalcValue) {
                        pres = PROC_BUFFER_OVERFLOW;
                        goto add_end;
                    }
                /* default:
                    bugDetected("The Specified Architecture Is Not Supported"); */
            }
add_end:
            OperationResult res;
            res.operation = node;
            res.res = pres;
            lgr bufr;
            memcpy((void*)bufr, (void*)&res, sizeof(OperationResult));
            vectorAppend(&processorResults, bufr);

            if (pres != PROC_SUCCESS)
                return result;
            memset((void*)bufr, 0, sizeof(OperationResult));
            lgr resultValue = {0};
            memcpy(resultValue, &leftOp, sizeof(leftOp));
            setValue(&result, resultValue);
            return result;

        } else if (instr->ioperator == SUB) {
            ProcessorResult pres;
            int64_t leftOp, rightOp;
            int loplen, roplen;
            dynvar var;
            switch (carch) {
                case x86_64:
                case x86:
                case i8086:
                    if (instr->operand->count != 2) {
                        std::cerr << "Ilegal Instruction." << std::endl;
                        exit(1);
                    }
                    const int64_t maxCalcValue = carch == x86_64 ? 9223372036854775807 :
                                  (carch == x86 ? 2147483647 : 32767);
                    
                    dynvar left = evalResult(getOperandNode(instr->operand, 0), cachingMode);
                    var = left;
                    loplen = var.length;
                    if (loplen > sizeof(int64_t)) {
                        pres = PROC_INTEGER_OVERFLOW;
                        goto sub_end;
                    }
                    memcpy((void*)leftOp, (void*)var.address, loplen);
                    
                    var = evalResult(getOperandNode(instr->operand, 1), cachingMode);
                    roplen = var.length;
                    if (roplen > sizeof(int64_t)) {
                        pres = PROC_INTEGER_OVERFLOW;
                        goto sub_end;
                    }
                    memcpy((void*)rightOp, (void*)var.address, roplen);

                    if ((leftOp > maxCalcValue || rightOp > maxCalcValue) ||
                        (leftOp < ((maxCalcValue * -1) -1) || rightOp < ((maxCalcValue * -1) -1))) {
                        pres = PROC_INTEGER_OVERFLOW;
                        goto sub_end;
                    }

                    if (cachingMode) {
                        result = left;
                        pres = subtract(result.address, loplen, (uintptr_t)rightOp,
                                                     roplen, emvec, emvec);
                        goto sub_end;
                        
                    } else
                        pres = subtract((uintptr_t)leftOp, loplen, (uintptr_t)rightOp,
                                                        roplen, emvec, emvec);
                    
                    if (leftOp > maxCalcValue) {
                        pres = PROC_BUFFER_OVERFLOW;
                        goto sub_end;
                    }
                /* default:
                    bugDetected("The Specified Architecture Is Not Supported"); */
            }
sub_end:
            OperationResult res;
            res.operation = node;
            res.res = pres;
            lgr bufr;
            memcpy((void*)bufr, (void*)&res, sizeof(OperationResult));
            vectorAppend(&processorResults, bufr);

            if (pres != PROC_SUCCESS)
                return result;
            memset((void*)bufr, 0, sizeof(OperationResult));
            lgr resultValue = {0};
            memcpy(resultValue, &leftOp, sizeof(leftOp));
            setValue(&result, resultValue);
            return result;

        } else if (instr->ioperator == MUL) {
            ProcessorResult pres;
            int64_t leftOp, rightOp;
            int loplen, roplen;
            dynvar var;
            switch (carch) {
                case x86_64:
                case x86:
                case i8086:
                    if (instr->operand->count != 2) {
                        std::cerr << "Ilegal Instruction." << std::endl;
                        exit(1);
                    }
                    const int64_t maxCalcValue = carch == x86_64 ? 9223372036854775807 :
                                  (carch == x86 ? 2147483647 : 32767);
                    
                    dynvar left = evalResult(getOperandNode(instr->operand, 0), cachingMode);
                    var = left;
                    loplen = var.length;
                    if (loplen > sizeof(int64_t)) {
                        pres = PROC_INTEGER_OVERFLOW;
                        goto mul_end;
                    }
                    memcpy((void*)leftOp, (void*)var.address, loplen);
                    
                    var = evalResult(getOperandNode(instr->operand, 1), cachingMode);
                    roplen = var.length;
                    if (roplen > sizeof(int64_t)) {
                        pres = PROC_INTEGER_OVERFLOW;
                        goto mul_end;
                    }
                    memcpy((void*)rightOp, (void*)var.address, roplen);

                    if ((leftOp > maxCalcValue || rightOp > maxCalcValue) ||
                        (leftOp < ((maxCalcValue * -1) -1) || rightOp < ((maxCalcValue * -1) -1))) {
                        pres = PROC_INTEGER_OVERFLOW;
                        goto mul_end;
                    }

                    if (cachingMode) {
                        result = left;
                        pres = multiply(result.address, loplen, (uintptr_t)rightOp,
                                                     roplen, emvec, emvec);
                        goto mul_end;
                        
                    } else
                        pres = multiply((uintptr_t)leftOp, loplen, (uintptr_t)rightOp,
                                                        roplen, emvec, emvec);
                    
                    if (leftOp > maxCalcValue) {
                        pres = PROC_BUFFER_OVERFLOW;
                        goto mul_end;
                    }
                /* default:
                    bugDetected("The Specified Architecture Is Not Supported"); */
            }
mul_end:
            OperationResult res;
            res.operation = node;
            res.res = pres;
            lgr bufr;
            memcpy((void*)bufr, (void*)&res, sizeof(OperationResult));
            vectorAppend(&processorResults, bufr);

            if (pres != PROC_SUCCESS)
                return result;
            memset((void*)bufr, 0, sizeof(OperationResult));
            lgr resultValue = {0};
            memcpy(resultValue, &leftOp, sizeof(leftOp));
            setValue(&result, resultValue);
            return result;

        } else if (instr->ioperator == DIV) {
            ProcessorResult pres;
            int64_t leftOp, rightOp;
            int loplen, roplen;
            dynvar var;
            switch (carch) {
                case x86_64:
                case x86:
                case i8086:
                    if (instr->operand->count != 2) {
                        std::cerr << "Ilegal Instruction." << std::endl;
                        exit(1);
                    }
                    const int64_t maxCalcValue = carch == x86_64 ? 9223372036854775807 :
                                  (carch == x86 ? 2147483647 : 32767);
                    
                    dynvar left = evalResult(getOperandNode(instr->operand, 0), cachingMode);
                    var = left;
                    loplen = var.length;
                    if (loplen > sizeof(int64_t)) {
                        pres = PROC_INTEGER_OVERFLOW;
                        goto div_end;
                    } else 
                    memcpy((void*)leftOp, (void*)var.address, loplen);
                    
                    var = evalResult(getOperandNode(instr->operand, 1), cachingMode);
                    roplen = var.length;
                    if (roplen > sizeof(int64_t)) {
                        pres = PROC_INTEGER_OVERFLOW;
                        goto div_end;
                    }
                    memcpy((void*)rightOp, (void*)var.address, roplen);

                    if (leftOp == 0 || rightOp == 0) {
                        pres = PROC_DIVISION_BY_ZERO;
                        goto div_end;
                    } else if ((leftOp > maxCalcValue || rightOp > maxCalcValue) ||
                        (leftOp < ((maxCalcValue * -1) -1) || rightOp < ((maxCalcValue * -1) -1))) {
                        pres = PROC_INTEGER_OVERFLOW;
                        goto div_end;
                    }

                    if (cachingMode) {
                        result = left;
                        pres = divide(result.address, loplen, (uintptr_t)rightOp,
                                                     roplen, emvec, emvec);
                        goto div_end;
                        
                    } else
                        pres = divide((uintptr_t)leftOp, loplen, (uintptr_t)rightOp,
                                                        roplen, emvec, emvec);
                    
                    if (leftOp > maxCalcValue) {
                        pres = PROC_BUFFER_OVERFLOW;
                        goto div_end;
                    }

                /* default:
                    bugDetected("The Specified Architecture Is Not Supported"); */
            }
div_end:
            OperationResult res;
            res.operation = node;
            res.res = pres;
            lgr bufr;
            memcpy((void*)bufr, (void*)&res, sizeof(OperationResult));
            vectorAppend(&processorResults, bufr);

            if (pres != PROC_SUCCESS)
                return result;
            memset((void*)bufr, 0, sizeof(OperationResult));
            lgr resultValue = {0};
            memcpy(resultValue, &leftOp, sizeof(leftOp));
            setValue(&result, resultValue);
            return result;
        }
    } else if (auto valNode = dynamic_cast<ValueNode*>(node))
        return valNode->value;

    return result;
}

void handleRealtimeOperation(void* arg) {
    ThreadCallbackArgs* callbackArgs = (ThreadCallbackArgs*)arg;
    InstructionQueue* queue = (InstructionQueue*)callbackArgs->originalArg;
    const int threadId = callbackArgs->threadId;
    int idx;
    long cindx = -1;

    dynvar value = evalResult(queue->node, true);
    cacheCalcResult(queue->node, value);

    if (queue->node->ioperator == ADD || queue->node->ioperator == SUB ||
        queue->node->ioperator == MUL || queue->node->ioperator == DIV ||
        queue->node->ioperator == INC || queue->node->ioperator == DEC) {
            cachedCalc cc;
            vectorInit(&cc.node, sizeof(InstructionNode*));
            lgr bufr = {0};
            memcpy(bufr, &queue->node, sizeof(queue->node));
            while (ccLockout != threadId || ccLockout != -1) usleep(2000);
            ccLockout = threadId;
            if(vectorAppend(&cachedCalcs, bufr) == DYNVAR_SUCCESS) {
                cindx = cachedCalcs.count - 1;
                ccLockout = -1;
            }
        }

    while (true) {
        while (lockout != threadId || lockout != -1) usleep(2000);
        lockout = threadId;
        if (cachedSteps.contains((AST*)queue))
            cachedSteps.find((AST*)queue)->second.second = threadId;
        std::map<AST*, std::pair<bool, int>> currentSteps = cachedSteps;
        lockout = -1;
        idx = 0;
        for (auto [cachedStep, stepInfo] : currentSteps) {
            auto [locked, stepThreadId] = stepInfo;
            if (auto instr = dynamic_cast<InstructionQueue*>(cachedStep))
                if (instr->targvar == queue->targvar) {
                    while (locked) {
                        locked = std::next(cachedSteps.begin(), idx)->second.first;
                        usleep(2000);
                    }
                    goto finalStep;
                }
            idx++;
        }
        usleep(2000);
    }

finalStep:
    lockout = -1;
    value = evalResult(queue->node, true);
    cacheCalcResult(queue->node, value);
    
    if (queue->node->ioperator == ADD || queue->node->ioperator == SUB ||
        queue->node->ioperator == MUL || queue->node->ioperator == DIV ||
        queue->node->ioperator == INC || queue->node->ioperator == DEC) {
        switch (carch) {
            case x86_64:
            case x86:
            case i8086:
                if (queue->node->operand->count == 0) {
                    std::cerr << "Ilegal Instruction." << std::endl;
                    exit(1);
                }

                ValueNode* targetNode = nullptr;
                lgr targetBuffer;
                vectorGetValue(queue->node->operand, 0, &targetBuffer);
                memcpy(&targetNode, targetBuffer, sizeof(ValueNode*));
                
                while (simvalLockout != threadId || simvalLockout == -1) usleep(2000);
                simvalLockout = threadId;
                for (int i; i < (simval.count - 1); i++) {
                    lgr bufr;
                    vectorGetValue(&simval, i, &bufr);
                    ValueAddressPair vap;
                    memcpy(&vap, bufr, sizeof(ValueAddressPair));
                    if (vap.addr == targetNode->orgAddress) {
                        if (!(targetNode->value.address == 0 || targetNode->value.length == 0)) {
                            if (targetNode->value.length < vap.value.length) {
                                OperationResult res;
                                res.operation = queue->node;
                                res.res = PROC_BUFFER_OVERFLOW;
                                lgr bufr;
                                memcpy((void*)bufr, (void*)&res, sizeof(OperationResult));
                                vectorAppend(&processorResults, bufr);
                                break;
                            }
                            memcpy((void*)vap.value.address, (void*)targetNode->value.address,
                                   vap.value.length);
                        } else
                            memcpy((void*)vap.value.address, (void*)targetNode->orgAddress,
                                   vap.value.length);
                        memcpy(VECTOR_FORMULA(&simval, i), &vap, sizeof(ValueAddressPair));
                        break;
                    }
                }
        }
    }
}

CommonOperator mapLLVMOpcodeToOperator(unsigned opcode, const llvm::MCInstrInfo* MII) {
    // Get instruction name from MCInstrInfo
    const llvm::StringRef instName = MII->getName(opcode);
    
    // Map instruction names to CommonOperator
    std::string name(instName.str());
    
    // Convert to uppercase for easier comparison
    std::string upperName = name;
    std::transform(upperName.begin(), upperName.end(), upperName.begin(), ::toupper);
    
    if (upperName.size() >= 3) {
        if (upperName.substr(0, 3) == "MOV")
            return MOV;
        else if (upperName.substr(0, 3) == "ADD")
            return ADD;
        else if (upperName.substr(0, 3) == "SUB")
            return SUB;
        else if (upperName.substr(0, 3) == "MUL")
            return MUL;
        else if (upperName.substr(0, 3) == "DIV")
            return DIV;
        else if (upperName.substr(0, 3) == "INC")
            return INC;
        else if (upperName.substr(0, 3) == "DEC")
            return DEC;
        else if (upperName.substr(0, 3) == "AND")
            return AND;
        else if (upperName.substr(0, 3) == "XOR")
            return XOR;
        else if (upperName.substr(0, 3) == "NOT")
            return NOT;
        else if (upperName.substr(0, 3) == "SHL")
            return SHL;
        else if (upperName.substr(0, 3) == "SHR")
            return SHR;
        else if (upperName.substr(0, 3) == "SAR")
            return SAR;
        else if (upperName.substr(0, 3) == "CMP")
            return CMP;
        else if (upperName.substr(0, 3) == "JMP")
            return JMP;
        else if (upperName.substr(0, 3) == "JNE")
            return JNE;
        else if (upperName.substr(0, 3) == "JGE")
            return JGE;
        else if (upperName.substr(0, 3) == "JLE")
            return JLE;
        else if (upperName.substr(0, 3) == "JAE")
            return JAE;
        else if (upperName.substr(0, 3) == "JBE")
            return JBE;
        else if (upperName.substr(0, 3) == "JNO")
            return JNO;
        else if (upperName.substr(0, 3) == "RET")
            return RET;
        else if (upperName.substr(0, 3) == "POP")
            return POP;
        else if (upperName.substr(0, 3) == "LEA")
            return LEA;
        else if (upperName.substr(0, 3) == "NEG")
            return NEG;
        else if (upperName.substr(0, 3) == "ROL")
            return ROL;
        else if (upperName.substr(0, 3) == "ROR")
            return ROR;
        else if (upperName.substr(0, 3) == "INT")
            return INT;
        else if (upperName.substr(0, 3) == "CLD")
            return CLD;
        else if (upperName.substr(0, 3) == "STD")
            return STD;
        else if (upperName.substr(0, 3) == "CDQ" || upperName.substr(0, 3) == "CQO")
            return CLTD;
        else if (upperName.substr(0, 3) == "CBW")
            return CBW;
        else if (upperName.substr(0, 3) == "NOP")
            return NOP;
    } else if (upperName.size() >= 4) {
        if (upperName.substr(0, 4) == "IDIV")
            return DIV;
        else if (upperName.substr(0, 4) == "TEST")
            return TEST;
        else if (upperName.substr(0, 4) == "CALL")
            return CALL;
        else if (upperName.substr(0, 4) == "PUSH")
            return PUSH;
        else if (upperName.substr(0, 4) == "XCHG")
            return XCHG;
        else if (upperName.substr(0, 4) == "CWDE" || upperName.substr(0, 4) == "CDQE")
            return CBW;
        else if (upperName.substr(0, 4) == "LOOP")
            return LOOP;
    } else if (upperName.size() >= 2) {
        if (upperName.substr(0, 2) == "OR")
            return OR;
        else if (upperName.substr(0, 2) == "JE")
            return JE;
        else if (upperName.substr(0, 2) == "JG")
            return JG;
        else if (upperName.substr(0, 2) == "JL")
            return JL;
        else if (upperName.substr(0, 2) == "JA")
            return JA;
        else if (upperName.substr(0, 2) == "JB")
            return JB;
        else if (upperName.substr(0, 2) == "JO")
            return JO;
        else if (upperName.substr(0, 2) == "JS")
            return JS;
    } else if (upperName.size() >= 7) {
        if (upperName.substr(0, 7) == "SYSCALL")
            return SYSCALL;
        else if (upperName.substr(0, 7) == "SYSEXIT")
            return SYSEXIT;
    } else if (upperName.size() >= 8) {
        if (upperName.substr(0, 8) == "SYSENTER")
            return SYSENTER;
    } else if (upperName.size() >= 5) {
        if (upperName.substr(0, 5) == "LEAVE")
            return LEAVE;
        else if (upperName.substr(0, 5) == "ENTER")
            return ENTER;
    }
    
    return UNKNOWN_OP;
}

extern "C" {
    void init(LLVMArch arch) {
        vectorInit(&instructionQueues, sizeof(int));
        vectorInit(&executionTasks, sizeof(int));
        vectorInit(&processorResults, sizeof(OperationResult));
        vectorInit(&memRegions, sizeof(MemoryRegion));
        vectorInit(&cachedCalcs, sizeof(cachedCalc));
        /*
        * Agent: GitHub Copilot
        * LLM: GPT-5.6 Luna
        */

        vectorInit(&simval, sizeof(ValueAddressPair));
        carch = arch;
        cinit(arch);
    }

    vector analyzeFunction(dynvar functionName, dynvar source) {
        vector result, pushedFunction;
        uintptr_t currentFunction;

        vectorInit(&pushedFunction, sizeof(currentFunction));
        static lgr bufr = {0};
        getValue(source, &bufr);
        
        // Load ELF file using LLVM Object library
        auto fileBufferOrErr = llvm::MemoryBuffer::getFile(bufr);
        if (!fileBufferOrErr) {
            std::cerr << "Cannot Analyze The Library From " << bufr << std::endl;
            exit(1);
        }
        
        auto objFileOrErr = llvm::object::ObjectFile::createObjectFile(fileBufferOrErr.get()->getMemBufferRef());
        if (!objFileOrErr) {
            std::cerr << "Cannot parse ELF file from " << bufr << std::endl;
            exit(1);
        }
        
        llvm::object::ObjectFile* objFile = objFileOrErr.get().get();
        auto* elfFile = llvm::dyn_cast<llvm::object::ELFObjectFileBase>(objFile);
        if (!elfFile) {
            std::cerr << bufr << " is not an ELF file." << std::endl;
            exit(1);
        }

        // Find .text section
        uintptr_t textStart = 0;
        uintptr_t textSize = 0;
        for (const auto& section : elfFile->sections()) {
            llvm::Expected<llvm::StringRef> nameOrErr = section.getName();
            if (nameOrErr && nameOrErr.get() == ".text") {
                textStart = section.getAddress();
                textSize = section.getSize();
                break;
            }
        }
        
        if (textSize == 0) {
            std::cerr << bufr << " is Useless (no .text section)." << std::endl;
            return result;
        }

        // Find symbol by name
        getValue(functionName, &bufr);
        std::string symName(bufr);
        uintptr_t symValue = 0;
        
        for (const auto& sym : elfFile->symbols()) {
            llvm::Expected<llvm::StringRef> nameOrErr = sym.getName();
            llvm::Expected<uint64_t> addrOrErr = sym.getAddress();
            if (nameOrErr && addrOrErr && nameOrErr.get() == symName) {
                symValue = addrOrErr.get();
                break;
            }
        }
        
        if (symValue == 0) {
            lgr libpath = {0};
            getValue(source, &libpath);
            std::cerr << symName << " Is Not Found From " << libpath
                      << " But Required." << std::endl;
            exit(1);
        }

        std::vector<AST*> nodes;
        llvm::Triple targetTriple;
        std::unique_ptr<llvm::MCDisassembler> dis;
        std::unique_ptr<llvm::MCInstrInfo> MII;
        std::unique_ptr<llvm::MCRegisterInfo> MRI;
        try {
            llvm::InitializeAllTargetInfos();
            llvm::InitializeAllTargetMCs();
            llvm::InitializeAllDisassemblers();

            targetTriple = llvm::Triple("x86_64-pc-linux-gnu");
            std::string err;
            const llvm::Target *target =
                        llvm::TargetRegistry::lookupTarget(targetTriple.getTriple(), err);

            if (!target) {
                char* errCopy = strdup(err.c_str());
                bugDetected(errCopy);
                free(errCopy);
            }

            MRI = std::unique_ptr<llvm::MCRegisterInfo>(
                target->createMCRegInfo(targetTriple.getTriple()));

            llvm::MCTargetOptions mcOpts;
            auto MAI = std::unique_ptr<llvm::MCAsmInfo>(
                target->createMCAsmInfo(*MRI, targetTriple.getTriple(), mcOpts));

            auto STI = std::unique_ptr<llvm::MCSubtargetInfo>(
                target->createMCSubtargetInfo(
                    targetTriple.getTriple(), "x86-64", ""));

            MII = std::unique_ptr<llvm::MCInstrInfo>(
                target->createMCInstrInfo());

            llvm::MCContext ctx(targetTriple, MAI.get(), MRI.get(), STI.get());

            dis = std::unique_ptr<llvm::MCDisassembler>(
                target->createMCDisassembler(*STI, ctx));
        } catch (...) {
            char* msg = strdup("Disassembler Is Failed To Initialize.");
            bugDetected(msg);
            free(msg);
        }
        // Get the file content from the MemoryBuffer
        llvm::StringRef fileContent = fileBufferOrErr.get()->getBuffer();
        llvm::ArrayRef<uint8_t> bytes(reinterpret_cast<const uint8_t*>(fileContent.data()),
                                       fileContent.size());

        currentFunction = symValue;
        const uintptr_t textEnd = textStart + textSize;
        uint64_t size = 0;
        
        for (uintptr_t addr = currentFunction; addr < textEnd; addr += size) {
            llvm::MCInst inst;
            size = 0;
            auto status = dis->getInstruction(
                        inst,
                        size,
                        bytes.slice(addr - textStart),
                        addr,
                        llvm::nulls());

            if (status == llvm::MCDisassembler::Success) {
                CommonOperator op = mapLLVMOpcodeToOperator(inst.getOpcode(), MII.get());
                
                /*
                 * Agent: GitHub Copilot
                 * LLM: GPT-5.6 Luna
                 */
                vector* operands = new vector();
                vectorInit(operands, sizeof(AST*));
                
                for (unsigned opIdx = 0; opIdx < inst.getNumOperands(); ++opIdx) {
                    const llvm::MCOperand &operand = inst.getOperand(opIdx);
                    /*
                     * Agent: GitHub Copilot
                     * LLM: GPT-5.6 Luna
                     */
                    uintptr_t operandValue = 0;
                    std::string operandText = "0";

                    if (operand.isReg()) {
                        operandValue = operand.getReg();
                        operandText = std::to_string(operandValue);
                        llvm::outs() << "Reg: " << MRI->getName(operand.getReg()) << "\n";
                    } else if (operand.isImm()) {
                        const int64_t immediate = operand.getImm();
                        operandValue = (uintptr_t)immediate;
                        operandText = std::to_string(immediate);
                        llvm::outs() << "Imm: " << operand.getImm() << "\n";
                    } else if (operand.isExpr())
                        llvm::outs() << "Expr\n";

                    ValueNode* operandNode = nullptr;
                    ValueAddressPair pair;
                    if (operand.isReg() && findSimValue(operandValue, &pair))
                        operandNode = new ValueNode(pair.value, operandValue);
                    if (operandNode == nullptr) {
                        dynvar value = {0, 0};
                        lgr initialValue = {0};
                        std::strncpy(initialValue, operandText.c_str(), sizeof(initialValue) - 1);
                        setValue(&value, initialValue);
                        operandNode = new ValueNode(value, operandValue);
                        if (operand.isReg())
                            appendSimValue(operandNode);
                    }
                    lgr operandBuf = {0};
                    memcpy(operandBuf, &operandNode, sizeof(AST*));
                    vectorAppend(operands, operandBuf);
                }
                
                InstructionNode* node = new InstructionNode(op, operands);
                nodes.push_back(node);
            } else
                size = 1; // Skip one byte if disassembly fails
        }

        /* 'simval' Variable will be erased after full simulation and analyze of compiled software */
        // vectorDeleteAll(&simval);
        vector reslt = analyzeCode(nodes);
        return reslt;
    }
}

static vector analyzeCode(std::vector<AST*> nodes) {
    vector result;
    // AST Interpretation logic separated from analyzeFunction
        for (AST* node : nodes) {
            if (auto instrNode = dynamic_cast<InstructionNode*>(node)) {
                if ((instrNode->ioperator == ADD || instrNode->ioperator == SUB ||
                     instrNode->ioperator == MUL || instrNode->ioperator == DIV ||
                     instrNode->ioperator == INC || instrNode->ioperator == DEC) &&
                    instrNode->operand->count > 0) {
                    InstructionQueue inqueue;
                    inqueue.node = instrNode;
                    lgr tvarBuffer;
                    vectorGetValue(instrNode->operand, 0, &tvarBuffer);
                    /*
                     * Agent: GitHub Copilot
                     * LLM: GPT-5.6 Luna
                     */
                    AST* tvar = nullptr;
                    memcpy(&tvar, tvarBuffer, sizeof(AST*));
                    inqueue.targvar = tvar;
                    int threadID = threadNew(handleRealtimeOperation, &inqueue);
                    if (threadID < 0)
                        handleRealtimeOperation(&inqueue);
                    else {
                        lgr threadIDBuffer;
                        memcpy(threadIDBuffer, &threadID, sizeof(int));
                        vectorAppend(&instructionQueues, threadIDBuffer);
                        threadDetach(threadID);
                    }
                } else if (instrNode->ioperator == MOV) {
                    if (carch == x86 || carch == x86_64) {
                        if (instrNode->operand->count != 2) {
                            std::cerr << "Ilegal Instruction." << std::endl;
                            exit(1);
                        }

                        lgr bufr;
                        vectorGetValue(instrNode->operand, 0, &bufr);
                        /*
                         * Agent: GitHub Copilot
                         * LLM: GPT-5.6 Luna
                         */
                        AST* tvar = nullptr;
                        memcpy(&tvar, bufr, sizeof(AST*));
                        vectorGetValue(instrNode->operand, 1, &bufr);
                        AST* value = nullptr;
                        memcpy(&value, bufr, sizeof(AST*));
                        for (int i = 0; i < (instructionQueues.count - 1); i++) {
                            vectorGetValue(&instructionQueues, i, &bufr);
                            InstructionQueue insQueue;
                            memcpy(&insQueue, bufr, sizeof(InstructionQueue));
                            if (auto val = dynamic_cast<ValueNode*>(insQueue.targvar))
                                if (auto target = dynamic_cast<ValueNode*>(tvar))
                                    if (val->orgAddress == target->orgAddress) {
                                        while (insQueue.thrid >= 0)
                                            usleep(2000);
                                        
                                        ProcessorResult pres = copy((uintptr_t)target->orgAddress,
                                                                    target->value.length, (uintptr_t)val->orgAddress,
                                                                    val->value.length, emvec, emvec);
                                        
                                        OperationResult res;
                                        res.operation = node;
                                        res.res = pres;
                                        lgr bufr;
                                        memcpy((void*)bufr, (void*)&res, sizeof(OperationResult));
                                        vectorAppend(&processorResults, bufr);

                                        break;
                                    }
                            
                            if (i == instructionQueues.count - 1)
                                bugDetected("Invalid Operand Node.");
                        }
                    }
                }
            }
        }

        // For now, clean up AST nodes after analysis
        // In a complete implementation, these would be stored in the result
        for (AST* node : nodes) {
            InstructionNode* instrNode = static_cast<InstructionNode*>(node);
            if (instrNode->operand) {
                /*
                 * Agent: GitHub Copilot
                 * LLM: GPT-5.6 Luna
                 */
                for (unsigned int index = 0; index < instrNode->operand->count; ++index) {
                    AST* operandNode = getOperandNode(instrNode->operand, index);
                    delete operandNode;
                }
                vectorDeleteAll(instrNode->operand);
                delete instrNode->operand;
            }
            delete instrNode;
        }

        return result;
}
