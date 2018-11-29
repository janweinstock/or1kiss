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

#include "or1kiss/disasm.h"

namespace or1kiss {

    static const char* g_opcode_str[NUM_OPCODES] = {
            "invalid opcode",
            "l.nop",
            "l.mfspr",
            "l.mtspr",
            "l.movhi",

            "l.j",
            "l.jr",
            "l.jal",
            "l.jalr",
            "l.bf",
            "l.bnf",

            "l.lwa",
            "l.lwz",
            "l.lws",
            "l.lhz",
            "l.lhs",
            "l.lbz",
            "l.lbs",
            "l.swa",
            "l.sw",
            "l.sh",
            "l.sb",

            "l.extwz",
            "l.extws",
            "l.exthz",
            "l.exths",
            "l.extbz",
            "l.extbs",

            "l.add",
            "l.addc",
            "l.sub",
            "l.and",
            "l.or",
            "l.xor",
            "l.cmov",
            "l.ff1",
            "l.fl1",
            "l.sll",
            "l.srl",
            "l.sra",
            "l.ror",
            "l.mul",
            "l.mulu",
            "l.muld",
            "l.muldu",
            "l.div",
            "l.divu",

            "l.addi",
            "l.addic",
            "l.andi",
            "l.ori",
            "l.xori",
            "l.muli",
            "l.slli",
            "l.srli",
            "l.srai",
            "l.rori",

            "l.sfeq",
            "l.sfne",
            "l.sfgtu",
            "l.sfgeu",
            "l.sfltu",
            "l.sfleu",
            "l.sfgts",
            "l.sfges",
            "l.sflts",
            "l.sfles",

            "l.sfeqi",
            "l.sfnei",
            "l.sfgtui",
            "l.sfgeui",
            "l.sfltui",
            "l.sfleui",
            "l.sfgtsi",
            "l.sfgesi",
            "l.sfltsi",
            "l.sflesi",

            "l.mac",
            "l.macu",
            "l.msb",
            "l.msbu",
            "l.maci",
            "l.macrc",

            "l.sys",
            "l.trap",
            "l.msync",
            "l.psync",
            "l.csync",
            "l.rfe",

            "l.cust1",
            "l.cust2",
            "l.cust3",
            "l.cust4",
            "l.cust5",
            "l.cust6",
            "l.cust7",
            "l.cust8",

            "lf.add.s",
            "lf.cust1.s",
            "lf.div.s",
            "lf.ftoi.s",
            "lf.itof.s",
            "lf.madd.s",
            "lf.mul.s",
            "lf.rem.s",
            "lf.sfeq.s",
            "lf.sfge.s",
            "lf.sfgt.s",
            "lf.sfle.s",
            "lf.sflt.s",
            "lf.sfne.s",
            "lf.sub.s",

            "lf.add.d",
            "lf.sub.d",
            "lf.mul.d",
            "lf.div.d",
            "lf.itof.d",
            "lf.ftoi.d",
            "lf.rem.d",
            "lf.madd.d",
            "lf.sfeq.d",
            "lf.sfne.d",
            "lf.sfgt.d",
            "lf.sfge.d",
            "lf.sflt.d",
            "lf.sfle.d",
            "lf.cust1.d"
    };

    static std::string reg_d(u32 insn) {
        std::stringstream ss;
        ss << "r" << bits32(insn, 25, 21);
        return ss.str();
    }

    static std::string reg_a(u32 insn) {
        std::stringstream ss;
        ss << "r" << bits32(insn, 20, 16);
        return ss.str();
    }

    static std::string reg_b(u32 insn) {
        std::stringstream ss;
        ss << "r" << bits32(insn, 15, 11);
        return ss.str();
    }

    static std::string imm_i(u32 insn) {
        s32 imm = static_cast<s32>(sign_extend32(insn, 15));
        std::stringstream ss;
        if (imm >= 0)
            ss << "0x" << std::hex << imm;
        else
            ss << imm;
        return ss.str();
    }

    static std::string imm_i2(u32 insn) {
        u16 temp = (bits32(insn, 25, 21) << 11) |
                        (bits32(insn, 10,  0) <<  0);
        s16 imm = static_cast<s16>(temp);
        std::stringstream ss;
        if (imm >= 0)
            ss << "0x" << std::hex << imm;
        else
            ss << imm;
        return ss.str();
    }

    static std::string imm_k(u32 insn) {
        u32 imm = bits32(insn, 15,  0);
        if (imm == 0)
            return "0";
        std::stringstream ss;
        ss << "0x" << std::hex << imm;
        return ss.str();
    }

    static std::string imm_k2(u32 insn) {
        u32 imm = (bits32(insn, 25, 21) << 11) |
                       (bits32(insn, 10,  0) <<  0);
        if (imm == 0)
            return "0";
        std::stringstream ss;
        ss << "0x" << std::hex << imm;
        return ss.str();
    }

    static std::string imm_l(u32 insn) {
        u32 imm = bits32(insn, 5,  0);
        if (imm == 0)
            return "0";
        std::stringstream ss;
        ss << "0x" << std::hex << imm << std::dec;
        return ss.str();
    }

    static std::string imm_n(u32 insn) {
        s32 imm = static_cast<s32>(sign_extend32(insn, 25));
        std::stringstream ss;
        if (imm >= 0)
            ss << "0x" << std::hex << imm;
        else
            ss << imm;
        return ss.str();
    }

    void disassemble(ostream& os, u32 insn) {
        opcode opcode = decode(insn);
        if (opcode == INVALID_OPCODE) {
            os << "invalid opcode";
            return;
        }

        // Decode opcode
        os.width(7);
        os.fill(' ');
        os << std::left << g_opcode_str[opcode] << std::right;

        // Decode instruction parameters (if there are any)
        switch (opcode) {
        case ORBIS32_NOP:
            os << " "
               << imm_k(insn);
            break;

        case ORBIS32_MFSPR:
            os << " "
               << reg_d(insn)
               << ","
               << reg_a(insn)
               << ","
               << imm_k(insn);
            break;

        case ORBIS32_MTSPR:
            os << " "
               << reg_a(insn)
               << ","
               << reg_b(insn)
               << ","
               << imm_k2(insn);
            break;

        case ORBIS32_MOVHI:
            os << " "
               << reg_d(insn)
               << ","
               << imm_k(insn);
            break;

        case ORBIS32_BF:
        case ORBIS32_BNF:
        case ORBIS32_J:
        case ORBIS32_JAL:
            os << " "
               << imm_n(insn);
            break;

        case ORBIS32_JR:
        case ORBIS32_JALR:
            os << " "
               << reg_b(insn);
            break;

        case ORBIS32_LWZ:
        case ORBIS32_LWS:
        case ORBIS32_LHZ:
        case ORBIS32_LHS:
        case ORBIS32_LBZ:
        case ORBIS32_LBS:
            os << " "
               << reg_d(insn)
               << ","
               << imm_i(insn)
               << "("
               << reg_a(insn)
               << ")";
            break;

        case ORBIS32_SW:
        case ORBIS32_SH:
        case ORBIS32_SB:
            os << " "
               << imm_i2(insn)
               << "("
               << reg_a(insn)
               << "),"
               << reg_b(insn);
            break;

        case ORBIS32_EXTWS:
        case ORBIS32_EXTWZ:
        case ORBIS32_EXTHS:
        case ORBIS32_EXTHZ:
        case ORBIS32_EXTBS:
        case ORBIS32_EXTBZ:
            os << " "
               << reg_d(insn)
               << ","
               << reg_a(insn);
            break;

        case ORBIS32_ADD:
        case ORBIS32_ADDC:
        case ORBIS32_SUB:
        case ORBIS32_AND:
        case ORBIS32_OR:
        case ORBIS32_XOR:
        case ORBIS32_CMOV:
        case ORBIS32_SLL:
        case ORBIS32_SRL:
        case ORBIS32_SRA:
        case ORBIS32_ROR:
        case ORBIS32_MULU:
        case ORBIS32_MULDU:
        case ORBIS32_DIVU:
        case ORBIS32_MUL:
        case ORBIS32_MULD:
        case ORBIS32_DIV:
            os << " "
               << reg_d(insn)
               << ","
               << reg_a(insn)
               << ","
               << reg_b(insn);
            break;

        case ORBIS32_FF1:
        case ORBIS32_FL1:
            os << " "
               << reg_d(insn)
               << ","
               << reg_a(insn);
            break;

        case ORBIS32_ADDI:
        case ORBIS32_ADDIC:
        case ORBIS32_XORI:
            os << " "
               << reg_d(insn)
               << ","
               << reg_a(insn)
               << ","
               << imm_i(insn);
            break;

        case ORBIS32_ANDI:
        case ORBIS32_ORI:
        case ORBIS32_MULI:
            os << " "
               << reg_d(insn)
               << ","
               << reg_a(insn)
               << ","
               << imm_k(insn);
            break;

        case ORBIS32_SLLI:
        case ORBIS32_SRLI:
        case ORBIS32_SRAI:
        case ORBIS32_RORI:
            os << " "
               << reg_d(insn)
               << ","
               << reg_a(insn)
               << ","
               << imm_l(insn);
            break;

        case ORBIS32_SFEQ:
        case ORBIS32_SFNE:
        case ORBIS32_SFGTU:
        case ORBIS32_SFGEU:
        case ORBIS32_SFLTU:
        case ORBIS32_SFLEU:
        case ORBIS32_SFGTS:
        case ORBIS32_SFGES:
        case ORBIS32_SFLTS:
        case ORBIS32_SFLES:
            os << " "
               << reg_a(insn)
               << ","
               << reg_b(insn);
            break;

        case ORBIS32_SFEQI:
        case ORBIS32_SFNEI:
        case ORBIS32_SFGTUI:
        case ORBIS32_SFGEUI:
        case ORBIS32_SFLTUI:
        case ORBIS32_SFLEUI:
        case ORBIS32_SFGTSI:
        case ORBIS32_SFGESI:
        case ORBIS32_SFLTSI:
        case ORBIS32_SFLESI:
            os << " "
               << reg_a(insn)
               << ","
               << imm_i(insn);
            break;

        case ORFPX32_ADD:
        case ORFPX32_SUB:
        case ORFPX32_MUL:
        case ORFPX32_DIV:
        case ORFPX32_REM:
        case ORFPX32_MADD:
            os << " "
               << reg_d(insn)
               << ","
               << reg_a(insn)
               << ","
               << reg_b(insn);
            break;

        case ORFPX32_ITOF:
        case ORFPX32_FTOI:
            os << " "
               << reg_d(insn)
               << ","
               << reg_a(insn);
            break;

        case ORFPX32_SFEQ:
        case ORFPX32_SFNE:
        case ORFPX32_SFGT:
        case ORFPX32_SFGE:
        case ORFPX32_SFLT:
        case ORFPX32_SFLE:
            os << " "
               << reg_a(insn)
               << ","
               << reg_b(insn);
            break;

        case ORFPX64_ADD:
        case ORFPX64_SUB:
        case ORFPX64_MUL:
        case ORFPX64_DIV:
        case ORFPX64_REM:
        case ORFPX64_MADD:
            os << " "
               << reg_d(insn)
               << ","
               << reg_a(insn)
               << ","
               << reg_b(insn);
            break;

        case ORFPX64_ITOF:
        case ORFPX64_FTOI:
            os << " "
               << reg_d(insn)
               << ","
               << reg_a(insn);
            break;

        case ORFPX64_SFEQ:
        case ORFPX64_SFNE:
        case ORFPX64_SFGT:
        case ORFPX64_SFGE:
        case ORFPX64_SFLT:
        case ORFPX64_SFLE:
            os << " "
               << reg_a(insn)
               << ","
               << reg_b(insn);
            break;

        default:
            break;
        }
    }

    string disassemble(u32 insn) {
        stringstream ss;
        disassemble(ss, insn);
        return ss.str();
    }
}
