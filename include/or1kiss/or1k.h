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

#ifndef OR1KISS_OR1K_ISS_H
#define OR1KISS_OR1K_ISS_H

#include "or1kiss/includes.h"
#include "or1kiss/types.h"
#include "or1kiss/utils.h"
#include "or1kiss/exception.h"

#include "or1kiss/bitops.h"
#include "or1kiss/decode.h"
#include "or1kiss/disasm.h"
#include "or1kiss/endian.h"
#include "or1kiss/port.h"
#include "or1kiss/tick.h"
#include "or1kiss/insn.h"
#include "or1kiss/mmu.h"
#include "or1kiss/spr.h"

#define OR1KISS_VER          (0x12) /* CPU Version (deprecated) */
#define OR1KISS_CFG          (0x00) /* Configuration Template (deprecated) */
#define OR1KISS_UVRP         (0x01) /* Updated Version Registers present */
#define OR1KISS_REF          (0x01) /* Processor Revision (deprecated) */

/* OR1KISS_VERSION initializes the Version register */
#define OR1KISS_VERSION      ((OR1KISS_VER  & 0xff) << 24 | \
                              (OR1KISS_CFG  & 0xff) << 16 | \
                              (OR1KISS_UVRP & 0x01) <<  6 | \
                              (OR1KISS_REF  & 0x3f))

#define OR1KISS_CPU_ID       (0x42)     /* CPU Identification Number */
#define OR1KISS_CPU_VER      (0x000001) /* CPU Version (imp. specific) */

/* CPU_VERSION is used to initialize the Version2 register */
#define OR1KISS_CPU_VERSION  ((OR1KISS_CPU_ID  & 0xff) << 24 | \
                              (OR1KISS_CPU_VER & 0xffffff))

#define OR1KISS_ARCH_MAJOR   (0x01) /* Major Architecture Version Number */
#define OR1KISS_ARCH_MINOR   (0x01) /* Minor Architecture Version Number */
#define OR1KISS_ARCH_REV     (0x01) /* Architecture Revision Number */

/* ORKISS_ARCH_VERSION is used to initialize the AVR register */
#define OR1KISS_ARCH_VERSION ((OR1KISS_ARCH_MAJOR & 0xff) << 24 | \
                              (OR1KISS_ARCH_MINOR & 0xff) << 16 | \
                              (OR1KISS_ARCH_REV   & 0xff) <<  8)

/* 100MHz clock */
#define OR1KISS_CLOCK        (100000000)

/* Support for non-maskable interrupts (needed for SMP linux) */
#define OR1KISS_PIC_NMI      (0x3) /* IRQ0 and IRQ1 are non-maskable */

namespace or1kiss {

    enum supervisor_status {
        SR_SM    = 1 <<  0, /* Supervisor Mode */
        SR_TEE   = 1 <<  1, /* Tick Timer Exception Enabled */
        SR_IEE   = 1 <<  2, /* Interrupt Exception Enabled */
        SR_DCE   = 1 <<  3, /* Data Cache Enabled */
        SR_ICE   = 1 <<  4, /* Instruction Cache Enabled */
        SR_DME   = 1 <<  5, /* Data MMU Enabled */
        SR_IME   = 1 <<  6, /* Instruction MMU Enabled */
        SR_LEE   = 1 <<  7, /* Little Endian Enabled */
        SR_CE    = 1 <<  8, /* Context ID Enabled */
        SR_F     = 1 <<  9, /* Conditional Branch Flag */
        SR_CY    = 1 << 10, /* Carry Flag */
        SR_OV    = 1 << 11, /* Overflow Flag */
        SR_OVE   = 1 << 12, /* Overflow Exception Enabled */
        SR_DSX   = 1 << 13, /* Delay Slot Exception */
        SR_EPH   = 1 << 14, /* Exception Prefix High */
        SR_FO    = 1 << 15, /* Fixed One */
        SR_SUMRA = 1 << 16  /* SPR User Mode Read Access */
    };

    enum cpucfg {
        CPUCFGR_NSGF   = 1 <<  0, /* number of shadow GRP files */
        CPUCFGR_CGF    = 1 <<  4, /* custom GPR file */
        CPUCFGR_OB32S  = 1 <<  5, /* ORBIS32 supported */
        CPUCFGR_OB64S  = 1 <<  6, /* ORBIS64 supported */
        CPUCFGR_OF32S  = 1 <<  7, /* ORFPX32 supported */
        CPUCFGR_OF64S  = 1 <<  8, /* ORFPX64 supported */
        CPUCFGR_OV64S  = 1 <<  9, /* ORVDX64 supported */
        CPUCFGR_ND     = 1 << 10, /* no delay for jump/branch */
        CPUCFGR_AVRP   = 1 << 11, /* AVR present */
        CPUCFGR_EVBARP = 1 << 12, /* EVBAR present */
        CPUCFGR_ISRP   = 1 << 13, /* ISR present */
        CPUCFGR_AECSRP = 1 << 14  /* AECR/AESR present */
    };

    enum arithmetic_exception {
        AE_CYADDE    = 1 << 0, /* Carry on Add Exception */
        AE_OVADDE    = 1 << 1, /* Overflow Add Exception */
        AE_CYMULE    = 1 << 2, /* Carry on Multiply Exception */
        AE_OVMULE    = 1 << 3, /* Overflow on Multiply Exception */
        AE_DBZE      = 1 << 4, /* Divide By Zero Exception */
        AE_CYMACADDE = 1 << 5, /* Carry on MAC Addition Exception */
        AE_OVMACADDE = 1 << 6  /* Overflow on MAC Addition Exception */
    };

    enum fp_status {
        FPS_FPEE = 1 <<  0, /* Floating Point Exception Enabled */
        FPS_RMN  = 0 <<  1, /* Round to Nearest */
        FPS_RMZ  = 1 <<  1, /* Round to Zero */
        FPS_RMU  = 2 <<  1, /* Round up */
        FPS_RMD  = 3 <<  1, /* Round down */
        FPS_OV   = 1 <<  3, /* Overflow Flag */
        FPS_UNF  = 1 <<  4, /* Underflow Flag */
        FPS_SNF  = 1 <<  5, /* SNAN Flag */
        FPS_QNF  = 1 <<  6, /* QNAN Flag */
        FPS_ZF   = 1 <<  7, /* Zero Flag */
        FPS_IXF  = 1 <<  8, /* Inexact Flag */
        FPS_IVF  = 1 <<  9, /* Invalid Flag */
        FPS_INF  = 1 << 10, /* Infinity Flag */
        FPS_DZF  = 1 << 11  /* Divide by Zero Flag */
    };

    enum pmr_status {
        PMR_SDF  = 0xf << 0, /* Slow Down Factor */
        PMR_DME  =   1 << 4, /* Doze Mode Enable */
        PMR_SME  =   1 << 5, /* Sleep Mode Enable */
        PMR_DCGE =   1 << 6, /* Dynamic Clock Gating Enable */
        PMR_SUME =   1 << 7, /* Suspend Mode Enable */
    };

    enum upr {
        UPR_UP   = 1 <<  0, /* UPR Present */
        UPR_DCP  = 1 <<  1, /* DATA Cache Present */
        UPR_ICP  = 1 <<  2, /* INSN Cache Present */
        UPR_DMP  = 1 <<  3, /* DATA MMU Present */
        UPR_IMP  = 1 <<  4, /* INSN MMU Present */
        UPR_MP   = 1 <<  5, /* MAC Present */
        UPR_DUP  = 1 <<  6, /* Debug Unit Present */
        UPR_PCUP = 1 <<  7, /* Performance Counters Unit Present */
        UPR_PICP = 1 <<  8, /* Programmable Interrupt Controller Present */
        UPR_PMP  = 1 <<  9, /* Power Management Present */
        UPR_TTP  = 1 << 10  /* Tick Timer Present */
    };

    enum or1kiss_exception_prio {
        EX_RESET           =  0, /* Hardware reset */
        EX_INSN_ALIGNMENT  =  1, /* Jump target not naturally aligned */
        EX_INSN_TLB_MISS   =  2, /* Instruction TLB Miss */
        EX_INSN_PAGE_FAULT =  3, /* Instruction Page Fault */
        EX_INSN_BUS_ERROR  =  4, /* Instruction Bus Error */
        EX_DATA_ALIGNMENT  =  5, /* Load/Store not naturally aligned */
        EX_DATA_TLB_MISS   =  6, /* Data TLB Miss */
        EX_DATA_PAGE_FAULT =  7, /* Data Page Fault */
        EX_DATA_BUS_ERROR  =  8, /* Data Bus Error */
        EX_ILLEGAL_INSN    =  9, /* Illegal Instruction */
        EX_SYSCALL         = 10, /* System Call */
        EX_TRAP            = 11, /* Trap Interrupt (l.trap) */
        EX_RANGE           = 12, /* Overflow Exception */
        EX_FP              = 13, /* Floating Point Exception */
        EX_TICK_TIMER      = 14, /* Tick Timer Interrupt */
        EX_EXTERNAL        = 15  /* External IRQ line asserted */
    };


#define OR1KISS_EX_TLB_MISS(imem)   \
    ((imem) ? EX_ITLB_MISS : EX_DTLB_MISS)
#define OR1KISS_EX_PAGE_FAULT(imem) \
    ((imem) ? EX_INSN_PAGE_FAULT : EX_DATA_PAGE_FAULT)
#define OR1KISS_EX_BUS_ERROR(imem)  \
    ((imem) ? EX_INSN_BUS_ERROR : EX_DATA_BUS_ERROR)

    enum nop_mode {
        NOP             = 0x0, /* Regular nop */
        NOP_EXIT        = 0x1, /* Exit simulation */
        NOP_REPORT      = 0x2, /* Simple report */
        NOP_PUTC        = 0x4, /* Print char in r3 */
        NOP_CNT_RESET   = 0x5, /* Reset statistics counters */
        NOP_GET_TICKS   = 0x6, /* Get number of ticks running */
        NOP_GET_PS      = 0x7, /* Get picoseconds per cycle */
        NOP_TRACE_ON    = 0x8, /* Turn on tracing */
        NOP_TRACE_OFF   = 0x9, /* Turn off tracing */
        NOP_RANDOM      = 0xa, /* Return 4 random bytes */
        NOP_OR1KSIM     = 0xb, /* Return non-zero if this a simulation */
        NOP_SILENT_EXIT = 0xc, /* End of simulation, exit silently */
        NOP_HOST_TIME   = 0xd, /* Current host time, in milliseconds */
        NOP_PUTS        = 0xe  /* Print string */
    };

    enum step_result {
        STEP_OK = 0,     /* Quantum step finished regularly */
        STEP_EXIT,       /* Exit request from software */
        STEP_BREAKPOINT, /* Quantum step stopped due to breakpoint hit */
        STEP_WATCHPOINT  /* Quantum step stopped due to watchpoint hit */
    };

    typedef union _double_register {
        double d;
        u64    u;
        s64    i;
        struct {
            u32 hi;
            u32 lo;
        };
    } double_register; // size must be 8 bytes!

    class or1k
    {
    private:
        decode_cache    m_decode_cache;
        decode_function m_decode_table[NUM_OPCODES];

        bool m_stop_requested;
        bool m_break_requested;

        u64 m_instructions;
        u64 m_cycles;
        u64 m_compiles;
        u64 m_limit;
        u64 m_sleep_cycles;

        clock_t  m_clock;

        u32 m_jump_target;
        u64 m_jump_insn;

        u32 m_phys_ipg;
        u32 m_virt_ipg;

        u32 m_prev_pc;
        u32 m_next_pc;

        u32 m_version;
        u32 m_version2;
        u32 m_avr;

        u32 m_dccfgr;
        u32 m_iccfgr;

        u32 m_unit;
        u32 m_cpucfg;
        u32 m_fpcfg;
        u32 m_status;
        u32 m_insn;

        u32 m_aecr;
        u32 m_aesr;

        u32 m_exsr;
        u32 m_expc;
        u32 m_exea;

        u32 m_shadow[OR1KISS_SHADOW_REGS];

        double_register m_mac;
        double_register m_fmac;

        u32  m_pmr;
        bool m_allow_sleep;

        u32 m_pic_mr;
        u32 m_pic_sr;

        u32 m_core_id;
        u32 m_num_cores;

        u32 m_excl_addr;
        u32 m_excl_data;

        u64 m_num_excl_read;
        u64 m_num_excl_write;
        u64 m_num_excl_failed;

        u64 m_tick_update;

        tick  m_tick;
        mmu   m_dmmu;
        mmu   m_immu;
        port* m_port;

        request m_ireq;
        request m_dreq;

        u8* m_imem_ptr;
        u32 m_imem_start;
        u32 m_imem_end;
        u64 m_imem_cycles;

        u8* m_dmem_ptr;
        u32 m_dmem_start;
        u32 m_dmem_end;
        u64 m_dmem_cycles;

        vector<u32> m_breakpoints;
        vector<u32> m_watchpoints_r;
        vector<u32> m_watchpoints_w;

        bool m_trace_enabled;
        u32  m_trace_addr;
        ostream*  m_user_trace_stream;
        ofstream* m_file_trace_stream;

        int m_fp_round_mode;

        inline void setup_fp_round_mode();
        inline void reset_fp_round_mode();

        inline void reset_fp_flags(float result);
        inline void reset_fp_flags(double result);

        inline bool breaks_quantum(const instruction* insn);

        inline void schedule_jump(u32 target, u32 delay);

        inline instruction* fetch();

        step_result advance(unsigned int cycles);

        void doze();
        bool transact(request& req);
        void exception(unsigned int type, u32 addr = 0);
        void do_trace(const instruction*);

        void update_timer();

        void vwarn(const char* format, va_list args) const;

        void warn(const char* format, ...) const;
        bool warn(bool condition, const char* format, ...) const;

        u64 next_breakpoint() const;
        bool breakpoint_hit() const;

        /* ORBIS32 */
        void decode_orbis32_mfspr(instruction*);
        void decode_orbis32_mtspr(instruction*);
        void decode_orbis32_movhi(instruction*);
        void decode_orbis32_nop(instruction*);

        /* Control */
        void decode_orbis32_bf(instruction*);
        void decode_orbis32_bnf(instruction*);
        void decode_orbis32_j(instruction*);
        void decode_orbis32_jr(instruction*);
        void decode_orbis32_jal(instruction*);
        void decode_orbis32_jalr(instruction*);

        /* Load & Store */
        void decode_orbis32_lwa(instruction*);
        void decode_orbis32_lwz(instruction*);
        void decode_orbis32_lws(instruction*);
        void decode_orbis32_lhz(instruction*);
        void decode_orbis32_lhs(instruction*);
        void decode_orbis32_lbz(instruction*);
        void decode_orbis32_lbs(instruction*);
        void decode_orbis32_swa(instruction*);
        void decode_orbis32_sw(instruction*);
        void decode_orbis32_sh(instruction*);
        void decode_orbis32_sb(instruction*);

        /* Sign/Zero Extend */
        void decode_orbis32_extwz(instruction*);
        void decode_orbis32_extws(instruction*);
        void decode_orbis32_exthz(instruction*);
        void decode_orbis32_exths(instruction*);
        void decode_orbis32_extbz(instruction*);
        void decode_orbis32_extbs(instruction*);

        /* ALU (reg, reg) */
        void decode_orbis32_add(instruction*);
        void decode_orbis32_addc(instruction*);
        void decode_orbis32_sub(instruction*);
        void decode_orbis32_and(instruction*);
        void decode_orbis32_or(instruction*);
        void decode_orbis32_xor(instruction*);
        void decode_orbis32_cmov(instruction*);
        void decode_orbis32_ff1(instruction*);
        void decode_orbis32_fl1(instruction*);
        void decode_orbis32_sll(instruction*);
        void decode_orbis32_srl(instruction*);
        void decode_orbis32_sra(instruction*);
        void decode_orbis32_ror(instruction*);
        void decode_orbis32_mul(instruction*);
        void decode_orbis32_mulu(instruction*);
        void decode_orbis32_muld(instruction*);
        void decode_orbis32_muldu(instruction*);
        void decode_orbis32_div(instruction*);
        void decode_orbis32_divu(instruction*);

        /* ALU (reg, imm) */
        void decode_orbis32_addi(instruction*);
        void decode_orbis32_addic(instruction*);
        void decode_orbis32_andi(instruction*);
        void decode_orbis32_ori(instruction*);
        void decode_orbis32_xori(instruction*);
        void decode_orbis32_rori(instruction*);
        void decode_orbis32_slli(instruction*);
        void decode_orbis32_srai(instruction*);
        void decode_orbis32_srli(instruction*);
        void decode_orbis32_muli(instruction*);

        /* Comparison (reg, reg) */
        void decode_orbis32_sfeq(instruction*);
        void decode_orbis32_sfne(instruction*);
        void decode_orbis32_sfgtu(instruction*);
        void decode_orbis32_sfgeu(instruction*);
        void decode_orbis32_sfltu(instruction*);
        void decode_orbis32_sfleu(instruction*);
        void decode_orbis32_sfgts(instruction*);
        void decode_orbis32_sfges(instruction*);
        void decode_orbis32_sflts(instruction*);
        void decode_orbis32_sfles(instruction*);

        /* Comparison (reg, imm) */
        void decode_orbis32_sfeqi(instruction*);
        void decode_orbis32_sfnei(instruction*);
        void decode_orbis32_sfgesi(instruction*);
        void decode_orbis32_sfgeui(instruction*);
        void decode_orbis32_sfgtsi(instruction*);
        void decode_orbis32_sfgtui(instruction*);
        void decode_orbis32_sflesi(instruction*);
        void decode_orbis32_sfleui(instruction*);
        void decode_orbis32_sfltsi(instruction*);
        void decode_orbis32_sfltui(instruction*);

        /* MAC Unit */
        void decode_orbis32_mac(instruction*);
        void decode_orbis32_maci(instruction*);
        void decode_orbis32_macu(instruction*);
        void decode_orbis32_macrc(instruction*);
        void decode_orbis32_msb(instruction*);
        void decode_orbis32_msbu(instruction*);

        /* System Interface */
        void decode_orbis32_sys(instruction*);
        void decode_orbis32_trap(instruction*);
        void decode_orbis32_csync(instruction*);
        void decode_orbis32_msync(instruction*);
        void decode_orbis32_psync(instruction*);
        void decode_orbis32_rfe(instruction*);

        /* ORFPX32 */
        void decode_orfpx32_add(instruction*);
        void decode_orfpx32_sub(instruction*);
        void decode_orfpx32_mul(instruction*);
        void decode_orfpx32_div(instruction*);
        void decode_orfpx32_itof(instruction*);
        void decode_orfpx32_ftoi(instruction*);
        void decode_orfpx32_rem(instruction*);
        void decode_orfpx32_madd(instruction*);
        void decode_orfpx32_sfeq(instruction*);
        void decode_orfpx32_sfne(instruction*);
        void decode_orfpx32_sfgt(instruction*);
        void decode_orfpx32_sfge(instruction*);
        void decode_orfpx32_sflt(instruction*);
        void decode_orfpx32_sfle(instruction*);

        /* ORFPX64 */
        void decode_orfpx64_add(instruction*);
        void decode_orfpx64_sub(instruction*);
        void decode_orfpx64_mul(instruction*);
        void decode_orfpx64_div(instruction*);
        void decode_orfpx64_itof(instruction*);
        void decode_orfpx64_ftoi(instruction*);
        void decode_orfpx64_rem(instruction*);
        void decode_orfpx64_madd(instruction*);
        void decode_orfpx64_sfeq(instruction*);
        void decode_orfpx64_sfne(instruction*);
        void decode_orfpx64_sfgt(instruction*);
        void decode_orfpx64_sfge(instruction*);
        void decode_orfpx64_sflt(instruction*);
        void decode_orfpx64_sfle(instruction*);

        void decode_na(instruction*);

        /* ORBIS32 */
        void execute_orbis32_mfspr(instruction*);
        void execute_orbis32_mtspr(instruction*);
        void execute_orbis32_movhi(instruction*);
        void execute_orbis32_nop(instruction*);

        /* Control */
        void execute_orbis32_bf(instruction*);
        void execute_orbis32_bnf(instruction*);
        void execute_orbis32_jump_rel(instruction*);
        void execute_orbis32_jump_abs(instruction*);

        /* Load & Store */
        void execute_orbis32_lwa(instruction*);
        void execute_orbis32_lw(instruction*);
        void execute_orbis32_lhz(instruction*);
        void execute_orbis32_lhs(instruction*);
        void execute_orbis32_lbz(instruction*);
        void execute_orbis32_lbs(instruction*);
        void execute_orbis32_swa(instruction*);
        void execute_orbis32_sw(instruction*);
        void execute_orbis32_sh(instruction*);
        void execute_orbis32_sb(instruction*);

        /* Sign/Zero Extend */
        void execute_orbis32_extw(instruction*);
        void execute_orbis32_exthz(instruction*);
        void execute_orbis32_exths(instruction*);
        void execute_orbis32_extbz(instruction*);
        void execute_orbis32_extbs(instruction*);

        /* Arithmetic & Logic */
        void execute_orbis32_add(instruction*);
        void execute_orbis32_addc(instruction*);
        void execute_orbis32_sub(instruction*);
        void execute_orbis32_and(instruction*);
        void execute_orbis32_or(instruction*);
        void execute_orbis32_xor(instruction*);
        void execute_orbis32_cmov(instruction*);
        void execute_orbis32_ff1(instruction*);
        void execute_orbis32_fl1(instruction*);
        void execute_orbis32_sll(instruction*);
        void execute_orbis32_srl(instruction*);
        void execute_orbis32_sra(instruction*);
        void execute_orbis32_ror(instruction*);
        void execute_orbis32_mul(instruction*);
        void execute_orbis32_mulu(instruction*);
        void execute_orbis32_muld(instruction*);
        void execute_orbis32_muldu(instruction*);
        void execute_orbis32_div(instruction*);
        void execute_orbis32_divu(instruction*);

        /* Comparision */
        void execute_orbis32_sfeq(instruction*);
        void execute_orbis32_sfne(instruction*);
        void execute_orbis32_sfgtu(instruction*);
        void execute_orbis32_sfgeu(instruction*);
        void execute_orbis32_sfltu(instruction*);
        void execute_orbis32_sfleu(instruction*);
        void execute_orbis32_sfgts(instruction*);
        void execute_orbis32_sfges(instruction*);
        void execute_orbis32_sflts(instruction*);
        void execute_orbis32_sfles(instruction*);

        /* MAC unit */
        void execute_orbis32_mac(instruction*);
        void execute_orbis32_macu(instruction*);
        void execute_orbis32_msb(instruction*);
        void execute_orbis32_msbu(instruction*);
        void execute_orbis32_macrc(instruction*);

        /* System Interface */
        void execute_orbis32_sys(instruction*);
        void execute_orbis32_trap(instruction*);
        void execute_orbis32_csync(instruction*);
        void execute_orbis32_msync(instruction*);
        void execute_orbis32_psync(instruction*);
        void execute_orbis32_rfe(instruction*);

        /* ORFPX32 */
        void execute_orfpx32_add(instruction*);
        void execute_orfpx32_sub(instruction*);
        void execute_orfpx32_mul(instruction*);
        void execute_orfpx32_div(instruction*);
        void execute_orfpx32_rem(instruction*);
        void execute_orfpx32_madd(instruction*);
        void execute_orfpx32_itof(instruction*);
        void execute_orfpx32_ftoi(instruction*);
        void execute_orfpx32_sfeq(instruction*);
        void execute_orfpx32_sfne(instruction*);
        void execute_orfpx32_sfgt(instruction*);
        void execute_orfpx32_sfge(instruction*);
        void execute_orfpx32_sflt(instruction*);
        void execute_orfpx32_sfle(instruction*);

        /* ORFPX64 */
        void execute_orfpx64_add(instruction*);
        void execute_orfpx64_sub(instruction*);
        void execute_orfpx64_mul(instruction*);
        void execute_orfpx64_div(instruction*);
        void execute_orfpx64_rem(instruction*);
        void execute_orfpx64_madd(instruction*);
        void execute_orfpx64_itof(instruction*);
        void execute_orfpx64_ftoi(instruction*);
        void execute_orfpx64_sfeq(instruction*);
        void execute_orfpx64_sfne(instruction*);
        void execute_orfpx64_sfgt(instruction*);
        void execute_orfpx64_sfge(instruction*);
        void execute_orfpx64_sflt(instruction*);
        void execute_orfpx64_sfle(instruction*);

        // Disabled
        or1k();
        or1k(const or1k&);

    public:
        u32 GPR[32];

        inline bool is_dmmu_active()      const { return m_status & SR_DME; }
        inline bool is_immu_active()      const { return m_status & SR_IME; }
        inline bool is_supervisor()       const { return m_status & SR_SM;  }
        inline bool is_ext_irq_enabled()  const { return m_status & SR_IEE; }
        inline bool is_tick_irq_enabled() const { return m_status & SR_TEE; }

        inline bool is_interrupt_pending() const {
            return m_pic_sr & m_pic_mr;
        }

        inline bool is_interrupt_pending(int no) const {
            return (m_pic_sr & m_pic_mr) & (1 << no);
        }

        inline bool is_exception_pending() const {
            return is_interrupt_pending() || m_tick.irq_pending();
        }

        inline bool is_sleep_allowed() const   { return m_allow_sleep; }
        inline void allow_sleep(bool b = true) { m_allow_sleep = b; }

        inline bool is_sleeping() const {
            return is_sleep_allowed() &&
                   !is_exception_pending() &&
                   (m_pmr & PMR_DME);
        }

        inline clock_t get_clock() const { return m_clock; }
        inline void set_clock(u32 clk)   { m_clock = clk; }

        inline u32 get_core_id()  const { return m_core_id; }
        inline void set_core_id(u32 id) { m_core_id = id; }
        inline u32 get_numcores() const { return m_num_cores; }
        inline void set_numcores(u32 n) { m_num_cores = n; }

        inline u64 get_num_lwa() const { return m_num_excl_read; }
        inline u64 get_num_swa() const { return m_num_excl_write; }
        inline u64 get_num_swa_failed() const {
            return m_num_excl_failed;
        }

        inline void reset_exclusive() {
            m_num_excl_read = m_num_excl_write = m_num_excl_failed = 0;
        }

        inline bool is_decode_cache_off() const {
            return m_decode_cache.is_enabled();
        }

        inline u64 get_num_cycles()       const { return m_cycles; }
        inline u64 get_num_instructions() const { return m_instructions; }
        inline u64 get_num_compiles()     const { return m_compiles; }
        inline u64 get_num_sleep_cycles() const { return m_sleep_cycles; }

        inline float get_decode_cache_hit_rate() const;

        inline void reset_cycles()       { m_cycles = 0; }
        inline void reset_instructions() { m_instructions = 0; }
        inline void reset_compiles()     { m_compiles = 0; }
        inline void reset_sleep_cycles() { m_sleep_cycles = 0; }

        inline void trigger_tlb_miss(u32 addr);

        inline void set_insn_ptr(unsigned char* ptr,
                                 u32 addr_start = 0x00000000,
                                 u32 addr_end   = 0xffffffff,
                                 u64 cycles = 0);
        inline void set_data_ptr(unsigned char* ptr,
                                 u32 addr_start = 0x00000000,
                                 u32 addr_end   = 0xffffffff,
                                 u64 cycles = 0);

        inline port* get_port() { return m_port; }
        inline mmu*  get_dmmu() { return &m_dmmu; }
        inline mmu*  get_immu() { return &m_immu; }

        void interrupt(int, bool);

        u32  get_spr(u32, bool = false) const;
        void set_spr(u32, u32, bool = false);

        or1k(port*, decode_cache_size = DECODE_CACHE_SIZE_8M);
        virtual ~or1k();

        step_result step(unsigned int& cycles);
        step_result run(unsigned int quantum = ~0u);

        void insert_breakpoint(u32 addr);
        void remove_breakpoint(u32 addr);

        void insert_watchpoint_r(u32 addr);
        void remove_watchpoint_r(u32 addr);

        void insert_watchpoint_w(u32 addr);
        void remove_watchpoint_w(u32 addr);

        inline const std::vector<u32>& get_breakpoints() const;
        inline       std::vector<u32>  get_breakpoints();
        inline const std::vector<u32>& get_watchpoints_r() const;
        inline       std::vector<u32>  get_watchpoints_r();
        inline const std::vector<u32>& get_watchpoints_w() const;
        inline       std::vector<u32>  get_watchpoints_w();

        void trace(ostream& = std::cout);
        void trace(const string&);
    };

    inline void or1k::setup_fp_round_mode() {
        // Setup rounding mode
        m_fp_round_mode = fegetround();
        u32 rounding_mode = m_fpcfg & 0x6;
        switch (rounding_mode) {
        case FPS_RMN: fesetround(FE_TONEAREST);  break;
        case FPS_RMZ: fesetround(FE_TOWARDZERO); break;
        case FPS_RMU: fesetround(FE_UPWARD);     break;
        case FPS_RMD: fesetround(FE_DOWNWARD);   break;
        default: OR1KISS_ERROR("Unknown rounding mode (%d)", rounding_mode);
        }
    }

    inline void or1k::reset_fp_round_mode() {
        fesetround(m_fp_round_mode);
    }

    inline void or1k::reset_fp_flags(float result) {
        // Clear flags
        m_fpcfg &= ~(FPS_ZF | FPS_INF | FPS_QNF | FPS_OV | FPS_UNF | FPS_DZF);

        // Test Zero
        if (result == 0.0f) {
            m_fpcfg |= FPS_ZF;
            if (m_fpcfg & FPS_FPEE)
                exception(EX_FP);
        }

        // Test Infinity
        if (isinff(result)) {
            m_fpcfg |= FPS_INF;
            if (m_fpcfg & FPS_FPEE)
                exception(EX_FP);
        }

        // Test NaN
        if (isnanf(result)) {
            m_fpcfg |= FPS_QNF;
            if (m_fpcfg & FPS_FPEE)
                exception(EX_FP);
        }

        // Test Overflow
        if (fetestexcept(FE_OVERFLOW)) {
            m_fpcfg |= FPS_OV;
            if (m_fpcfg & FPS_FPEE)
                exception(EX_FP);
        }

        // Test Underflow
        if (fetestexcept(FE_UNDERFLOW)) {
            m_fpcfg |= FPS_UNF;
            if (m_fpcfg & FPS_FPEE)
                exception(EX_FP);
        }

        // Test Division by Zero
        if (fetestexcept(FE_DIVBYZERO)) {
            m_fpcfg |= FPS_DZF;
            if (m_fpcfg & FPS_FPEE)
                exception(EX_FP);
        }
    }

    inline void or1k::reset_fp_flags(double result) {
        // Clear flags
        m_fpcfg &= ~(FPS_ZF | FPS_INF | FPS_QNF | FPS_OV | FPS_UNF | FPS_DZF);

        // Test Zero
        if (result == 0.0) {
            m_fpcfg |= FPS_ZF;
            if (m_fpcfg & FPS_FPEE)
                exception(EX_FP);
        }

        // Test Infinity
        if (isinf(result)) {
            m_fpcfg |= FPS_INF;
            if (m_fpcfg & FPS_FPEE)
                exception(EX_FP);
        }

        // Test NaN
        if (isnan(result)) {
            m_fpcfg |= FPS_QNF;
            if (m_fpcfg & FPS_FPEE)
                exception(EX_FP);
        }

        // Test Overflow
        if (fetestexcept(FE_OVERFLOW)) {
            m_fpcfg |= FPS_OV;
            if (m_fpcfg & FPS_FPEE)
                exception(EX_FP);
        }

        // Test Underflow
        if (fetestexcept(FE_UNDERFLOW)) {
            m_fpcfg |= FPS_UNF;
            if (m_fpcfg & FPS_FPEE)
                exception(EX_FP);
        }

        // Test Division by Zero
        if (fetestexcept(FE_DIVBYZERO)) {
            m_fpcfg |= FPS_DZF;
            if (m_fpcfg & FPS_FPEE)
                exception(EX_FP);
        }
    }

    inline bool or1k::breaks_quantum(const instruction* insn) {
        if (insn == NULL)
            return true;
        if (insn->exec == &or1k::execute_orbis32_mtspr)
            return true;
        return false;
    }

    inline void or1k::schedule_jump(u32 target, u32 delay) {
        m_jump_target = target;
        m_jump_insn = m_instructions + delay;

        if (!is_aligned(m_jump_target, 4))
            exception(EX_INSN_ALIGNMENT, m_jump_target);
    }

    inline float or1k::get_decode_cache_hit_rate() const {
        if (m_instructions == 0)
            return 0.0f;

        u64 hits = m_instructions - m_compiles;
        if (hits == 0)
            return 0.0f;

        return (float)(hits) / m_instructions;
    }

    inline void or1k::trigger_tlb_miss(u32 addr) {
        exception(EX_DATA_TLB_MISS, addr);
    }

    inline void or1k::set_insn_ptr(unsigned char* ptr, u32 start,
                                   u32 end, u64 cycles) {
        if (start > end)
            OR1KISS_ERROR("invalid range specified %u..%u", start, end);

        m_imem_ptr = ptr;
        m_imem_start = start;
        m_imem_end = end;
        m_imem_cycles = cycles;
    }

    inline void or1k::set_data_ptr(unsigned char* ptr, u32 start,
                                   u32 end, u64 cycles) {
        if (start > end)
            OR1KISS_ERROR("invalid range specified %u..%u", start, end);

        m_dmem_ptr = ptr;
        m_dmem_start = start;
        m_dmem_end = end;
        m_dmem_cycles = cycles;
    }

    inline const std::vector<u32>& or1k::get_breakpoints() const {
        return m_breakpoints;
    }

    inline std::vector<u32> or1k::get_breakpoints() {
        return m_breakpoints;
    }

    inline const std::vector<u32>& or1k::get_watchpoints_r() const {
        return m_watchpoints_r;
    }

    inline std::vector<u32> or1k::get_watchpoints_r() {
        return m_watchpoints_r;
    }

    inline const std::vector<u32>& or1k::get_watchpoints_w() const {
        return m_watchpoints_w;
    }

    inline std::vector<u32> or1k::get_watchpoints_w() {
        return m_watchpoints_w;
    }

}

#endif
