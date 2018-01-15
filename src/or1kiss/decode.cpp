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

#include "or1kiss/or1k.h"

namespace or1kiss {

    // ALU instructions
    static opcode decode_alu(u32 insn) {
        u32 opcode1 = bits32(insn, 9, 8);
        u32 opcode2 = bits32(insn, 3, 0);

        if (opcode1 == 0x0) {
            switch (opcode2) {
            case 0x0: return ORBIS32_ADD;
            case 0x1: return ORBIS32_ADDC;
            case 0x2: return ORBIS32_SUB;
            case 0x3: return ORBIS32_AND;
            case 0x4: return ORBIS32_OR;
            case 0x5: return ORBIS32_XOR;
            case 0xe: return ORBIS32_CMOV;
            case 0xf: return ORBIS32_FF1;
            default: break;
            }
        } else if (opcode1 == 0x1) {
            switch (opcode2) {
            case 0xf: return ORBIS32_FL1;
            default:  return INVALID_OPCODE;
            }
        } else if (opcode1 == 0x2) {
            switch (opcode2) {
            default:  return INVALID_OPCODE;
            }
        } else if (opcode1 == 0x3) {
            switch (opcode2) {
            case 0x6: return ORBIS32_MUL;
            case 0x7: return ORBIS32_MULD;
            case 0x9: return ORBIS32_DIV;
            case 0xa: return ORBIS32_DIVU;
            case 0xb: return ORBIS32_MULU;
            case 0xc: return ORBIS32_MULDU;
            default:  return INVALID_OPCODE;
            }
        }

        opcode1 = bits32(insn, 9, 6);
        if (opcode1 == 0x0) {
            switch (opcode2) {
            case 0x8: return ORBIS32_SLL;
            case 0xc: return ORBIS32_EXTHS;
            case 0xd: return ORBIS32_EXTWS;
            default:  return INVALID_OPCODE;
            }
        } else if (opcode1 == 0x1) {
            switch (opcode2) {
            case 0x8: return ORBIS32_SRL;
            case 0xc: return ORBIS32_EXTBS;
            case 0xd: return ORBIS32_EXTWZ;
            default:  return INVALID_OPCODE;
            }
        } else if (opcode1 == 0x2) {
            switch (opcode2) {
            case 0x8: return ORBIS32_SRA;
            case 0xc: return ORBIS32_EXTHZ;
            default:  return INVALID_OPCODE;
            }
        } else if (opcode1 == 0x3) {
            switch (opcode2) {
            case 0x8: return ORBIS32_ROR;
            case 0xc: return ORBIS32_EXTBZ;
            default:  return INVALID_OPCODE;
            }
        }

        return INVALID_OPCODE;
    }

    static opcode decode_util(u32 insn) {
        u32 opcode1 = bits32(insn, 16, 0);
        if (opcode1 == 0x10000)
            return ORBIS32_MACRC;

        u32 opcode2 = bits32(insn, 16, 16);
        if (opcode2 == 0x0)
            return ORBIS32_MOVHI;

        return INVALID_OPCODE;
    }

    // Shift/Rotate with Immediate
    static opcode decode_shift(u32 insn) {
        switch (bits32(insn, 7, 6)) {
        case 0x0: return ORBIS32_SLLI;
        case 0x1: return ORBIS32_SRLI;
        case 0x2: return ORBIS32_SRAI;
        case 0x3: return ORBIS32_RORI;
        default:  return INVALID_OPCODE;
        }
    }

    // MAC Unit based instructions
    static opcode decode_mac(u32 insn) {
        switch (insn & 0xf) {
        case 0x1: return ORBIS32_MAC;
        case 0x2: return ORBIS32_MSB;
        case 0x3: return ORBIS32_MACU;
        case 0x4: return ORBIS32_MSBU;
        default:  return INVALID_OPCODE;
        }
    }

    // FPU instructions
    static opcode decode_fpx(u32 insn) {
        u32 opcode = insn & 0xff;
        switch (opcode) {
        case 0x0:  return ORFPX32_ADD;
        case 0x1:  return ORFPX32_SUB;
        case 0x2:  return ORFPX32_MUL;
        case 0x3:  return ORFPX32_DIV;
        case 0x4:  return ORFPX32_ITOF;
        case 0x5:  return ORFPX32_FTOI;
        case 0x6:  return ORFPX32_REM;
        case 0x7:  return ORFPX32_MADD;
        case 0x8:  return ORFPX32_SFEQ;
        case 0x9:  return ORFPX32_SFNE;
        case 0xa:  return ORFPX32_SFGT;
        case 0xb:  return ORFPX32_SFGE;
        case 0xc:  return ORFPX32_SFLT;
        case 0xd:  return ORFPX32_SFLE;
        case 0x10: return ORFPX64_ADD;
        case 0x11: return ORFPX64_SUB;
        case 0x12: return ORFPX64_MUL;
        case 0x13: return ORFPX64_DIV;
        case 0x14: return ORFPX64_ITOF;
        case 0x15: return ORFPX64_FTOI;
        case 0x16: return ORFPX64_REM;
        case 0x17: return ORFPX64_MADD;
        case 0x18: return ORFPX64_SFEQ;
        case 0x19: return ORFPX64_SFNE;
        case 0x1a: return ORFPX64_SFGT;
        case 0x1b: return ORFPX64_SFGE;
        case 0x1c: return ORFPX64_SFLT;
        case 0x1d: return ORFPX64_SFLE;

        default:
            switch (opcode >> 4) {
            case 0xd: return ORFPX32_CUST1;
            case 0xe: return ORFPX64_CUST1;
            default:  break;
            }

            return INVALID_OPCODE;
        }
    }

    opcode decode(u32 insn) {
        switch (bits32(insn, 31, 26)) {
        case 0x38: return decode_alu(insn);
        case 0x06: return decode_util(insn);
        case 0x2e: return decode_shift(insn);
        case 0x31: return decode_mac(insn);
        case 0x32: return decode_fpx(insn);

        // Control
        case 0x00: return ORBIS32_J;
        case 0x01: return ORBIS32_JAL;
        case 0x03: return ORBIS32_BNF;
        case 0x04: return ORBIS32_BF;
        case 0x11: return ORBIS32_JR;
        case 0x12: return ORBIS32_JALR;

        // ALU Immediate
        case 0x27: return ORBIS32_ADDI;
        case 0x28: return ORBIS32_ADDIC;
        case 0x29: return ORBIS32_ANDI;
        case 0x2a: return ORBIS32_ORI;
        case 0x2b: return ORBIS32_XORI;
        case 0x2c: return ORBIS32_MULI;

        // Load & Store
        case 0x1b: return ORBIS32_LWA;
        case 0x20: return ORBIS32_LD;
        case 0x21: return ORBIS32_LWZ;
        case 0x22: return ORBIS32_LWS;
        case 0x23: return ORBIS32_LBZ;
        case 0x24: return ORBIS32_LBS;
        case 0x25: return ORBIS32_LHZ;
        case 0x26: return ORBIS32_LHS;
        case 0x33: return ORBIS32_SWA;
        case 0x34: return ORBIS32_SD;
        case 0x35: return ORBIS32_SW;
        case 0x36: return ORBIS32_SB;
        case 0x37: return ORBIS32_SH;

        // System Interface
        case 0x09: return ORBIS32_RFE;
        case 0x2d: return ORBIS32_MFSPR;
        case 0x30: return ORBIS32_MTSPR;
        case 0x13: return ORBIS32_MACI;

        // Custom Instructions
        case 0x1c: return ORBIS32_CUST1;
        case 0x1d: return ORBIS32_CUST2;
        case 0x1e: return ORBIS32_CUST3;
        case 0x1f: return ORBIS32_CUST4;
        case 0x3c: return ORBIS32_CUST5;
        case 0x3d: return ORBIS32_CUST6;
        case 0x3e: return ORBIS32_CUST7;
        case 0x3f: return ORBIS32_CUST8;
        default:   break;
        }

        // No operation (8 bits opcode)
        switch (bits32(insn, 31, 24)) {
        case 0x15: return ORBIS32_NOP;
        default:   break;
        }

        // Comparison (11 bits opcode)
        switch (bits32(insn, 31, 21)) {
        case 0x5e0: return ORBIS32_SFEQI;
        case 0x5e1: return ORBIS32_SFNEI;
        case 0x5e2: return ORBIS32_SFGTUI;
        case 0x5e3: return ORBIS32_SFGEUI;
        case 0x5e4: return ORBIS32_SFLTUI;
        case 0x5e5: return ORBIS32_SFLEUI;
        case 0x5ea: return ORBIS32_SFGTSI;
        case 0x5eb: return ORBIS32_SFGESI;
        case 0x5ec: return ORBIS32_SFLTSI;
        case 0x5ed: return ORBIS32_SFLESI;
        case 0x720: return ORBIS32_SFEQ;
        case 0x721: return ORBIS32_SFNE;
        case 0x722: return ORBIS32_SFGTU;
        case 0x723: return ORBIS32_SFGEU;
        case 0x724: return ORBIS32_SFLTU;
        case 0x725: return ORBIS32_SFLEU;
        case 0x72a: return ORBIS32_SFGTS;
        case 0x72b: return ORBIS32_SFGES;
        case 0x72c: return ORBIS32_SFLTS;
        case 0x72d: return ORBIS32_SFLES;
        default:    break;
        }

        // System Interface (16 bits opcode)
        switch (bits32(insn, 31, 16)) {
        case 0x2000: return ORBIS32_SYS;
        case 0x2100: return ORBIS32_TRAP;
        }

        // System Interface (32 bits opcode)
        switch (insn) {
        case 0x22000000: return ORBIS32_MSYNC;
        case 0x23000000: return ORBIS32_CSYNC;
        case 0x22800000: return ORBIS32_PSYNC;
        default:         break;
        }

        // Nothing found, illegal instruction
        return INVALID_OPCODE;
    }

    void or1k::decode_orbis32_mfspr(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 A = bits32(m_insn, 20, 16);
        u32 K = bits32(m_insn, 15,  0);

        ci->imm  = K;
        ci->src1 = GPR + A;
        ci->src2 = &ci->imm;
        ci->dest = GPR + D;
        ci->exec = &or1k::execute_orbis32_mfspr;
    }

    void or1k::decode_orbis32_mtspr(instruction* ci) {
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);
        u32 K = (bits32(m_insn, 25, 21) << 11) |
                     (bits32(m_insn, 10,  0) <<  0);

        ci->imm  = K;
        ci->src1 = GPR + B;
        ci->src2 = &ci->imm;
        ci->dest = GPR + A;
        ci->exec = &or1k::execute_orbis32_mtspr;
    }

    void or1k::decode_orbis32_movhi(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 K = bits32(m_insn, 15, 0) << 16;

        ci->imm  = K;
        ci->dest = GPR + D;
        ci->exec = &or1k::execute_orbis32_movhi;
    }

    void or1k::decode_orbis32_nop(instruction* ci) {
        u32 K = bits32(m_insn, 15, 0);

        ci->imm  = K;
        ci->src1 = GPR + 3;
        ci->exec = &or1k::execute_orbis32_nop;
    }

    void or1k::decode_orbis32_bf(instruction* ci) {
        u32 N = bits32(m_insn, 25, 0);

        ci->imm  = sign_extend32(N << 2, 27);
        ci->exec = &or1k::execute_orbis32_bf;
    }

    void or1k::decode_orbis32_bnf(instruction* ci) {
        u32 N = bits32(m_insn, 25, 0);

        ci->imm  = sign_extend32(N << 2, 27);
        ci->exec = &or1k::execute_orbis32_bnf;
    }

    void or1k::decode_orbis32_j(instruction* ci) {
        u32 N = bits32(m_insn, 25, 0);

        ci->imm  = sign_extend32(N << 2, 27);
        ci->src1 = NULL;
        ci->src2 = &ci->imm;
        ci->exec = &or1k::execute_orbis32_jump_rel;
    }

    void or1k::decode_orbis32_jr(instruction* ci) {
        u32 B = bits32(m_insn, 15, 11);

        ci->src1 = NULL;
        ci->src2 = GPR + B;
        ci->exec = &or1k::execute_orbis32_jump_abs;
    }

    void or1k::decode_orbis32_jal(instruction* ci) {
        u32 N = bits32(m_insn, 25, 0);

        ci->imm  = sign_extend32(N << 2, 27);
        ci->src1 = GPR + 9;
        ci->src2 = &ci->imm;
        ci->exec = &or1k::execute_orbis32_jump_rel;
    }

    void or1k::decode_orbis32_jalr(instruction* ci) {
        u32 B = bits32(m_insn, 15, 11);

        ci->src1 = GPR + 9;
        ci->src2 = GPR + B;
        ci->exec = &or1k::execute_orbis32_jump_abs;
    }

    void or1k::decode_orbis32_lwa(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 A = bits32(m_insn, 20, 16);
        u32 I = bits32(m_insn, 15,  0);

        ci->imm  = sign_extend32(I, 15);
        ci->src1 = GPR + A;
        ci->dest = GPR + D;
        ci->exec = &or1k::execute_orbis32_lwa;
    }

    void or1k::decode_orbis32_lwz(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 A = bits32(m_insn, 20, 16);
        u32 I = bits32(m_insn, 15,  0);

        ci->imm  = sign_extend32(I, 15);
        ci->src1 = GPR + A;
        ci->dest = GPR + D;
        ci->exec = &or1k::execute_orbis32_lw;
    }

    void or1k::decode_orbis32_lws(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 A = bits32(m_insn, 20, 16);
        u32 I = bits32(m_insn, 15,  0);

        ci->imm  = sign_extend32(I, 15);
        ci->src1 = GPR + A;
        ci->dest = GPR + D;
        ci->exec = &or1k::execute_orbis32_lw;
    }

    void or1k::decode_orbis32_lhz(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 A = bits32(m_insn, 20, 16);
        u32 I = bits32(m_insn, 15,  0);

        ci->imm  = sign_extend32(I, 15);
        ci->src1 = GPR + A;
        ci->dest = GPR + D;
        ci->exec = &or1k::execute_orbis32_lhz;
    }

    void or1k::decode_orbis32_lhs(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 A = bits32(m_insn, 20, 16);
        u32 I = bits32(m_insn, 15,  0);

        ci->imm  = sign_extend32(I, 15);
        ci->src1 = GPR + A;
        ci->dest = GPR + D;
        ci->exec = &or1k::execute_orbis32_lhs;
    }

    void or1k::decode_orbis32_lbz(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 A = bits32(m_insn, 20, 16);
        u32 I = bits32(m_insn, 15,  0);

        ci->imm  = sign_extend32(I, 15);
        ci->src1 = GPR + A;
        ci->dest = GPR + D;
        ci->exec = &or1k::execute_orbis32_lbz;
    }

    void or1k::decode_orbis32_lbs(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 A = bits32(m_insn, 20, 16);
        u32 I = bits32(m_insn, 15,  0);

        ci->imm  = sign_extend32(I, 15);
        ci->src1 = GPR + A;
        ci->dest = GPR + D;
        ci->exec = &or1k::execute_orbis32_lbs;
    }

    void or1k::decode_orbis32_swa(instruction* ci) {
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);
        u32 I = (bits32(m_insn, 25, 21) << 11) |
                     (bits32(m_insn, 10,  0) <<  0);

        ci->imm  = sign_extend32(I, 15);
        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->exec = &or1k::execute_orbis32_swa;
    }

    void or1k::decode_orbis32_sw(instruction* ci) {
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);
        u32 I = (bits32(m_insn, 25, 21) << 11) |
                     (bits32(m_insn, 10,  0) <<  0);

        ci->imm  = sign_extend32(I, 15);
        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->exec = &or1k::execute_orbis32_sw;
    }

    void or1k::decode_orbis32_sh(instruction* ci) {
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);
        u32 I = (bits32(m_insn, 25, 21) << 11) |
                     (bits32(m_insn, 10,  0) <<  0);

        ci->imm  = sign_extend32(I, 15);
        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->exec = &or1k::execute_orbis32_sh;
    }

    void or1k::decode_orbis32_sb(instruction* ci) {
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);
        u32 I = (bits32(m_insn, 25, 21) << 11) |
                     (bits32(m_insn, 10,  0) <<  0);

        ci->imm  = sign_extend32(I, 15);
        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->exec = &or1k::execute_orbis32_sb;
    }

    void or1k::decode_orbis32_extwz(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 A = bits32(m_insn, 20, 16);

        ci->src1 = GPR + A;
        ci->dest = GPR + D;
        ci->exec = &or1k::execute_orbis32_extw;
    }

    void or1k::decode_orbis32_extws(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 A = bits32(m_insn, 20, 16);

        ci->src1 = GPR + A;
        ci->dest = GPR + D;
        ci->exec = &or1k::execute_orbis32_extw;
    }

    void or1k::decode_orbis32_exthz(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 A = bits32(m_insn, 20, 16);

        ci->src1 = GPR + A;
        ci->dest = GPR + D;
        ci->exec = &or1k::execute_orbis32_exthz;
    }

    void or1k::decode_orbis32_exths(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 A = bits32(m_insn, 20, 16);

        ci->src1 = GPR + A;
        ci->dest = GPR + D;
        ci->exec = &or1k::execute_orbis32_exths;
    }

    void or1k::decode_orbis32_extbz(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 A = bits32(m_insn, 20, 16);

        ci->src1 = GPR + A;
        ci->dest = GPR + D;
        ci->exec = &or1k::execute_orbis32_extbz;
    }

    void or1k::decode_orbis32_extbs(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 A = bits32(m_insn, 20, 16);

        ci->src1 = GPR + A;
        ci->dest = GPR + D;
        ci->exec = &or1k::execute_orbis32_extbs;
    }

    void or1k::decode_orbis32_add(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);

        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->dest = GPR + D;
        ci->exec = &or1k::execute_orbis32_add;
    }

    void or1k::decode_orbis32_addc(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);

        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->dest = GPR + D;
        ci->exec = &or1k::execute_orbis32_addc;
    }

    void or1k::decode_orbis32_sub(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);

        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->dest = GPR + D;
        ci->exec = &or1k::execute_orbis32_sub;
    }

    void or1k::decode_orbis32_and(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);

        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->dest = GPR + D;
        ci->exec = &or1k::execute_orbis32_and;
    }

    void or1k::decode_orbis32_or(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);

        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->dest = GPR + D;
        ci->exec = &or1k::execute_orbis32_or;
    }

    void or1k::decode_orbis32_xor(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);

        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->dest = GPR + D;
        ci->exec = &or1k::execute_orbis32_xor;
    }

    void or1k::decode_orbis32_cmov(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);

        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->dest = GPR + D;
        ci->exec = &or1k::execute_orbis32_cmov;
    }

    void or1k::decode_orbis32_ff1(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 A = bits32(m_insn, 20, 16);

        ci->src1 = GPR + A;
        ci->dest = GPR + D;
        ci->exec = &or1k::execute_orbis32_ff1;
    }

    void or1k::decode_orbis32_fl1(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 A = bits32(m_insn, 20, 16);

        ci->src1 = GPR + A;
        ci->dest = GPR + D;
        ci->exec = &or1k::execute_orbis32_fl1;
    }


    void or1k::decode_orbis32_sll(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);

        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->dest = GPR + D;
        ci->exec = &or1k::execute_orbis32_sll;
    }

    void or1k::decode_orbis32_srl(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);

        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->dest = GPR + D;
        ci->exec = &or1k::execute_orbis32_srl;
    }

    void or1k::decode_orbis32_sra(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);

        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->dest = GPR + D;
        ci->exec = &or1k::execute_orbis32_sra;
    }

    void or1k::decode_orbis32_ror(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);

        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->dest = GPR + D;
        ci->exec = &or1k::execute_orbis32_ror;
    }

    void or1k::decode_orbis32_mul(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);

        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->dest = GPR + D;
        ci->exec = &or1k::execute_orbis32_mul;
    }

    void or1k::decode_orbis32_mulu(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);

        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->dest = GPR + D;
        ci->exec = &or1k::execute_orbis32_mulu;
    }

    void or1k::decode_orbis32_muld(instruction* ci) {
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);

        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->exec = &or1k::execute_orbis32_muld;
    }

    void or1k::decode_orbis32_muldu(instruction* ci) {
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);

        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->exec = &or1k::execute_orbis32_muldu;
    }

    void or1k::decode_orbis32_div(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);

        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->dest = GPR + D;
        ci->exec = &or1k::execute_orbis32_div;
    }

    void or1k::decode_orbis32_divu(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);

        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->dest = GPR + D;
        ci->exec = &or1k::execute_orbis32_divu;
    }

    void or1k::decode_orbis32_addi(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 A = bits32(m_insn, 20, 16);
        u32 I = bits32(m_insn, 15,  0);

        ci->imm  = sign_extend32(I, 15);
        ci->dest = GPR + D;
        ci->src1 = GPR + A;
        ci->src2 = &ci->imm;
        ci->exec = &or1k::execute_orbis32_add;
    }

    void or1k::decode_orbis32_addic(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 A = bits32(m_insn, 20, 16);
        u32 m = bits32(m_insn, 15,  0);

        ci->imm  = sign_extend32(m, 15);
        ci->dest = GPR + D;
        ci->src1 = GPR + A;
        ci->src2 = &ci->imm;
        ci->exec = &or1k::execute_orbis32_addc;
    }

    void or1k::decode_orbis32_andi(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 A = bits32(m_insn, 20, 16);
        u32 K = bits32(m_insn, 15,  0);

        ci->imm  = K;
        ci->dest = GPR + D;
        ci->src1 = GPR + A;
        ci->src2 = &ci->imm;
        ci->exec = &or1k::execute_orbis32_and;
    }

    void or1k::decode_orbis32_ori(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 A = bits32(m_insn, 20, 16);
        u32 K = bits32(m_insn, 15,  0);

        ci->imm  = K;
        ci->dest = GPR + D;
        ci->src1 = GPR + A;
        ci->src2 = &ci->imm;
        ci->exec = &or1k::execute_orbis32_or;
    }

    void or1k::decode_orbis32_xori(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 A = bits32(m_insn, 20, 16);
        u32 I = bits32(m_insn, 15,  0);

        ci->imm  = sign_extend32(I, 15);
        ci->dest = GPR + D;
        ci->src1 = GPR + A;
        ci->src2 = &ci->imm;
        ci->exec = &or1k::execute_orbis32_xor;
    }

    void or1k::decode_orbis32_rori(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 A = bits32(m_insn, 20, 16);
        u32 L = bits32(m_insn,  5,  0);

        ci->imm  = L;
        ci->dest = GPR + D;
        ci->src1 = GPR + A;
        ci->src2 = &ci->imm;
        ci->exec = &or1k::execute_orbis32_ror;
    }

    void or1k::decode_orbis32_slli(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 A = bits32(m_insn, 20, 16);
        u32 L = bits32(m_insn,  5,  0);

        ci->imm  = L;
        ci->dest = GPR + D;
        ci->src1 = GPR + A;
        ci->src2 = &ci->imm;
        ci->exec = &or1k::execute_orbis32_sll;
    }

    void or1k::decode_orbis32_srli(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 A = bits32(m_insn, 20, 16);
        u32 L = bits32(m_insn,  5,  0);

        ci->imm  = L;
        ci->dest = GPR + D;
        ci->src1 = GPR + A;
        ci->src2 = &ci->imm;
        ci->exec = &or1k::execute_orbis32_srl;
    }

    void or1k::decode_orbis32_srai(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 A = bits32(m_insn, 20, 16);
        u32 L = bits32(m_insn,  5,  0);

        ci->imm  = L;
        ci->dest = GPR + D;
        ci->src1 = GPR + A;
        ci->src2 = &ci->imm;
        ci->exec = &or1k::execute_orbis32_sra;
    }

    void or1k::decode_orbis32_muli(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 A = bits32(m_insn, 20, 16);
        u32 I = bits32(m_insn, 15,  0);

        ci->imm  = sign_extend32(I, 15);
        ci->dest = GPR + D;
        ci->src1 = GPR + A;
        ci->src2 = &ci->imm;
        ci->exec = &or1k::execute_orbis32_mul;
    }

    void or1k::decode_orbis32_sfeq(instruction* ci) {
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);

        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->exec = &or1k::execute_orbis32_sfeq;
    }

    void or1k::decode_orbis32_sfne(instruction* ci) {
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);

        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->exec = &or1k::execute_orbis32_sfne;
    }

    void or1k::decode_orbis32_sfgtu(instruction* ci) {
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);

        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->exec = &or1k::execute_orbis32_sfgtu;
    }

    void or1k::decode_orbis32_sfgeu(instruction* ci) {
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);

        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->exec = &or1k::execute_orbis32_sfgeu;
    }

    void or1k::decode_orbis32_sfltu(instruction* ci) {
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);

        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->exec = &or1k::execute_orbis32_sfltu;
    }

    void or1k::decode_orbis32_sfleu(instruction* ci) {
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);

        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->exec = &or1k::execute_orbis32_sfleu;
    }

    void or1k::decode_orbis32_sfgts(instruction* ci) {
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);

        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->exec = &or1k::execute_orbis32_sfgts;
    }

    void or1k::decode_orbis32_sfges(instruction* ci) {
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);

        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->exec = &or1k::execute_orbis32_sfges;
    }

    void or1k::decode_orbis32_sflts(instruction* ci) {
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);

        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->exec = &or1k::execute_orbis32_sflts;
    }

    void or1k::decode_orbis32_sfles(instruction* ci) {
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);

        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->exec = &or1k::execute_orbis32_sfles;
    }

    void or1k::decode_orbis32_sfeqi(instruction* ci) {
        u32 A = bits32(m_insn, 20, 16);
        u32 I = bits32(m_insn, 15,  0);

        ci->imm  = sign_extend32(I, 15);
        ci->src1 = GPR + A;
        ci->src2 = &(ci->imm);
        ci->exec = &or1k::execute_orbis32_sfeq;
    }

    void or1k::decode_orbis32_sfnei(instruction* ci) {
        u32 A = bits32(m_insn, 20, 16);
        u32 I = bits32(m_insn, 15,  0);

        ci->imm  = sign_extend32(I, 15);
        ci->src1 = GPR + A;
        ci->src2 = &ci->imm;
        ci->exec = &or1k::execute_orbis32_sfne;
    }

    void or1k::decode_orbis32_sfgtui(instruction* ci) {
        u32 A = bits32(m_insn, 20, 16);
        u32 I = bits32(m_insn, 15,  0);

        ci->imm  = sign_extend32(I, 15);
        ci->src1 = GPR + A;
        ci->src2 = &ci->imm;
        ci->exec = &or1k::execute_orbis32_sfgtu;
    }

    void or1k::decode_orbis32_sfgeui(instruction* ci) {
        u32 A = bits32(m_insn, 20, 16);
        u32 I = bits32(m_insn, 15,  0);

        ci->imm  = sign_extend32(I, 15);
        ci->src1 = GPR + A;
        ci->src2 = &ci->imm;
        ci->exec = &or1k::execute_orbis32_sfgeu;
    }

    void or1k::decode_orbis32_sfltui(instruction* ci) {
        u32 A = bits32(m_insn, 20, 16);
        u32 I = bits32(m_insn, 15,  0);

        ci->imm  = sign_extend32(I, 15);
        ci->src1 = GPR + A;
        ci->src2 = &ci->imm;
        ci->exec = &or1k::execute_orbis32_sfltu;
    }

    void or1k::decode_orbis32_sfleui(instruction* ci) {
        u32 A = bits32(m_insn, 20, 16);
        u32 I = bits32(m_insn, 15,  0);

        ci->imm  = sign_extend32(I, 15);
        ci->src1 = GPR + A;
        ci->src2 = &ci->imm;
        ci->exec = &or1k::execute_orbis32_sfleu;
    }

    void or1k::decode_orbis32_sfgtsi(instruction* ci) {
        u32 A = bits32(m_insn, 20, 16);
        u32 I = bits32(m_insn, 15,  0);

        ci->imm  = sign_extend32(I, 15);
        ci->src1 = GPR + A;
        ci->src2 = &ci->imm;
        ci->exec = &or1k::execute_orbis32_sfgts;
    }

    void or1k::decode_orbis32_sfgesi(instruction* ci) {
        u32 A = bits32(m_insn, 20, 16);
        u32 I = bits32(m_insn, 15,  0);

        ci->imm  = sign_extend32(I, 15);
        ci->src1 = GPR + A;
        ci->src2 = &ci->imm;
        ci->exec = &or1k::execute_orbis32_sfges;
    }

    void or1k::decode_orbis32_sfltsi(instruction* ci) {
        u32 A = bits32(m_insn, 20, 16);
        u32 I = bits32(m_insn, 15,  0);

        ci->imm  = sign_extend32(I, 15);
        ci->src1 = GPR + A;
        ci->src2 = &ci->imm;
        ci->exec = &or1k::execute_orbis32_sflts;
    }

    void or1k::decode_orbis32_sflesi(instruction* ci) {
        u32 A = bits32(m_insn, 20, 16);
        u32 I = bits32(m_insn, 15,  0);

        ci->imm  = sign_extend32(I, 15);
        ci->src1 = GPR + A;
        ci->src2 = &ci->imm;
        ci->exec = &or1k::execute_orbis32_sfles;
    }

    void or1k::decode_orbis32_mac(instruction* ci) {
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);

        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->exec = &or1k::execute_orbis32_mac;
    }

    void or1k::decode_orbis32_maci(instruction* ci) {
        u32 A = bits32(m_insn, 20, 16);
        u32 I = bits32(m_insn, 15,  0);

        ci->imm = sign_extend32(I, 15);
        ci->src1 = GPR + A;
        ci->src2 = &ci->imm;
        ci->exec = &or1k::execute_orbis32_mac;
    }

    void or1k::decode_orbis32_macu(instruction* ci) {
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);

        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->exec = &or1k::execute_orbis32_macu;
    }

    void or1k::decode_orbis32_msb(instruction* ci) {
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);

        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->exec = &or1k::execute_orbis32_msb;
    }

    void or1k::decode_orbis32_msbu(instruction* ci) {
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);

        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->exec = &or1k::execute_orbis32_msbu;
    }

    void or1k::decode_orbis32_macrc(instruction* ci) {
        u32 D = bits32(m_insn, 25, 17);

        ci->dest = GPR + D;
        ci->exec = &or1k::execute_orbis32_macrc;
    }

    void or1k::decode_orbis32_sys(instruction* ci) {
        u32 K = bits32(m_insn, 15, 0);

        ci->imm  = K;
        ci->exec = &or1k::execute_orbis32_sys;
    }

    void or1k::decode_orbis32_trap(instruction* ci) {
        u32 K = bits32(m_insn, 15, 0);

        ci->imm  = K;
        ci->exec = &or1k::execute_orbis32_trap;
    }

    void or1k::decode_orbis32_csync(instruction* ci) {
        ci->exec = &or1k::execute_orbis32_csync;
    }

    void or1k::decode_orbis32_msync(instruction* ci) {
        ci->exec = &or1k::execute_orbis32_msync;
    }

    void or1k::decode_orbis32_psync(instruction* ci) {
        ci->exec = &or1k::execute_orbis32_psync;
    }

    void or1k::decode_orbis32_rfe(instruction* ci) {
        ci->exec = &or1k::execute_orbis32_rfe;
    }

    void or1k::decode_orfpx32_add(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);

        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->dest = GPR + D;
        ci->exec = &or1k::execute_orfpx32_add;
    }

    void or1k::decode_orfpx32_sub(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);

        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->dest = GPR + D;
        ci->exec = &or1k::execute_orfpx32_sub;
    }

    void or1k::decode_orfpx32_mul(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);

        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->dest = GPR + D;
        ci->exec = &or1k::execute_orfpx32_mul;
    }

    void or1k::decode_orfpx32_div(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);

        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->dest = GPR + D;
        ci->exec = &or1k::execute_orfpx32_div;
    }

    void or1k::decode_orfpx32_itof(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 A = bits32(m_insn, 20, 16);

        ci->src1 = GPR + A;
        ci->dest = GPR + D;
        ci->exec = &or1k::execute_orfpx32_itof;
    }

    void or1k::decode_orfpx32_ftoi(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 A = bits32(m_insn, 20, 16);

        ci->src1 = GPR + A;
        ci->dest = GPR + D;
        ci->exec = &or1k::execute_orfpx32_ftoi;
    }

    void or1k::decode_orfpx32_rem(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);

        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->dest = GPR + D;
        ci->exec = &or1k::execute_orfpx32_rem;
    }

    void or1k::decode_orfpx32_madd(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);

        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->dest = GPR + D;
        ci->exec = &or1k::execute_orfpx32_madd;
    }

    void or1k::decode_orfpx32_sfeq(instruction* ci) {
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);

        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->exec = &or1k::execute_orfpx32_sfeq;
    }

    void or1k::decode_orfpx32_sfne(instruction* ci) {
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);

        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->exec = &or1k::execute_orfpx32_sfne;
    }

    void or1k::decode_orfpx32_sfgt(instruction* ci) {
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);

        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->exec = &or1k::execute_orfpx32_sfgt;
    }

    void or1k::decode_orfpx32_sfge(instruction* ci) {
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);

        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->exec = &or1k::execute_orfpx32_sfge;
    }

    void or1k::decode_orfpx32_sflt(instruction* ci) {
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);

        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->exec = &or1k::execute_orfpx32_sflt;
    }

    void or1k::decode_orfpx32_sfle(instruction* ci) {
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);

        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->exec = &or1k::execute_orfpx32_sfle;
    }

    void or1k::decode_orfpx64_add(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);

        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->dest = GPR + D;
        ci->exec = &or1k::execute_orfpx64_add;
    }

    void or1k::decode_orfpx64_sub(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);

        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->dest = GPR + D;
        ci->exec = &or1k::execute_orfpx64_sub;
    }

    void or1k::decode_orfpx64_mul(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);

        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->dest = GPR + D;
        ci->exec = &or1k::execute_orfpx64_mul;
    }

    void or1k::decode_orfpx64_div(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);

        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->dest = GPR + D;
        ci->exec = &or1k::execute_orfpx64_div;
    }

    void or1k::decode_orfpx64_itof(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 A = bits32(m_insn, 20, 16);

        ci->src1 = GPR + A;
        ci->dest = GPR + D;
        ci->exec = &or1k::execute_orfpx64_itof;
    }

    void or1k::decode_orfpx64_ftoi(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 A = bits32(m_insn, 20, 16);

        ci->src1 = GPR + A;
        ci->dest = GPR + D;
        ci->exec = &or1k::execute_orfpx64_ftoi;
    }

    void or1k::decode_orfpx64_rem(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);

        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->dest = GPR + D;
        ci->exec = &or1k::execute_orfpx64_rem;
    }

    void or1k::decode_orfpx64_madd(instruction* ci) {
        u32 D = bits32(m_insn, 25, 21);
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);

        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->dest = GPR + D;
        ci->exec = &or1k::execute_orfpx64_madd;
    }

    void or1k::decode_orfpx64_sfeq(instruction* ci) {
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);

        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->exec = &or1k::execute_orfpx64_sfeq;
    }

    void or1k::decode_orfpx64_sfne(instruction* ci) {
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);

        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->exec = &or1k::execute_orfpx64_sfne;
    }

    void or1k::decode_orfpx64_sfgt(instruction* ci) {
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);

        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->exec = &or1k::execute_orfpx64_sfgt;
    }

    void or1k::decode_orfpx64_sfge(instruction* ci) {
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);

        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->exec = &or1k::execute_orfpx64_sfge;
    }

    void or1k::decode_orfpx64_sflt(instruction* ci) {
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);

        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->exec = &or1k::execute_orfpx64_sflt;
    }

    void or1k::decode_orfpx64_sfle(instruction* ci) {
        u32 A = bits32(m_insn, 20, 16);
        u32 B = bits32(m_insn, 15, 11);

        ci->src1 = GPR + A;
        ci->src2 = GPR + B;
        ci->exec = &or1k::execute_orfpx64_sfle;
    }

    void or1k::decode_na(instruction* ci) {
        OR1KISS_ERROR("Instruction 0x%08x at address 0x%08x not supported",
                      m_insn, m_next_pc);
    }

}
