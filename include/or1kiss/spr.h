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

#ifndef OR1KISS_SPR_H
#define OR1KISS_SPR_H

#define OR1KISS_SHADOW_REGS 512

namespace or1kiss {

enum sp_groups {
    SPG_SYS  = 0 << 11,  /* System Control and Status */
    SPG_DMMU = 1 << 11,  /* Data Memory Management Unit */
    SPG_IMMU = 2 << 11,  /* Instruction Memory Management Unit */
    SPG_DC   = 3 << 11,  /* Data Cache */
    SPG_IC   = 4 << 11,  /* Instruction Cache */
    SPG_MAC  = 5 << 11,  /* Multiply-Accumulate Unit */
    SPG_DBG  = 6 << 11,  /* Debug Unit */
    SPG_PC   = 7 << 11,  /* Performance Counters Unit */
    SPG_PM   = 8 << 11,  /* Power Management Unit */
    SPG_PIC  = 9 << 11,  /* Programmable Interrupt Controller */
    SPG_TT   = 10 << 11, /* Tick Timer */
    SPG_FPU  = 11 << 11  /* Floating Point Unit */
};

enum sp_registers {
    /* System status and control */
    SPR_VR       = SPG_SYS + 0,    /* Version register */
    SPR_UPR      = SPG_SYS + 1,    /* Unit Present register */
    SPR_CPUCFGR  = SPG_SYS + 2,    /* CPU Configuration register */
    SPR_DMMUCFGR = SPG_SYS + 3,    /* Data MMU Configuration register */
    SPR_IMMUCFGR = SPG_SYS + 4,    /* Instruction MMU Configuration */
    SPR_DCCFGR   = SPG_SYS + 5,    /* Data Cache Configuration register */
    SPR_ICCFGR   = SPG_SYS + 6,    /* Instruction Cache Configuration */
    SPR_DCFGR    = SPG_SYS + 7,    /* Debug Configuration register */
    SPR_PCCFGR   = SPG_SYS + 8,    /* Performance Counter Configuration */
    SPR_VR2      = SPG_SYS + 9,    /* Version register 2 */
    SPR_AVR      = SPG_SYS + 10,   /* Architecture version register */
    SPR_EVBAR    = SPG_SYS + 11,   /* Exception vector base address */
    SPR_AECR     = SPG_SYS + 12,   /* Arithmetic Exception Control */
    SPR_AESR     = SPG_SYS + 13,   /* Arithmetic Exception Status */
    SPR_NPC      = SPG_SYS + 16,   /* Next Program Counter register */
    SPR_SR       = SPG_SYS + 17,   /* Supervisor register */
    SPR_PPC      = SPG_SYS + 18,   /* Previous Program Counter register */
    SPR_FPCSR    = SPG_SYS + 20,   /* FP Control Status register */
    SPR_ISR      = SPG_SYS + 21,   /* Implementation specific registers */
    SPR_EPCR     = SPG_SYS + 32,   /* Exception PC registers */
    SPR_EEAR     = SPG_SYS + 48,   /* Exception EA registers */
    SPR_ESR      = SPG_SYS + 64,   /* Exception SR registers */
    SPR_COREID   = SPG_SYS + 128,  /* Core ID register */
    SPR_NUMCORES = SPG_SYS + 129,  /* Number of Cores register */
    SPR_GPR      = SPG_SYS + 1024, /* GPRs mapped into SPR space */

    /* Data MMU */
    SPR_DMMUCR   = SPG_DMMU + 0,    /* DMMU Control register */
    SPR_DMMUPR   = SPG_DMMU + 1,    /* DMMU Protection register */
    SPR_DTLBEIR  = SPG_DMMU + 2,    /* DMMU TLB Entry Invalidate reg */
    SPR_DATBMR   = SPG_DMMU + 4,    /* DATB Match registers */
    SPR_DATBTR   = SPG_DMMU + 8,    /* DATB Translate registers */
    SPR_DTLBW0MR = SPG_DMMU + 512,  /* DTLB Match registers Way 0 */
    SPR_DTLBW0TR = SPG_DMMU + 640,  /* DTLB Translate registers Way 0 */
    SPR_DTLBW1MR = SPG_DMMU + 768,  /* DTLB Match registers Way 1 */
    SPR_DTLBW1TR = SPG_DMMU + 896,  /* DTLB Translate registers Way 1 */
    SPR_DTLBW2MR = SPG_DMMU + 1024, /* DTLB Match registers Way 2 */
    SPR_DTLBW2TR = SPG_DMMU + 1152, /* DTLB Translate registers Way 2 */
    SPR_DTLBW3MR = SPG_DMMU + 1280, /* DTLB Match registers Way 3 */
    SPR_DTLBW3TR = SPG_DMMU + 1408, /* DTLB Translate registers Way 3 */

    /* Instruction MMU */
    SPR_IMMUCR   = SPG_IMMU + 0,    /* IMMU Control register */
    SPR_IMMUPR   = SPG_IMMU + 1,    /* IMMU Protection register */
    SPR_ITLBEIR  = SPG_IMMU + 2,    /* IMMU TLB Entry Invalidate reg */
    SPR_IATBMR   = SPG_IMMU + 4,    /* IATB Match registers */
    SPR_IATBTR   = SPG_IMMU + 8,    /* IATB Translate registers */
    SPR_ITLBW0MR = SPG_IMMU + 512,  /* ITLB Match registers Way 0 */
    SPR_ITLBW0TR = SPG_IMMU + 640,  /* ITLB Translate registers Way 0 */
    SPR_ITLBW1MR = SPG_IMMU + 768,  /* ITLB Match registers Way 1 */
    SPR_ITLBW1TR = SPG_IMMU + 896,  /* ITLB Translate registers Way 1 */
    SPR_ITLBW2MR = SPG_IMMU + 1024, /* ITLB Match registers Way 2 */
    SPR_ITLBW2TR = SPG_IMMU + 1152, /* ITLB Translate registers Way 2 */
    SPR_ITLBW3MR = SPG_IMMU + 1280, /* ITLB Match registers Way 3 */
    SPR_ITLBW3TR = SPG_IMMU + 1408, /* ITLB Translate registers Way 3 */

    /* Data Cache */
    SPR_DCCR  = SPG_DC + 0, /* DC Control register */
    SPR_DCBPR = SPG_DC + 1, /* DC Block Prefetch register */
    SPR_DCBFR = SPG_DC + 2, /* DC Block Flush register */
    SPR_DCBIR = SPG_DC + 3, /* DC Block Invalidate register */
    SPR_DCBWR = SPG_DC + 4, /* DC Block Write-back register */
    SPR_DCBLR = SPG_DC + 5, /* DC Block Lock register */

    /* Instruction Cache */
    SPR_ICCR  = SPG_IC + 0, /* IC Control register */
    SPR_ICBPR = SPG_IC + 1, /* IC Block Prefetch register */
    SPR_ICBIR = SPG_IC + 2, /* IC Block Invalidate register */
    SPR_ICBLR = SPG_IC + 3, /* IC Block Lock register */

    /* MAC unit */
    SPR_MACLO = SPG_MAC + 1, /* MAC Low */
    SPR_MACHI = SPG_MAC + 2, /* MAC High */

    /* Power Management */
    SPR_PMR = SPG_PM + 0, /* Power Management register */

    /* Programmable Interrupt Controller */
    SPR_PICMR = SPG_PIC + 0, /* PIC Mode register */
    SPR_PICSR = SPG_PIC + 2, /* PIC Status register */

    /* Tick Timer */
    SPR_TTMR = SPG_TT + 0, /* Tick Timer Mode register */
    SPR_TTCR = SPG_TT + 1  /* Tick Timer Count register */
};

inline unsigned int spr_group(int spr) {
    return (unsigned int)spr >> 11;
}

inline unsigned int spr_regno(int spr) {
    return (unsigned int)spr & 0x7ff;
}

enum spr_access {
    SPR_SRE = 1 << 0,
    SPR_SWE = 1 << 1,
    SPR_URE = 1 << 2,
    SPR_UWE = 1 << 3,
};

int spr_get_access(int spr);

inline bool spr_check_access(int spr, bool is_write, bool is_super) {
    int a = spr_get_access(spr);
    if (is_super)
        return a & (is_write ? SPR_SWE : SPR_SRE);
    else
        return a & (is_write ? SPR_UWE : SPR_URE);
}

inline bool spr_super_read(int spr) {
    return spr_get_access(spr) & SPR_SRE;
}

inline bool spr_super_write(int spr) {
    return spr_get_access(spr) & SPR_SWE;
}

inline bool spr_user_read(int spr) {
    return spr_get_access(spr) & SPR_URE;
}

inline bool spr_user_write(int spr) {
    return spr_get_access(spr) & SPR_UWE;
}

} // namespace or1kiss

#endif
