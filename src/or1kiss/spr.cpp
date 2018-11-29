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

#include "or1kiss/spr.h"
#include "or1kiss/or1k.h"

namespace or1kiss {

    int spr_get_access(int spr) {
        switch (spr) {
        /* System group */
        case SPR_VR       : return SPR_SRE;
        case SPR_UPR      : return SPR_SRE;
        case SPR_CPUCFGR  : return SPR_SRE;
        case SPR_DMMUCFGR : return SPR_SRE;
        case SPR_IMMUCFGR : return SPR_SRE;
        case SPR_DCCFGR   : return SPR_SRE;
        case SPR_ICCFGR   : return SPR_SRE;
        case SPR_DCFGR    : return SPR_SRE;
        case SPR_PCCFGR   : return SPR_SRE;
        case SPR_VR2      : return SPR_SRE;
        case SPR_AVR      : return SPR_SRE;
        case SPR_EVBAR    : return SPR_SRE | SPR_SWE;
        case SPR_AECR     : return SPR_SRE | SPR_SWE;
        case SPR_AESR     : return SPR_SRE | SPR_SWE;
        case SPR_NPC      : return SPR_SRE | SPR_SWE;
        case SPR_SR       : return SPR_SRE | SPR_SWE;
        case SPR_PPC      : return SPR_SRE | SPR_SWE;
        case SPR_FPCSR    : return SPR_SRE | SPR_SWE;
        case SPR_EPCR     : return SPR_SRE | SPR_SWE;
        case SPR_EEAR     : return SPR_SRE | SPR_SWE;
        case SPR_ESR      : return SPR_SRE | SPR_SWE;
        case SPR_COREID   : return SPR_SRE;
        case SPR_NUMCORES : return SPR_SRE;

        /* DMMU group */
        case SPR_DMMUCR   : return SPR_SRE | SPR_SWE;
        case SPR_DMMUPR   : return SPR_SRE | SPR_SWE;
        case SPR_DTLBEIR  : return SPR_SWE;

        /* IMMU group */
        case SPR_IMMUCR   : return SPR_SRE | SPR_SWE;
        case SPR_IMMUPR   : return SPR_SRE | SPR_SWE;
        case SPR_ITLBEIR  : return SPR_SWE;

        /* Data Cache group*/
        case SPR_DCCR     : return SPR_SRE | SPR_SWE;
        case SPR_DCBPR    : return SPR_SWE | SPR_UWE;
        case SPR_DCBFR    : return SPR_SWE | SPR_UWE;
        case SPR_DCIR     : return SPR_SWE;
        case SPR_DCBWR    : return SPR_SWE | SPR_UWE;
        case SPR_DCBLR    : return SPR_SWE | SPR_UWE;

        /* Instruction Cache group */
        case SPR_ICCR     : return SPR_SRE | SPR_SWE;
        case SPR_ICBPR    : return SPR_SWE | SPR_UWE;
        case SPR_ICBIR    : return SPR_SWE;
        case SPR_ICBLR    : return SPR_SWE | SPR_UWE;

        /* MAC group */
        case SPR_MACHI    : return SPR_SRE | SPR_SWE;
        case SPR_MACLO    : return SPR_SRE | SPR_SWE;

        /* Power Management group */
        case SPR_PMR      : return SPR_SRE | SPR_SWE;

        /* PIC group */
        case SPR_PICMR    : return SPR_SRE | SPR_SWE;
        case SPR_PICSR    : return SPR_SRE | SPR_SWE;

        /* Tick Timer group */
        case SPR_TTMR     : return SPR_SRE | SPR_SWE;
        case SPR_TTCR     : return SPR_SRE | SPR_SWE;

        default:
            break;
        }

        /* GPR shadow registers */
        if ((spr >= SPR_GPR) && (spr < (SPR_GPR + OR1KISS_SHADOW_REGS)))
            return SPR_SRE | SPR_SWE;

        /* Data MMU ATB and TLB register sets */
        if ((spr >= SPR_DATBMR) && (spr < (SPR_DATBTR + 4)))
            return SPR_SRE | SPR_SWE;
        if ((spr >= SPR_DTLBW0MR) && (spr < (SPR_DTLBW3TR + 128)))
            return SPR_SRE | SPR_SWE;

        /* Instruction MMU ATB and TLB register sets */
        if ((spr >= SPR_IATBMR) && (spr < (SPR_IATBTR + 4)))
            return SPR_SRE | SPR_SWE;
        if ((spr >= SPR_ITLBW0MR) && (spr < (SPR_ITLBW3TR + 128)))
            return SPR_SRE | SPR_SWE;

        /* Grant all access for unchecked register */
        return SPR_SRE | SPR_SWE | SPR_URE | SPR_UWE;
    }

    u32 or1k::get_spr(u32 reg, bool debug) const {
        bool is_super = is_supervisor() || (m_status & SR_SUMRA);
        if (warn(!debug && !spr_check_access(reg, false, is_super),
                 "illegal attempt to read SPR %d", reg)) {
            return 0;
        }

        switch (reg) {
        /* System group */
        case SPR_VR       : return m_version;
        case SPR_VR2      : return m_version2;
        case SPR_AVR      : return m_avr;
        case SPR_UPR      : return m_unit;
        case SPR_CPUCFGR  : return m_cpucfg;
        case SPR_DCCFGR   : return m_dccfgr;
        case SPR_ICCFGR   : return m_iccfgr;
        case SPR_DMMUCFGR : return m_dmmu.get_cfgr();
        case SPR_IMMUCFGR : return m_immu.get_cfgr();
        case SPR_AECR     : return m_aecr;
        case SPR_AESR     : return m_aesr;
        case SPR_SR       : return m_status;
        case SPR_NPC      : return m_next_pc;
        case SPR_PPC      : return m_prev_pc;
        case SPR_FPCSR    : return m_fpcfg;
        case SPR_EPCR     : return m_expc;
        case SPR_EEAR     : return m_exea;
        case SPR_ESR      : return m_exsr;
        case SPR_EVBAR    : return m_evba;
        case SPR_COREID   : return m_core_id;
        case SPR_NUMCORES : return m_num_cores;

        /* DMMU group */
        case SPR_DMMUCR   : return m_dmmu.get_cr();
        case SPR_DMMUPR   : return m_dmmu.get_pr();
        case SPR_DTLBEIR  : warn("attempt to read register DTLBEIR"); return 0;

        /* IMMU group */
        case SPR_IMMUCR   : return m_immu.get_cr();
        case SPR_IMMUPR   : return m_immu.get_pr();
        case SPR_ITLBEIR  : warn("attempt to read register ITLBEIR"); return 0;

        /* Data Cache group */
        case SPR_DCBPR    : return 0;
        case SPR_DCBFR    : return 0;

        /* Instruction Cache group */
        case SPR_ICBPR    : return 0;
        case SPR_ICBIR    : return 0;

        /* MAC group */
        case SPR_MACHI    : return m_mac.hi;
        case SPR_MACLO    : return m_mac.lo;

        /* Power Management group */
        case SPR_PMR      : return m_pmr;

        /* PIC group */
        case SPR_PICMR    : return m_pic_mr;
        case SPR_PICSR    : return m_pic_sr;

        /* Tick Timer group */
        case SPR_TTMR     : return m_tick.get_ttmr();
        case SPR_TTCR     : return m_tick.get_ttcr();

        default:
            break;
        }

        /* GPR shadow registers */
        if ((reg >= SPR_GPR) && (reg < (SPR_GPR + OR1KISS_SHADOW_REGS)))
            return m_shadow[reg - SPR_GPR];

        /* Data MMU ATB and TLB register sets */
        if ((reg >= SPR_DATBMR) && (reg < (SPR_DATBTR + 4)))
            return m_dmmu.get_atb(reg - SPR_DATBMR);
        if ((reg >= SPR_DTLBW0MR) && (reg < (SPR_DTLBW3TR + 128)))
            return m_dmmu.get_tlb(reg - SPR_DTLBW0MR);

        /* Instruction MMU ATB and TLB register sets */
        if ((reg >= SPR_IATBMR) && (reg < (SPR_IATBTR + 4)))
            return m_immu.get_atb(reg - SPR_IATBMR);
        if ((reg >= SPR_ITLBW0MR) && (reg < (SPR_ITLBW3TR + 128)))
            return m_immu.get_tlb(reg - SPR_ITLBW0MR);

        /* Show warning that we ignored the command */
        warn("(or1k %d) ignoring SPR read (g%d:r%d) @ 0x%08x\n",
             m_core_id, spr_group(reg), spr_regno(reg), m_next_pc);

        return 0;
    }

    void or1k::set_spr(u32 reg, u32 val, bool debug) {
        m_break_requested = true;

        if (warn(!debug && !spr_check_access(reg, true, is_supervisor()),
                 "illegal attempt to write to SPR %d", reg)) {
            return;
        }

        switch (reg) {
        /* System group */
        case SPR_VR       : warn("attempt to write to VR"); return;
        case SPR_VR2      : warn("attempt to write to VR2"); return;
        case SPR_AVR      : warn("attempt to write to AVR"); return;
        case SPR_UPR      : warn("attempt to write to UPR"); return;
        case SPR_CPUCFGR  : warn("attempt to write to CPUCFGR"); return;
        case SPR_DCCFGR   : warn("attempt to write to DCCFGR"); return;
        case SPR_ICCFGR   : warn("attempt to write to ICCFGR"); return;
        case SPR_DMMUCFGR : warn("attempt to write to DMMUCFGR"); return;
        case SPR_IMMUCFGR : warn("attempt to write to IMMUCFGR"); return;
        case SPR_NPC      : m_next_pc = val; return;
        case SPR_PPC      : m_prev_pc = val; return;
        case SPR_FPCSR    : m_fpcfg = val; return;
        case SPR_EPCR     : m_expc = val; return;
        case SPR_EEAR     : m_exea = val; return;
        case SPR_ESR      : m_exsr = val; return;
        case SPR_EVBAR    : m_evba = val; return;
        case SPR_AECR     : m_aecr = val; return;
        case SPR_AESR     : m_aesr = val; return;
        case SPR_COREID   : warn("attempt to write to COREID"); return;
        case SPR_NUMCORES : warn("attempt to write to NUMCORES"); return;
        case SPR_SR       : m_status = val; return;

        /* DMMU group */
        case SPR_DMMUCR   : m_dmmu.set_cr(val); return;
        case SPR_DTLBEIR  : m_dmmu.flush_tlb_entry(val); return;

        /* IMMU group */
        case SPR_IMMUCR   : m_immu.set_cr(val); return;
        case SPR_ITLBEIR  : m_immu.flush_tlb_entry(val); return;

        /* Data Cache group */
        case SPR_DCBPR    : return;
        case SPR_DCBFR    : return;

        /* Instruction Cache group */
        case SPR_ICBPR    : return;
        case SPR_ICBIR    : m_decode_cache.invalidate_block(val, 32); return;

        /* MAC group */
        case SPR_MACHI    : m_mac.hi = val; return;
        case SPR_MACLO    : m_mac.lo = val; return;

        /* Power Management group */
        case SPR_PMR      : m_pmr = val; doze(); return;

        /* PIC group */
        case SPR_PICMR    : m_pic_mr = val | OR1KISS_PIC_NMI; return;
        case SPR_PICSR    : m_pic_sr = m_pic_level ? val : m_pic_sr & ~val;
                            return;

        /* Tick Timer group */
        case SPR_TTMR     : m_tick.set_ttmr(val); return;
        case SPR_TTCR     : m_tick.set_ttcr(val); return;

        default:
            break;
        }

        /* GPR shadow registers */
        if ((reg >= SPR_GPR) && (reg < (SPR_GPR + OR1KISS_SHADOW_REGS))) {
            m_shadow[reg - SPR_GPR] = val;
            return;
        }

        /* Data MMU ATB and TLB register sets */
        if ((reg >= SPR_DATBMR) && (reg < (SPR_DATBTR + 4)))
            return m_dmmu.set_atb(reg - SPR_DATBMR, val);
        if ((reg >= SPR_DTLBW0MR) && (reg < (SPR_DTLBW3TR + 128)))
            return m_dmmu.set_tlb(reg - SPR_DTLBW0MR, val);

        /* Instruction MMU ATB and TLB register sets */
        if ((reg >= SPR_IATBMR) && (reg < (SPR_IATBTR + 4)))
            return m_immu.set_atb(reg - SPR_IATBMR, val);
        if ((reg >= SPR_ITLBW0MR) && (reg < (SPR_ITLBW3TR + 128)))
            return m_immu.set_tlb(reg - SPR_ITLBW0MR, val);

        /* Show warning that we ignored the command */
        warn("(or1k %d) ignoring SPR write g%d:r%d = 0x%08x @ 0x%08x\n",
             m_core_id, spr_group(reg), spr_regno(reg), val, m_next_pc);
    }

}
