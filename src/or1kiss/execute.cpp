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

    void or1k::execute_orbis32_mfspr(instruction* ci) {
        u32* rD = ci->dest;
        u32* rA = ci->src1;
        *rD = get_spr(*rA | ci->imm);
    }

    void or1k::execute_orbis32_mtspr(instruction* ci) {
        u32* rB = ci->src1;
        u32* rA = ci->dest;
        set_spr(*rA | ci->imm, *rB);
    }

    void or1k::execute_orbis32_movhi(instruction* ci) {
        u32* rD = ci->dest;
        *rD = ci->imm;
    }

    void or1k::execute_orbis32_nop(instruction* ci) {
        nop_mode mode = static_cast<nop_mode>(ci->imm);
        switch(mode) {
        case NOP:
            break;

        case NOP_EXIT:
            std::cout << "(or1kiss) exit(" << *ci->src1 << ")" << std::endl;
            m_cycles--; // last cycle is not counted
            m_instructions--;
            m_stop_requested = true;
            break;

        case NOP_REPORT:
            std::cout << "(or1kiss) report(0x"
                      << std::setw(8)
                      << std::setfill('0')
                      << std::right
                      << std::hex
                      << *ci->src1
                      << ")" << std::endl;
            break;

        case NOP_PUTC:
            std::cout << static_cast<char>(*ci->src1) << std::flush;
            break;

        case NOP_CNT_RESET:
            std::cout << "(or1kiss) info: statistics reset" << std::endl;
            reset_instructions();
            reset_compiles();

            // Recalculate limits
            m_break_requested = true;
            break;

        case NOP_GET_TICKS:
            GPR[11] = m_cycles & 0xffffffff;
            GPR[12] = m_cycles >> 32;
            break;

        case NOP_GET_PS: {
            u64 temp = 1000000000000llu / m_clock;
            GPR[11] = static_cast<u32>(temp);
            break;
        }

        case NOP_TRACE_ON:
            m_trace_enabled = true;
            std::cout << "(or1kiss) info: tracing enabled" << std::endl;
            break;

        case NOP_TRACE_OFF:
            m_trace_enabled = false;
            std::cout << "(or1kiss) info: tracing disabled" << std::endl;
            break;

        case NOP_RANDOM:
            GPR[11] = rand();
            break;

        case NOP_OR1KSIM:
            // Although we are not really or1ksim, we use this to indicate to
            // the software that it is running in a virtual environment.
            GPR[11] = 2;
            break;

        case NOP_SILENT_EXIT:
            std::cout << "(or1kiss) silent exit("
                      << *ci->src1 << ")" << std::endl;
            m_cycles--; // last cycle is not counted
            m_instructions--;
            m_stop_requested = true;
            break;

        case NOP_HOST_TIME: {
                struct timeval tv;
                gettimeofday(&tv, NULL);
                u64 ms = tv.tv_sec * 1000ull + tv.tv_usec / 1000ull;
                GPR[11] = ms & 0xffffffffull;
                GPR[12] = ms >> 32;
            }
            break;

        case NOP_PUTS:
            std::cout << (char*)m_env->get_data_ptr(GPR[3]) << std::flush;
            break;

        default:
            break;
        }
    }

    void or1k::execute_orbis32_bf(instruction* ci) {
        u32 target = ci->imm + m_next_pc;
        u32 delay = (m_cpucfg & CPUCFGR_ND) ? 0 : 1;

        if (m_status & SR_F)
            schedule_jump(target, delay);
    }

    void or1k::execute_orbis32_bnf(instruction* ci) {
        u32 target = ci->imm + m_next_pc;
        u32 delay = (m_cpucfg & CPUCFGR_ND) ? 0 : 1;

        if (!(m_status & SR_F))
            schedule_jump(target, delay);
    }

    void or1k::execute_orbis32_jump_rel(instruction* ci) {
        u32* rL = ci->src1;
        u32* rB = ci->src2;

        u32 target = *rB + m_next_pc;
        u32 delay = (m_cpucfg & CPUCFGR_ND) ? 0 : 1;

        if (rL)
            *rL = m_next_pc + (delay + 1) * 4;

        schedule_jump(target, delay);
    }

    void or1k::execute_orbis32_jump_abs(instruction* ci) {
        u32* rL = ci->src1;
        u32* rB = ci->src2;

        u32 target = *rB;
        u32 delay = (m_cpucfg & CPUCFGR_ND) ? 0 : 1;

        if (rL)
            *rL = m_next_pc + (delay + 1) * 4;

        schedule_jump(target, delay);
    }

    void or1k::execute_orbis32_lwa(instruction* ci) {
        u32* rA = ci->src1;
        u32* rD = ci->dest;

        m_dreq.set_read();
        m_dreq.set_exclusive();

        m_dreq.addr = *rA + ci->imm;
        m_dreq.data = rD;
        m_dreq.size = SIZE_WORD;

        transact(m_dreq);
    }

    void or1k::execute_orbis32_lw(instruction* ci) {
        u32* rA = ci->src1;
        u32* rD = ci->dest;

        m_dreq.set_read();
        m_dreq.set_exclusive(false);

        m_dreq.addr = *rA + ci->imm;
        m_dreq.data = rD;
        m_dreq.size = SIZE_WORD;

        transact(m_dreq);
    }

    void or1k::execute_orbis32_lhz(instruction* ci) {
        u32* rA = ci->src1;
        u32* rD = ci->dest;

        m_dreq.set_read();
        m_dreq.set_exclusive(false);

        m_dreq.addr = *rA + ci->imm;
        m_dreq.data = rD;
        m_dreq.size = SIZE_HALFWORD;

        if (transact(m_dreq))
            *rD &= 0xffff;
    }

    void or1k::execute_orbis32_lhs(instruction* ci) {
        u32* rA = ci->src1;
        u32* rD = ci->dest;

        m_dreq.set_read();
        m_dreq.set_exclusive(false);

        m_dreq.addr = *rA + ci->imm;
        m_dreq.data = rD;
        m_dreq.size = SIZE_HALFWORD;

        if (transact(m_dreq))
            *rD = sign_extend32(*rD, 15);
    }

    void or1k::execute_orbis32_lbz(instruction* ci) {
        u32* rA = ci->src1;
        u32* rD = ci->dest;

        m_dreq.set_read();
        m_dreq.set_exclusive(false);

        m_dreq.addr = *rA + ci->imm;
        m_dreq.data = rD;
        m_dreq.size = SIZE_BYTE;

        if (transact(m_dreq))
            *rD &= 0xff;
    }

    void or1k::execute_orbis32_lbs(instruction* ci) {
        u32* rA = ci->src1;
        u32* rD = ci->dest;

        m_dreq.set_read();
        m_dreq.set_exclusive(false);

        m_dreq.addr = *rA + ci->imm;
        m_dreq.data = rD;
        m_dreq.size = SIZE_BYTE;

        if (transact(m_dreq))
            *rD = sign_extend32(*rD, 7);
    }

    void or1k::execute_orbis32_swa(instruction* ci) {
        u32* rA = ci->src1;
        u32* rB = ci->src2;

        m_dreq.set_write();
        m_dreq.set_exclusive();

        m_dreq.addr = *rA + ci->imm;
        m_dreq.data = rB;
        m_dreq.size = SIZE_WORD;

        transact(m_dreq);
    }

    void or1k::execute_orbis32_sw(instruction* ci) {
        u32* rA = ci->src1;
        u32* rB = ci->src2;

        m_dreq.set_write();
        m_dreq.set_exclusive(false);

        m_dreq.addr = *rA + ci->imm;
        m_dreq.data = rB;
        m_dreq.size = SIZE_WORD;

        transact(m_dreq);
    }

    void or1k::execute_orbis32_sh(instruction* ci) {
        u32* rA = ci->src1;
        u32* rB = ci->src2;

        m_dreq.set_write();
        m_dreq.set_exclusive(false);

        m_dreq.addr = *rA + ci->imm;
        m_dreq.data = rB;
        m_dreq.size = SIZE_HALFWORD;

        transact(m_dreq);
    }

    void or1k::execute_orbis32_sb(instruction* ci) {
        u32* rA = ci->src1;
        u32* rB = ci->src2;

        m_dreq.set_write();
        m_dreq.set_exclusive(false);

        m_dreq.addr = *rA + ci->imm;
        m_dreq.data = rB;
        m_dreq.size = SIZE_BYTE;

        transact(m_dreq);
    }

    void or1k::execute_orbis32_extw(instruction* ci) {
        u32* rA = ci->src1;
        u32* rD = ci->dest;

        *rD = *rA;
    }

    void or1k::execute_orbis32_exthz(instruction* ci) {
        u32* rA = ci->src1;
        u32* rD = ci->dest;

        *rD = *rA & 0xffff;
    }

    void or1k::execute_orbis32_exths(instruction* ci) {
        u32* rA = ci->src1;
        u32* rD = ci->dest;

        *rD = sign_extend32(*rA, 15);
    }

    void or1k::execute_orbis32_extbz(instruction* ci) {
        u32* rA = ci->src1;
        u32* rD = ci->dest;

        *rD = *rA & 0xff;
    }

    void or1k::execute_orbis32_extbs(instruction* ci) {
        u32* rA = ci->src1;
        u32* rD = ci->dest;

        *rD = sign_extend32(*rA, 7);
    }

    void or1k::execute_orbis32_add(instruction* ci) {
        u32 src1 = *ci->src1;
        u32 src2 = *ci->src2;

        // Perform operation
        u32 result = src1 + src2;
        *ci->dest = result;

        // Clear overflow flags
        m_status &= ~SR_CY;
        m_status &= ~SR_OV;

        // Check for unsigned overflow
        if (result < src1)
            m_status |= SR_CY;

        // Check for signed overflow
        s32 src1s   = static_cast<s32>(src1);
        s32 src2s   = static_cast<s32>(src2);
        s32 results = static_cast<s32>(result);
        if (((src1s <  0) && (src2s <  0) && (results >= 0)) || // neg + neg should always be neg
            ((src1s >= 0) && (src2s >= 0) && (results <  0))) { // pos + pos should always be pos
            m_status |= SR_OV;

            // If enabled, set range exception here. Note that the range
            // exception is only produced for signed overflows, not for carry
            // overflows.
            if ((m_status & SR_OVE) && (m_aecr & AE_OVADDE)) {
                m_aesr |= AE_OVADDE;
                exception(EX_RANGE);
            }
        }
    }

    void or1k::execute_orbis32_addc(instruction* ci) {
        u32 src1 = *ci->src1;
        u32 src2 = *ci->src2;

        // Perform operation
        u32 result = src1 + src2;
        if (m_status & SR_CY)
            result++;
        *ci->dest = result;

        // Clear overflow flags
        m_status &= ~SR_CY;
        m_status &= ~SR_OV;

        // Check for unsigned overflow
        if (result < src1) {
            m_status |= SR_CY;

            // If enabled, set range exception here
            if ((m_status & SR_OVE) && (m_aecr & AE_CYADDE)) {
                m_aesr |= AE_CYADDE;
                exception(EX_RANGE);
            }
        }

        // Check for signed overflow
        s32 src1s   = static_cast<s32>(src1);
        s32 src2s   = static_cast<s32>(src2);
        s32 results = static_cast<s32>(result);
        if (((src1s <  0) && (src2s <  0) && (results >= 0)) || // neg + neg should always be neg
            ((src1s >= 0) && (src2s >= 0) && (results <  0))) { // pos + pos should always be pos
            m_status |= SR_OV;

            // If enabled, set range exception here
            if ((m_status & SR_OVE) && (m_aecr & AE_OVADDE)) {
                m_aesr |= AE_OVADDE;
                exception(EX_RANGE);
            }
        }
    }

    void or1k::execute_orbis32_sub(instruction* ci) {
        u32 src1 = *ci->src1;
        u32 src2 = *ci->src2;

        // Perform operation
        u32 result = src1 - src2;
        *ci->dest = result;

        // Clear overflow flags
        m_status &= ~SR_CY;
        m_status &= ~SR_OV;

        // Check for unsigned overflow
        if (src2 > src1) {
            m_status |= SR_CY;

            // If enabled, set range exception here
            if ((m_status & SR_OVE) && (m_aecr & AE_CYADDE)) {
                m_aesr |= AE_CYADDE;
                exception(EX_RANGE);
            }
        }

        // Check for signed overflow
        s32 src1s   = static_cast<s32>(src1);
        s32 src2s   = static_cast<s32>(src2);
        s32 results = static_cast<s32>(result);
        if (((src1s <  0) && (src2s >= 0) && (results >= 0)) ||  // neg - pos should always be neg
            ((src1s >= 0) && (src2s <  0) && (results <  0))) {  // pos - neg should always be pos
            m_status |= SR_OV;

            // If enabled, set range exception here
            if ((m_status & SR_OVE) && (m_aecr & AE_OVADDE)) {
                m_aesr |= AE_OVADDE;
                exception(EX_RANGE);
            }
        }
    }

    void or1k::execute_orbis32_and(instruction* ci) {
        u32* rA = ci->src1;
        u32* rB = ci->src2;
        u32* rD = ci->dest;

        *rD = *rA & *rB;
    }

    void or1k::execute_orbis32_or(instruction* ci) {
        u32* rA = ci->src1;
        u32* rB = ci->src2;
        u32* rD = ci->dest;

        *rD = *rA | *rB;
    }

    void or1k::execute_orbis32_xor(instruction* ci) {
        u32* rD = ci->dest;
        u32* rA = ci->src1;
        u32* rB = ci->src2;

        *rD = *rA ^ *rB;
    }

    void or1k::execute_orbis32_cmov(instruction* ci) {
        u32* rA = ci->src1;
        u32* rB = ci->src2;
        u32* rD = ci->dest;

        *rD = (m_status & SR_F) ? *rA : *rB;
    }

    void or1k::execute_orbis32_ff1(instruction* ci) {
        u32* rA = ci->src1;
        u32* rD = ci->dest;

        *rD = ffs32(*rA);
    }

    void or1k::execute_orbis32_fl1(instruction* ci) {
        u32* rA = ci->src1;
        u32* rD = ci->dest;

        *rD = fls32(*rA);
    }

    void or1k::execute_orbis32_sll(instruction* ci) {
        u32* rD = ci->dest;
        u32* rA = ci->src1;
        u32* rB = ci->src2;

        *rD = *rA << (*rB & 0x1f);
    }

    void or1k::execute_orbis32_srl(instruction* ci) {
        u32* rD = ci->dest;
        u32* rA = ci->src1;
        u32* rB = ci->src2;

        *rD = *rA >> (*rB & 0x1f);
    }

    void or1k::execute_orbis32_sra(instruction* ci) {
        u32* rD = ci->dest;
        u32* rA = ci->src1;
        u32* rB = ci->src2;

        u32 shift = *rB & 0x1f;
        *rD = sign_extend32(*rA >> shift, 31 - shift);
    }

    void or1k::execute_orbis32_ror(instruction* ci) {
        u32* rA = ci->src1;
        u32* rB = ci->src2;
        u32* rD = ci->dest;

        u32 rotate = *rB & 0x1f;
        *rD = (*rA >> rotate) | (*rA << (31 - rotate));
    }

    void or1k::execute_orbis32_mul(instruction* ci) {
        s64 src1 = static_cast<s64>(static_cast<s32>(*ci->src1));
        s64 src2 = static_cast<s64>(static_cast<s32>(*ci->src2));

        // Perform operation
        s64 result = src1 * src2;
        *ci->dest = static_cast<u32>(static_cast<s32>(result));

        // Clear overflow flag
        m_status &= ~SR_OV;

        if ((result > numeric_limits<s32>::max()) ||
            (result < numeric_limits<s32>::min())) {
            m_status |= SR_OV;
            if ((m_status & SR_OVE) && (m_aecr & AE_OVMULE)) {
                m_aesr |= AE_OVMULE;
                exception(EX_RANGE);
            }
        }
    }

    void or1k::execute_orbis32_mulu(instruction* ci) {
        u64 src1 = *ci->src1;
        u64 src2 = *ci->src2;

        // Perform operation
        u64 result = src1 * src2;
        *ci->dest = static_cast<u32>(result);

        // Clear carry flag
        m_status &= ~SR_CY;

        // Check for unsigned overflow
        if (result > std::numeric_limits<u32>::max()) {
            m_status |= SR_CY;
            if ((m_status & SR_OVE) && (m_aecr & AE_CYMULE)) {
                m_aesr |= AE_CYMULE;
                exception(EX_RANGE);
            }
        }
    }

    void or1k::execute_orbis32_muld(instruction* ci) {
        s64 src1 = static_cast<s64>(static_cast<s32>(*ci->src1));
        s64 src2 = static_cast<s64>(static_cast<s32>(*ci->src2));

        // Perform operation
        s64 result = src1 * src2;
        m_mac.hi = static_cast<u32>(static_cast<s32>(result >> 32));
        m_mac.lo = static_cast<u32>(static_cast<s32>(result >>  0));

        // Clear overflow flag
        m_status &= ~SR_OV;

        // Check for signed overflow
        if ((result > numeric_limits<s32>::max()) ||
            (result < numeric_limits<s32>::min())) {
            m_status |= SR_OV;
            if ((m_status & SR_OVE) && (m_aecr & AE_OVMULE)) {
                m_aesr |= AE_OVMULE;
                exception(EX_RANGE);
            }
        }
    }

    void or1k::execute_orbis32_muldu(instruction* ci) {
        u64 src1 = static_cast<u64>(*ci->src1);
        u64 src2 = static_cast<u64>(*ci->src2);

        // Perform operation
        u64 result = src1 * src2;
        m_mac.hi = static_cast<u32>(result >> 32);
        m_mac.lo = static_cast<u32>(result >>  0);

        // Clear carry flag
        m_status &= ~SR_CY;

        // Check for signed overflow
        if (result > std::numeric_limits<u32>::max()) {
            m_status |= SR_CY;
            if ((m_status & SR_OVE) && (m_aecr & AE_CYMULE)) {
                m_aesr |= AE_CYMULE;
                exception(EX_RANGE);
            }
        }
    }

    void or1k::execute_orbis32_div(instruction* ci) {
        s32 src1 = static_cast<s32>(*ci->src1);
        s32 src2 = static_cast<s32>(*ci->src2);

        // Check for divide by zero
        if (src2 == 0) {
            m_status |= SR_OV;
            if ((m_status & SR_OVE) && (m_aecr & AE_DBZE)) {
                m_aesr |= AE_DBZE;
                exception(EX_RANGE);
            }
            return;
        }

        // Clear flag and perform operation
        m_status &= ~SR_OV;
        *ci->dest = static_cast<u32>(src1 / src2);
    }

    void or1k::execute_orbis32_divu(instruction* ci) {
        u32 src1 = *ci->src1;
        u32 src2 = *ci->src2;

        if (src2 == 0) {
            m_status |= SR_CY;
            if ((m_status & SR_OVE) && (m_aecr & AE_DBZE)) {
                m_aesr |= AE_DBZE;
                exception(EX_RANGE);
            }
            return;
        }

        // Clear flag and perform operation
        m_status &= ~SR_CY;
        *ci->dest = src1 / src2;
    }

    void or1k::execute_orbis32_sfeq(instruction* ci) {
        u32 src1 = *ci->src1;
        u32 src2 = *ci->src2;

        if (src1 == src2)
            m_status |= SR_F;
        else
            m_status &= ~SR_F;
    }

    void or1k::execute_orbis32_sfne(instruction* ci) {
        u32 src1 = *ci->src1;
        u32 src2 = *ci->src2;

        if (src1 != src2)
            m_status |= SR_F;
        else
            m_status &= ~SR_F;
    }

    void or1k::execute_orbis32_sfgtu(instruction* ci) {
        u32 src1 = *ci->src1;
        u32 src2 = *ci->src2;

        if (src1 > src2)
            m_status |= SR_F;
        else
            m_status &= ~SR_F;
    }

    void or1k::execute_orbis32_sfgeu(instruction* ci) {
        u32 src1 = *ci->src1;
        u32 src2 = *ci->src2;

        if (src1 >= src2)
            m_status |= SR_F;
        else
            m_status &= ~SR_F;
    }

    void or1k::execute_orbis32_sfltu(instruction* ci) {
        u32 src1 = *ci->src1;
        u32 src2 = *ci->src2;

        if (src1 < src2)
            m_status |= SR_F;
        else
            m_status &= ~SR_F;
    }

    void or1k::execute_orbis32_sfleu(instruction* ci) {
        u32 src1 = *ci->src1;
        u32 src2 = *ci->src2;

        if (src1 <= src2)
            m_status |= SR_F;
        else
            m_status &= ~SR_F;
    }

    void or1k::execute_orbis32_sfgts(instruction* ci) {
        s32 src1 = static_cast<s32>(*ci->src1);
        s32 src2 = static_cast<s32>(*ci->src2);

        if (src1 > src2)
            m_status |= SR_F;
        else
            m_status &= ~SR_F;
    }

    void or1k::execute_orbis32_sfges(instruction* ci) {
        s32 src1 = static_cast<s32>(*ci->src1);
        s32 src2 = static_cast<s32>(*ci->src2);

        if (src1 >= src2)
            m_status |= SR_F;
        else
            m_status &= ~SR_F;
    }

    void or1k::execute_orbis32_sflts(instruction* ci) {
        s32 src1 = static_cast<s32>(*ci->src1);
        s32 src2 = static_cast<s32>(*ci->src2);

        if (src1 < src2)
            m_status |= SR_F;
        else
            m_status &= ~SR_F;
    }

    void or1k::execute_orbis32_sfles(instruction* ci) {
        s32 src1 = static_cast<s32>(*ci->src1);
        s32 src2 = static_cast<s32>(*ci->src2);

        if (src1 <= src2)
            m_status |= SR_F;
        else
            m_status &= ~SR_F;
    }

    void or1k::execute_orbis32_mac(instruction* ci) {
        s64 src1 = static_cast<s64>(static_cast<s32>(*ci->src1));
        s64 src2 = static_cast<s64>(static_cast<s32>(*ci->src2));

        s64 result = static_cast<s64>(m_mac.hi) << 32 |
                         static_cast<s64>(m_mac.lo);
        result += src1 * src2;

        m_mac.hi = static_cast<u32>(static_cast<u64>(result >> 32));
        m_mac.lo = static_cast<u32>(static_cast<u64>(result >>  0));

        m_status &= ~SR_OV;
        if ((result > std::numeric_limits<s32>::max()) ||
            (result < std::numeric_limits<s32>::min())) {
            m_status |= SR_OV;
            if ((m_status & SR_OVE) && (m_aecr & AE_OVMACADDE)) {
                m_aesr |= AE_OVMACADDE;
                exception(EX_RANGE);
            }
        }
    }

    void or1k::execute_orbis32_macu(instruction* ci) {
        u64 src1 = static_cast<u64>(*ci->src1);
        u64 src2 = static_cast<u64>(*ci->src2);

        u64 result = static_cast<u64>(m_mac.hi) << 32 |
                          static_cast<u64>(m_mac.lo);
        result += src1 * src2;

        m_mac.hi = static_cast<u32>(result >> 32);
        m_mac.lo = static_cast<u32>(result);

        m_status &= ~SR_CY;
        if (result > std::numeric_limits<u32>::max()) {
            m_status |= SR_CY;
            if ((m_status & SR_OVE) && (m_aecr & AE_CYMACADDE)) {
                m_aesr |= AE_CYMACADDE;
                exception(EX_RANGE);
            }
        }
    }

    void or1k::execute_orbis32_msb(instruction* ci) {
        s64 src1 = static_cast<s64>(static_cast<s32>(*ci->src1));
        s64 src2 = static_cast<s64>(static_cast<s32>(*ci->src2));

        s64 result = static_cast<s64>(m_mac.hi) << 32 |
                         static_cast<s64>(m_mac.lo);
        result -= src1 * src2;

        m_mac.hi = static_cast<u32>(static_cast<u64>(result >> 32));
        m_mac.lo = static_cast<u32>(static_cast<u64>(result >>  0));

        m_status &= ~SR_OV;
        if ((result > std::numeric_limits<s32>::max()) ||
            (result < std::numeric_limits<s32>::min())) {
            m_status |= SR_OV;
            if ((m_status & SR_OVE) && (m_aecr & AE_OVMACADDE)) {
                m_aesr |= AE_OVMACADDE;
                exception(EX_RANGE);
            }
        }
    }

    void or1k::execute_orbis32_msbu(instruction* ci) {
        u64 src1 = static_cast<u64>(*ci->src1);
        u64 src2 = static_cast<u64>(*ci->src2);

        u64 result = static_cast<u64>(m_mac.hi) << 32 |
                          static_cast<u64>(m_mac.lo);
        result -= src1 * src2;

        m_mac.hi = static_cast<u32>(result >> 32);
        m_mac.lo = static_cast<u32>(result);

        m_status &= ~SR_CY;
        if (result > std::numeric_limits<u32>::max()) {
            m_status |= SR_CY;
            if ((m_status & SR_OVE) && (m_aecr & AE_CYMACADDE)) {
                m_aesr |= AE_CYMACADDE;
                exception(EX_RANGE);
            }
        }
    }

    void or1k::execute_orbis32_macrc(instruction* ci) {
        *ci->dest = m_mac.lo;
        m_mac.lo = 0;
        m_mac.hi = 0;
    }

    void or1k::execute_orbis32_sys(instruction* ci) {
        // Syscall id is stored in ci->imm, but not used
        exception(EX_SYSCALL);
    }

    void or1k::execute_orbis32_trap(instruction* ci) {
        // Trap id is stored in ci->imm, but not used
        exception(EX_TRAP);
    }

    void or1k::execute_orbis32_csync(instruction* ci) {
        // Nothing to do
    }

    void or1k::execute_orbis32_msync(instruction* ci) {
        // Nothing to do
    }

    void or1k::execute_orbis32_psync(instruction* ci) {
        // Nothing to do
    }

    void or1k::execute_orbis32_rfe(instruction* ci) {
        // Leave exception handler
        u32 target = get_spr(SPR_EPCR);
        schedule_jump(target, 0);

        // Restore status register
        u32 status = get_spr(SPR_ESR);
        set_spr(SPR_SR, status);
    }

    void or1k::execute_orfpx32_add(instruction* ci) {
        float* src1 = reinterpret_cast<float*>(ci->src1);
        float* src2 = reinterpret_cast<float*>(ci->src2);
        float* dest = reinterpret_cast<float*>(ci->dest);
        setup_fp_round_mode();
        *dest = *src1 + *src2;
        reset_fp_round_mode();
        reset_fp_flags(*dest);
    }

    void or1k::execute_orfpx32_sub(instruction* ci) {
        float* src1 = reinterpret_cast<float*>(ci->src1);
        float* src2 = reinterpret_cast<float*>(ci->src2);
        float* dest = reinterpret_cast<float*>(ci->dest);

        setup_fp_round_mode();
        *dest = *src1 - *src2;
        reset_fp_round_mode();
        reset_fp_flags(*dest);
    }

    void or1k::execute_orfpx32_mul(instruction* ci) {
        float* src1 = reinterpret_cast<float*>(ci->src1);
        float* src2 = reinterpret_cast<float*>(ci->src2);
        float* dest = reinterpret_cast<float*>(ci->dest);

        setup_fp_round_mode();
        *dest = *src1 * *src2;
        reset_fp_round_mode();
        reset_fp_flags(*dest);
    }

    void or1k::execute_orfpx32_div(instruction* ci) {
        float* src1 = reinterpret_cast<float*>(ci->src1);
        float* src2 = reinterpret_cast<float*>(ci->src2);
        float* dest = reinterpret_cast<float*>(ci->dest);

        setup_fp_round_mode();
        *dest = *src1 / *src2;
        reset_fp_round_mode();
        reset_fp_flags(*dest);
    }

    void or1k::execute_orfpx32_rem(instruction* ci) {
        float* src1 = reinterpret_cast<float*>(ci->src1);
        float* src2 = reinterpret_cast<float*>(ci->src2);
        float* dest = reinterpret_cast<float*>(ci->dest);

        setup_fp_round_mode();
        *dest = std::fmod(*src1, *src2);
        reset_fp_round_mode();
        reset_fp_flags(*dest);
    }

    void or1k::execute_orfpx32_madd(instruction* ci) {
        float* src1 = reinterpret_cast<float*>(ci->src1);
        float* src2 = reinterpret_cast<float*>(ci->src2);
        float* dest = reinterpret_cast<float*>(&m_fmac.lo);

        setup_fp_round_mode();
        *dest += *src1 + *src2;
        reset_fp_round_mode();
        reset_fp_flags(*dest);
    }

    void or1k::execute_orfpx32_itof(instruction* ci) {
        u32* rA = ci->src1;
        float*    rD = reinterpret_cast<float*>(ci->dest);

        *rD = *rA;
    }

    void or1k::execute_orfpx32_ftoi(instruction* ci) {
        float*    rA = reinterpret_cast<float*>(ci->src1);
        u32* rD = ci->dest;

        *rD = *rA;
    }

    void or1k::execute_orfpx32_sfeq(instruction* ci) {
        float* rA = reinterpret_cast<float*>(ci->src1);
        float* rB = reinterpret_cast<float*>(ci->src2);

        m_status &= ~SR_F;
        if (*rA == *rB)
            m_status |= SR_F;
    }

    void or1k::execute_orfpx32_sfne(instruction* ci) {
        float* rA = reinterpret_cast<float*>(ci->src1);
        float* rB = reinterpret_cast<float*>(ci->src2);

        m_status &= ~SR_F;
        if (*rA != *rB)
            m_status |= SR_F;
    }

    void or1k::execute_orfpx32_sfgt(instruction* ci) {
        float* rA = reinterpret_cast<float*>(ci->src1);
        float* rB = reinterpret_cast<float*>(ci->src2);

        m_status &= ~SR_F;
        if (*rA > *rB)
            m_status |= SR_F;
    }

    void or1k::execute_orfpx32_sfge(instruction* ci) {
        float* rA = reinterpret_cast<float*>(ci->src1);
        float* rB = reinterpret_cast<float*>(ci->src2);

        m_status &= ~SR_F;
        if (*rA >= *rB)
            m_status |= SR_F;
    }

    void or1k::execute_orfpx32_sflt(instruction* ci) {
        float* rA = reinterpret_cast<float*>(ci->src1);
        float* rB = reinterpret_cast<float*>(ci->src2);

        m_status &= ~SR_F;
        if (*rA < *rB)
            m_status |= SR_F;
    }

    void or1k::execute_orfpx32_sfle(instruction* ci) {
        float* rA = reinterpret_cast<float*>(ci->src1);
        float* rB = reinterpret_cast<float*>(ci->src2);

        m_status &= ~SR_F;
        if (*rA <= *rB)
            m_status |= SR_F;
    }

    void or1k::execute_orfpx64_add(instruction* ci) {
        double_register src1, src2, dest;
        src1.hi = *(ci->src1 + 1);
        src1.lo = *(ci->src1 + 0);
        src2.hi = *(ci->src2 + 1);
        src2.lo = *(ci->src2 + 0);

        setup_fp_round_mode();
        dest.d = src1.d + src2.d;

        *(ci->dest + 0) = dest.lo;
        *(ci->dest + 1) = dest.hi;

        reset_fp_round_mode();
        reset_fp_flags(dest.d);
    }

    void or1k::execute_orfpx64_sub(instruction* ci) {
        double_register src1, src2, dest;
        src1.hi = *(ci->src1 + 1);
        src1.lo = *(ci->src1 + 0);
        src2.hi = *(ci->src2 + 1);
        src2.lo = *(ci->src2 + 0);

        setup_fp_round_mode();
        dest.d = src1.d - src2.d;

        *(ci->dest + 0) = dest.lo;
        *(ci->dest + 1) = dest.hi;

        reset_fp_round_mode();
        reset_fp_flags(dest.d);
    }

    void or1k::execute_orfpx64_mul(instruction* ci) {
        double_register src1, src2, dest;
        src1.hi = *(ci->src1 + 1);
        src1.lo = *(ci->src1 + 0);
        src2.hi = *(ci->src2 + 1);
        src2.lo = *(ci->src2 + 0);

        setup_fp_round_mode();
        dest.d = src1.d * src2.d;

        *(ci->dest + 0) = dest.lo;
        *(ci->dest + 1) = dest.hi;

        reset_fp_round_mode();
        reset_fp_flags(dest.d);
    }

    void or1k::execute_orfpx64_div(instruction* ci) {
        double_register src1, src2, dest;
        src1.hi = *(ci->src1 + 1);
        src1.lo = *(ci->src1 + 0);
        src2.hi = *(ci->src2 + 1);
        src2.lo = *(ci->src2 + 0);

        setup_fp_round_mode();
        dest.d = src1.d / src2.d;

        *(ci->dest + 0) = dest.lo;
        *(ci->dest + 1) = dest.hi;

        reset_fp_round_mode();
        reset_fp_flags(dest.d);
    }

    void or1k::execute_orfpx64_rem(instruction* ci) {
        double_register src1, src2, dest;
        src1.hi = *(ci->src1 + 1);
        src1.lo = *(ci->src1 + 0);
        src2.hi = *(ci->src2 + 1);
        src2.lo = *(ci->src2 + 0);

        setup_fp_round_mode();
        dest.d = std::fmod(src1.d, src2.d);

        *(ci->dest + 0) = dest.lo;
        *(ci->dest + 1) = dest.hi;

        reset_fp_round_mode();
        reset_fp_flags(dest.d);
    }

    void or1k::execute_orfpx64_madd(instruction* ci) {
        double_register src1, src2;
        src1.hi = *(ci->src1 + 1);
        src1.lo = *(ci->src1 + 0);
        src2.hi = *(ci->src2 + 1);
        src2.lo = *(ci->src2 + 0);

        setup_fp_round_mode();
        m_fmac.d = src1.d - src2.d;

        reset_fp_round_mode();
        reset_fp_flags(m_fmac.d);
    }

    void or1k::execute_orfpx64_itof(instruction* ci) {
        double_register src1, dest;
        src1.hi = *(ci->src1 + 1);
        src1.lo = *(ci->src1 + 0);

        dest.d = src1.i;

        *(ci->dest + 1) = dest.hi;
        *(ci->dest + 0) = dest.lo;
    }

    void or1k::execute_orfpx64_ftoi(instruction* ci) {
        double_register src1, dest;
        src1.hi = *(ci->src1 + 1);
        src1.lo = *(ci->src1 + 0);

        dest.i = src1.d;

        *(ci->dest + 1) = dest.hi;
        *(ci->dest + 0) = dest.lo;
    }

    void or1k::execute_orfpx64_sfeq(instruction* ci) {
        double_register src1, src2;
        src1.hi = *(ci->src1 + 1);
        src1.lo = *(ci->src1 + 0);
        src2.hi = *(ci->src2 + 1);
        src2.lo = *(ci->src2 + 0);

        m_status &= ~SR_F;
        if (src1.d == src2.d)
            m_status |= SR_F;
    }

    void or1k::execute_orfpx64_sfne(instruction* ci) {
        double_register src1, src2;
        src1.hi = *(ci->src1 + 1);
        src1.lo = *(ci->src1 + 0);
        src2.hi = *(ci->src2 + 1);
        src2.lo = *(ci->src2 + 0);

        m_status &= ~SR_F;
        if (src1.d != src2.d)
            m_status |= SR_F;
    }

    void or1k::execute_orfpx64_sfgt(instruction* ci) {
        double_register src1, src2;
        src1.hi = *(ci->src1 + 1);
        src1.lo = *(ci->src1 + 0);
        src2.hi = *(ci->src2 + 1);
        src2.lo = *(ci->src2 + 0);

        m_status &= ~SR_F;
        if (src1.d > src2.d)
            m_status |= SR_F;
    }

    void or1k::execute_orfpx64_sfge(instruction* ci) {
        double_register src1, src2;
        src1.hi = *(ci->src1 + 1);
        src1.lo = *(ci->src1 + 0);
        src2.hi = *(ci->src2 + 1);
        src2.lo = *(ci->src2 + 0);

        m_status &= ~SR_F;
        if (src1.d >= src2.d)
            m_status |= SR_F;
    }

    void or1k::execute_orfpx64_sflt(instruction* ci) {
        double_register src1, src2;
        src1.hi = *(ci->src1 + 1);
        src1.lo = *(ci->src1 + 0);
        src2.hi = *(ci->src2 + 1);
        src2.lo = *(ci->src2 + 0);

        m_status &= ~SR_F;
        if (src1.d < src2.d)
            m_status |= SR_F;
    }

    void or1k::execute_orfpx64_sfle(instruction* ci) {
        double_register src1, src2;
        src1.hi = *(ci->src1 + 1);
        src1.lo = *(ci->src1 + 0);
        src2.hi = *(ci->src2 + 1);
        src2.lo = *(ci->src2 + 0);

        m_status &= ~SR_F;
        if (src1.d <= src2.d)
            m_status |= SR_F;
    }

}
