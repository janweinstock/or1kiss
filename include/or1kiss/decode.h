/******************************************************************************
 *                                                                            *
 * Copyright 2018 Jan Henrik Weinstock                                        *
 *                                                                            *
 * Licensed under the Apache License, Version 2.0 (the "License");            *
 * you may not use this file except in compliance with the License.           *
 * You may obtain a copy of the License at                                    *
 *                                                                            *
 *     http://www.apache.org/licenses/LICENSE-2.0                             *
 *                                                                            *
 * Unless required by applicable law or agreed to in writing, software        *
 * distributed under the License is distributed on an "AS IS" BASIS,          *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   *
 * See the License for the specific language governing permissions and        *
 * limitations under the License.                                             *
 *                                                                            *
 ******************************************************************************/

#ifndef OR1KISS_OPCODE_H
#define OR1KISS_OPCODE_H

#include "or1kiss/includes.h"
#include "or1kiss/types.h"
#include "or1kiss/utils.h"

namespace or1kiss {

    enum opcode {
        INVALID_OPCODE = 0,

        /* ORBIS32 */
        ORBIS32_NOP,    /* l.nop    (no operation) */
        ORBIS32_MFSPR,  /* l.mfspr  (move from special-purpose register) */
        ORBIS32_MTSPR,  /* l.mtspr  (move to special-purpose register) */
        ORBIS32_MOVHI,  /* l.movhi  (move immediate high) */

        /* Control */
        ORBIS32_J,      /* l.j      (jump) */
        ORBIS32_JR,     /* l.jr     (jump register) */
        ORBIS32_JAL,    /* l.jal    (jump and link) */
        ORBIS32_JALR,   /* l.jalr   (jump and link register) */
        ORBIS32_BF,     /* l.bf     (branch if flag) */
        ORBIS32_BNF,    /* l.bnf    (branch if no flag) */

        /* Load & Store */
        ORBIS32_LWA,    /* l.lwa    (load word atomic) */
        ORBIS32_LWZ,    /* l.lwz    (load single word and extend with zero) */
        ORBIS32_LWS,    /* l.lws    (load single word and extend with sign) */
        ORBIS32_LHZ,    /* l.lhz    (load half word and extend with zero) */
        ORBIS32_LHS,    /* l.lhs    (load half word and extend with sign) */
        ORBIS32_LBZ,    /* l.lbz    (load byte and extend with zero) */
        ORBIS32_LBS,    /* l.lbs    (load byte and extend with sign) */
        ORBIS32_SWA,    /* l.swa    (store word atomic) */
        ORBIS32_SW,     /* l.sw     (store single word) */
        ORBIS32_SH,     /* l.sh     (store half word) */
        ORBIS32_SB,     /* l.sb     (store byte) */

        /* Sign/Zero Extend */
        ORBIS32_EXTWZ,  /* l.extwz  (extend word with zero) */
        ORBIS32_EXTWS,  /* l.extws  (extend word with sign) */
        ORBIS32_EXTHZ,  /* l.exthz  (extend half word with zero) */
        ORBIS32_EXTHS,  /* l.exths  (extend half word with sign) */
        ORBIS32_EXTBZ,  /* l.extbz  (extend byte with zero) */
        ORBIS32_EXTBS,  /* l.extbs  (extend byte with sign) */

        /* ALU (reg, reg) */
        ORBIS32_ADD,    /* l.add    (add) */
        ORBIS32_ADDC,   /* l.addc   (add and carry) */
        ORBIS32_SUB,    /* l.sub    (subtract) */
        ORBIS32_AND,    /* l.and    (and) */
        ORBIS32_OR,     /* l.or     (or) */
        ORBIS32_XOR,    /* l.xor    (exclusive or) */
        ORBIS32_CMOV,   /* l.cmov   (conditional move) */
        ORBIS32_FF1,    /* l.ff1    (find first 1) */
        ORBIS32_FL1,    /* l.fl1    (find last 1) */
        ORBIS32_SLL,    /* l.sll    (shift left logical) */
        ORBIS32_SRL,    /* l.srl    (shift right logical) */
        ORBIS32_SRA,    /* l.sra    (shift right arithmetic) */
        ORBIS32_ROR,    /* l.ror    (rotate right) */
        ORBIS32_MUL,    /* l.mul    (multiply signed) */
        ORBIS32_MULU,   /* l.mulu   (multiply unsigned) */
        ORBIS32_MULD,   /* l.muld   (multiply signed to double) */
        ORBIS32_MULDU,  /* l.muldu  (multiply unsigned to double) */
        ORBIS32_DIV,    /* l.div    (divide signed) */
        ORBIS32_DIVU,   /* l.divu   (divide unsigned) */

        /* ALU (reg, imm) */
        ORBIS32_ADDI,   /* l.addi   (add immediate) */
        ORBIS32_ADDIC,  /* l.addic  (add immediate and carry) */
        ORBIS32_ANDI,   /* l.andi   (and with immediate half word) */
        ORBIS32_ORI,    /* l.ori    (or with immediate half word) */
        ORBIS32_XORI,   /* l.xori   (exclusive or with immediate half word) */
        ORBIS32_MULI,   /* l.muli   (multiply immediate signed) */
        ORBIS32_SLLI,   /* l.slli   (shift left logical with immediate) */
        ORBIS32_SRLI,   /* l.srli   (shift right logical with immediate) */
        ORBIS32_SRAI,   /* l.srai   (shift right arithmetic with immediate) */
        ORBIS32_RORI,   /* l.rori   (rotate right with immediate) */

        /* Comparison (reg, reg) */
        ORBIS32_SFEQ,   /* l.sfeq   (set flag if equal) */
        ORBIS32_SFNE,   /* l.sfne   (set flag if not equal) */
        ORBIS32_SFGTU,  /* l.sfgtu  (set flag if greater than unsigned) */
        ORBIS32_SFGEU,  /* l.sfgeu  (set flag if greater or equal than unsigned) */
        ORBIS32_SFLTU,  /* l.sfltu  (set flag if less than unsigned) */
        ORBIS32_SFLEU,  /* l.sfleu  (set flag if less or equal than unsigned) */
        ORBIS32_SFGTS,  /* l.sfgts  (set flag if greater than signed) */
        ORBIS32_SFGES,  /* l.sfges  (set flag if greater or equal than signed)*/
        ORBIS32_SFLTS,  /* l.sflts  (set flag if less than signed) */
        ORBIS32_SFLES,  /* l.sfles  (set flag if less or equal than signed) */

        /* Comparison (reg, imm) */
        ORBIS32_SFEQI,  /* l.sfeqi  (set flag if equal immediate) */
        ORBIS32_SFNEI,  /* l.sfnei  (set flag if not equal immediate) */
        ORBIS32_SFGTUI, /* l.sfgtui (set flag if greater than immediate unsigned) */
        ORBIS32_SFGEUI, /* l.sfgeui (set flag if greater or equal than immediate unsigned) */
        ORBIS32_SFLTUI, /* l.sfltui (set flag if less than immediate unsigned) */
        ORBIS32_SFLEUI, /* l.sfleui (set flag if less or equal than immediate unsigned) */
        ORBIS32_SFGTSI, /* l.sfgtsi (set flag if greater than immediate signed) */
        ORBIS32_SFGESI, /* l.sfgesi (set flag if greater or equal than immediate signed) */
        ORBIS32_SFLTSI, /* l.sfltsi (set flag if less than immediate signed) */
        ORBIS32_SFLESI, /* l.sflesi (set flag if less or equal than immediate signed) */

        /* Multiply Accumulate */
        ORBIS32_MAC,    /* l.mac    (multiply and accumulate signed) */
        ORBIS32_MACU,   /* l.macu   (multiply and accumulate unsigned) */
        ORBIS32_MSB,    /* l.msb    (multiply and subtract signed) */
        ORBIS32_MSBU,   /* l.msbu   (multiply and subtract signed) */
        ORBIS32_MACI,   /* l.maci   (multiply immediate and accumulate signed) */
        ORBIS32_MACRC,  /* l.macrc  (mac read and clear) */

        /* System Interface */
        ORBIS32_SYS,    /* l.sys    (system call) */
        ORBIS32_TRAP,   /* l.trap   (trap) */
        ORBIS32_MSYNC,  /* l.msync  (memory synchronization) */
        ORBIS32_PSYNC,  /* l.psync  (pipeline synchronization) */
        ORBIS32_CSYNC,  /* l.csync  (context synchronization) */
        ORBIS32_RFE,    /* l.rfe    (return from exception) */

        /* Custom Instructions */
        ORBIS32_CUST1,  /* l.cust1  (ORBIS32 custom instructions) */
        ORBIS32_CUST2,  /* l.cust2  (ORBIS32 custom instructions) */
        ORBIS32_CUST3,  /* l.cust3  (ORBIS32 custom instructions) */
        ORBIS32_CUST4,  /* l.cust4  (ORBIS32 custom instructions) */
        ORBIS32_CUST5,  /* l.cust5  (ORBIS32 custom instructions) */
        ORBIS32_CUST6,  /* l.cust6  (ORBIS32 custom instructions) */
        ORBIS32_CUST7,  /* l.cust7  (ORBIS32 custom instructions) */
        ORBIS32_CUST8,  /* l.cust8  (ORBIS32 custom instructions) */

        /* ORFPX32 */
        ORFPX32_ADD,    /* lf.add.s   (add floating point single precision) */
        ORFPX32_CUST1,  /* lf.cust1.s (ORFPX32 custom instruction 1) */
        ORFPX32_DIV,    /* lf.div.s   (divide floating point single precision) */
        ORFPX32_FTOI,   /* lf.ftoi.s  (floating point single precision to integer) */
        ORFPX32_ITOF,   /* lf.itof.s  (integer to floating point single precision) */
        ORFPX32_MADD,   /* lf.madd.s  (multiply and add floating point single precision) */
        ORFPX32_MUL,    /* lf.mul.s   (multiply floating point single precision) */
        ORFPX32_REM,    /* lf.rem.s   (remainder floating point single precision) */
        ORFPX32_SFEQ,   /* lf.sfeq.s  (set flag if equal floating point single precision) */
        ORFPX32_SFGE,   /* lf.sfge.s  (set flag if greater or equal floating point single precision) */
        ORFPX32_SFGT,   /* lf.sfgt.s  (set flag if greater than floating point single precision) */
        ORFPX32_SFLE,   /* lf.sfle.s  (set flag if less or equal floating point single precision) */
        ORFPX32_SFLT,   /* lf.sflt.s  (set flag if lass than floating point single precision) */
        ORFPX32_SFNE,   /* lf.sfne.s  (set flag if not equal floating point single precision) */
        ORFPX32_SUB,    /* lf.sub.s   (subtract floating point single precision) */

        /* ORFPX64 */
        ORFPX64_ADD,    /* lf.add.s   (add floating point double precision) */
        ORFPX64_SUB,    /* lf.sub.s   (subtract floating point double precision) */
        ORFPX64_MUL,    /* lf.mul.s   (multiply floating point double precision) */
        ORFPX64_DIV,    /* lf.div.s   (divide floating point double precision) */
        ORFPX64_ITOF,   /* lf.itof.s  (integer to floating point double precision) */
        ORFPX64_FTOI,   /* lf.ftoi.s  (floating point double precision to integer) */
        ORFPX64_REM,    /* lf.rem.s   (remainder floating point double precision) */
        ORFPX64_MADD,   /* lf.madd.s  (multiply and add floating point double precision) */
        ORFPX64_SFEQ,   /* lf.sfeq.s  (set flag if equal floating point double precision) */
        ORFPX64_SFNE,   /* lf.sfne.s  (set flag if not equal floating point double precision) */
        ORFPX64_SFGT,   /* lf.sfgt.s  (set flag if greater than floating point double precision) */
        ORFPX64_SFGE,   /* lf.sfge.s  (set flag if greater or equal floating point double precision) */
        ORFPX64_SFLT,   /* lf.sflt.s  (set flag if lass than floating point double precision) */
        ORFPX64_SFLE,   /* lf.sfle.s  (set flag if less or equal floating point double precision) */
        ORFPX64_CUST1,  /* lf.cust1.s (ORFPX64 custom instruction 1) */

        /**/
        NUM_OPCODES
    };

    opcode decode(u32 insn);

}

#endif
