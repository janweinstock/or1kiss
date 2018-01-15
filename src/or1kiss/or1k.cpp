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

    step_result or1k::advance(unsigned int cycles)
    {
        // Start simulation for a quantum of n cycles. We assume every
        // instruction takes one cycle to complete. If an instruction takes
        // longer, we might overshoot this limit.
        m_limit = m_cycles + cycles;

        // Check for any unmasked interrupts
        if (unlikely(m_pic_sr & m_pic_mr))
            exception(EX_EXTERNAL);

        // Check if we can sleep: doze() will advance the cycle counter until
        // the first cycle we are not allowed to sleep anymore. Therefore it is
        // possible that after a call to doze() the cycle counter reaches the
        // limit and the loop below will not be entered.
        doze();

        while (m_cycles < m_limit) {
            m_stop_requested = false;
            m_break_requested = false;

            u64 limit = min(m_limit, next_breakpoint());
            if (m_tick.enabled())
                limit = min(limit, m_cycles + m_tick.next_tick());

            // This loop is performance critical. Make it as fast as possible.
            while (m_cycles < limit) {
                // Begin new cycle.
                m_cycles++;
                m_instructions++;

                // Fetch the instruction. If possible, fetch returns the
                // instruction from the instruction cache, otherwise it will
                // fetch it from memory and decode it.
                instruction* insn = fetch();

                // Execute instruction, if the previous instruction fetch
                // completed, i.e. it did not produce an exception.
                if (likely(insn != NULL)) {
                    (this->*insn->exec)(insn);
                    if (unlikely(m_trace_enabled))
                        do_trace(insn);
                }

                // Restore fixed values in case they were tainted
                m_status |= SR_FO;
                GPR[0] = 0;

                // Update program counter and handle jumping
                m_prev_pc = m_next_pc;
                m_next_pc = m_next_pc + 4;

                if (unlikely(m_instructions == m_jump_insn)) {
                    m_next_pc = m_jump_target;
                    limit = min(limit, next_breakpoint());
                }

                // Check if an instruction wanted to exit
                if (unlikely(m_stop_requested))
                    return STEP_EXIT;

                // Break quantum (usually on SPR write)
                if (unlikely(m_break_requested))
                    break;
            }

            // At this point the current mini-quantum has been completed. Since
            // during a mini quantum we are guaranteed that no timer exceptions
            // can happen, the timer may be update only afterwards.
            update_timer();

            // Check for interrupts in case we just woke up from sleep.
            if (unlikely(m_pic_sr & m_pic_mr))
                exception(EX_EXTERNAL);

            // Check if we dropped out due to a breakpoint.
            if (unlikely(breakpoint_hit()))
                return STEP_BREAKPOINT;
        }

        return STEP_OK;
    }

    void or1k::doze()
    {
        // Check if we are allowed to doze
        if (!(m_pmr & PMR_DME) || !(m_allow_sleep))
            return;

        u64 cycles, skip = ~0ull;
        if (m_tick.enabled() && m_tick.irq_enabled())
            skip = min(m_tick.next_tick(), (u64)m_tick.limit());

        if ((cycles = m_port->sleep(skip))) { // try SystemC-sleep first
            m_cycles += cycles;
            m_sleep_cycles += cycles;
            m_limit += cycles;
            m_pmr &= ~PMR_DME;
        } else { // regular sleep, skip over the quantum but stay asleep
            skip = min(skip, m_limit - m_cycles);
            m_cycles += skip;
            m_sleep_cycles += skip;
            update_timer();
        }
    }

    bool or1k::transact(request& req)
    {
        // Set common request properties
        req.set_supervisor(is_supervisor());
        req.cycles = 0;

        assert(req.is_dmem());

        // Remember address for tracing
        m_trace_addr = req.addr;

        // Check if address is properly aligned
        if (!req.is_aligned() && !req.is_debug()) {
            exception(EX_DATA_ALIGNMENT, req.addr);
            return false;
        }

        // Perform address translation
        if (is_dmmu_active()) {
            switch (m_dmmu.translate(req)) {
            case MMU_TLB_MISS:
                exception(EX_DATA_TLB_MISS, req.addr);
                return false;

            case MMU_PAGE_FAULT:
                exception(EX_DATA_PAGE_FAULT, req.addr);
                return false;

            case MMU_OKAY:
            default:
                break;
            }
        }

        // Handle exclusive memory access intricacies
        if (req.is_exclusive()) {
            assert(req.size == SIZE_WORD);
            if (req.is_read())
                m_num_excl_read++;
            if (req.is_write())
                m_num_excl_write++;
        }

        // Let port convert endianess and send the request
        switch (m_port->convert_and_transact(req)) {
        case RESP_ERROR:
            exception(EX_DATA_BUS_ERROR, req.addr);
            return false;

        case RESP_FAILED:
            if (!req.is_exclusive())
                OR1KISS_ERROR("invalid response from port");

            m_status &= ~SR_F;
            m_num_excl_failed++;
            break;

        case RESP_SUCCESS:
            if (req.is_exclusive())
                m_status |= SR_F;
            break;

        default:
            OR1KISS_ERROR("invalid response from port");
        }

        // If this is a non-debug data memory access, it costs one extra cycle
        // to get the data from memory. In case an exception occurs no extra
        // cycle is consumed (ToDo: verify this).
        if (!req.is_debug()) {
            m_cycles += req.cycles;
            m_limit += req.cycles;
        }

        return true;
    }

    instruction* or1k::fetch()
    {
        // Fetch instruction from memory
        m_ireq.set_supervisor(is_supervisor());
        m_ireq.addr = m_next_pc;
        m_ireq.cycles = 0;

        // Perform address translation if MMU is active
        if (is_immu_active()) {
            // First check if we are still on the same page. No need to bother
            // the MMU since we can just compute the virtual address ourself.
            if (OR1KISS_PAGE_COMPARE(m_virt_ipg, m_next_pc)) {
                m_ireq.addr = m_phys_ipg | OR1KISS_PAGE_OFFSET(m_next_pc);
            } else {
                switch (m_immu.translate(m_ireq)) {
                case MMU_TLB_MISS:
                    exception(EX_INSN_TLB_MISS, m_ireq.addr);
                    return NULL;

                case MMU_PAGE_FAULT:
                    exception(EX_INSN_PAGE_FAULT, m_ireq.addr);
                    return NULL;

                default:
                    // Remember the page we are on. We can check next time if
                    // we are still on the same page and compute the physical
                    // address faster.
                    m_virt_ipg = OR1KISS_PAGE_ALIGN(m_next_pc);
                    m_phys_ipg = OR1KISS_PAGE_ALIGN(m_ireq.addr);
                    break;
                }
            }
        }

        // Lookup instruction in cache first
        instruction& insn = m_decode_cache.lookup(m_ireq.addr);
        if ((insn.addr == m_ireq.addr) && (!is_decode_cache_off())) {
            m_insn = insn.insn;
            return &insn; // Cache hit
        }

        // Fetch instruction from memory
        unsigned char* pmem = m_port->get_insn_ptr(m_ireq.addr);
        if (pmem) {
            m_insn = byte_swap(*(u32*)(pmem));
        } else {
            switch (m_port->convert_and_transact(m_ireq)) {
            case RESP_ERROR:
                exception(EX_INSN_BUS_ERROR, m_ireq.addr);
                return NULL;

            case RESP_FAILED:
                OR1KISS_ERROR("invalid response from port");

            case RESP_SUCCESS:
            default:
                break;
            }
        }

        // Try to compile instruction
        opcode code = decode(m_insn);
        if (code == INVALID_OPCODE) {
            exception(EX_ILLEGAL_INSN, m_ireq.addr);
            return NULL;
        }

        // ToDo: just for testing purposes, remove from decode or implement
        // for exceptions and set in SPR_UPR.
        if (code == ORBIS32_CUST1) {
            exception(EX_ILLEGAL_INSN, m_ireq.addr);
            return NULL;
        }

        memset(&insn, 0, sizeof(insn));
        insn.addr = m_ireq.addr;
        insn.insn = m_insn;

        (this->*m_decode_table[code])(&insn);
        m_compiles++;

        // Compilation successful
        return &insn;
    }

    // Exception handler addresses
    static const u32 g_exception_vector[] = {
        0x00000100, /* EX_RESET */
        0x00000600, /* EX_INSN_ALIGNMENT */
        0x00000a00, /* EX_INSN_TLB_MISS */
        0x00000400, /* EX_INSN_PAGE_FAULT */
        0x00000200, /* EX_INSN_BUS_ERROR */
        0x00000600, /* EX_DATA_ALIGNMENT */
        0x00000900, /* EX_DATA_TLB_MISS */
        0x00000300, /* EX_DATA_PAGE_FAULT */
        0x00000200, /* EX_DATA_BUS_ERROR */
        0x00000700, /* EX_ILLEGAL_INSN */
        0x00000c00, /* EX_SYSCALL */
        0x00000e00, /* EX_TRAP */
        0x00000b00, /* EX_RANGE */
        0x00000d00, /* EX_FP */
        0x00000500, /* EX_TICK_TIMER */
        0x00000800, /* EX_EXTERNAL */
    };

    void or1k::exception(unsigned int type, u32 addr)
    {
        // Some exceptions can be turned off, check in reverse priority order.
        if ((type == EX_EXTERNAL) && !(m_status & SR_IEE))
            return;
        if ((type == EX_TICK_TIMER) && !(m_status & SR_TEE))
            return;

        bool is_jump_insn  = (m_instructions == (m_jump_insn - 1));
        bool is_delay_insn = (m_instructions == (m_jump_insn - 0));

        // Determine exception pc
        switch (type) {
        case EX_RESET:
        case EX_INSN_ALIGNMENT:
        case EX_INSN_TLB_MISS:
        case EX_INSN_PAGE_FAULT:
        case EX_INSN_BUS_ERROR:
        case EX_DATA_ALIGNMENT:
        case EX_DATA_TLB_MISS:
        case EX_DATA_PAGE_FAULT:
        case EX_DATA_BUS_ERROR:
        case EX_ILLEGAL_INSN:
        case EX_RANGE:
        case EX_TRAP:
            m_expc = m_next_pc;
            if (is_delay_insn)
                m_expc = m_prev_pc;
            break;

        case EX_SYSCALL:
        case EX_FP:
            m_expc = m_next_pc + 4;
            if (is_jump_insn)
                m_expc = m_jump_target;
            break;

        case EX_TICK_TIMER:
        case EX_EXTERNAL:
            m_expc = m_next_pc;
            if (is_jump_insn)
                m_expc = m_prev_pc;
            break;

        default:
            OR1KISS_ERROR("Unknown exception (%u)", type);
        }

        m_jump_insn = 0;     // Cancel any outstanding jumps
        m_exea = addr;       // Remember exception address
        m_exsr = m_status;   // Save supervisor register
        m_status |= SR_SM;   // Switch to supervisor mode
        if (is_delay_insn)
            m_status |= SR_DSX;

        m_status &= ~SR_IEE; // Disable external interrupts
        m_status &= ~SR_TEE; // Disable tick timer exceptions
        m_status &= ~SR_IME; // Disable instruction MMU
        m_status &= ~SR_DME; // Disable data MMU

        m_pmr &= ~PMR_DME;   // Wake up from doze

        // Calculate address of exception vector
        u32 target = g_exception_vector[type];
        if (m_status & SR_EPH)
            target |= 0xf0000000;

        // Jump to exception vector address. Because external and tick
        // exceptions are raised only after updating the PC, schedule_jump
        // cannot be used and jumping needs to be done manually.
        if (type != EX_TICK_TIMER && type != EX_EXTERNAL)
            schedule_jump(target, 0);
        else
            m_next_pc = target;
    }

    void or1k::interrupt(int id, bool set)
    {
        const u32 irq_mask = 1 << id;

        // Update status register
        if (set)
            m_pic_sr |=  irq_mask;
        else
            m_pic_sr &= ~irq_mask;
    }

    void or1k::update_timer()
    {
        if (m_tick.enabled()) {
            m_tick.update(m_cycles - m_tick_update);
            if (m_tick.irq_pending())
                exception(EX_TICK_TIMER);
        }
        m_tick_update = m_cycles;
    }

    void or1k::vwarn(const char* format, va_list args) const
    {
        vfprintf(stderr, format, args);
    }

    void or1k::warn(const char* format, ...) const
    {
        va_list args;
        va_start(args, format);
        vwarn(format, args);
        va_end(args);
    }

    bool or1k::warn(bool condition, const char* format, ...) const
    {
        if (condition) {
            va_list args;
            va_start(args, format);
            vwarn(format, args);
            va_end(args);
        }

        return condition;
    }

    u64 or1k::next_breakpoint() const
    {
        u64 next = 0xffffffffull;
        for (unsigned int i = m_breakpoints.size(); i != 0; i--) {
            u64 until = (m_breakpoints[i-1] - m_next_pc) / 4;
            next = min(next, until);
        }

        return next + m_cycles;
    }

    bool or1k::breakpoint_hit() const
    {
        for (unsigned int i = 0; i < m_breakpoints.size(); i++) {
            if (m_breakpoints[i] == m_next_pc)
                return true;
        }

        return false;
    }

    or1k::or1k(port* port, decode_cache_size size):
        m_decode_cache(size),
        m_decode_table(),
        m_stop_requested(false),
        m_break_requested(false),
        m_instructions(0),
        m_cycles(0),
        m_compiles(0),
        m_limit(0),
        m_sleep_cycles(0),
        m_clock(OR1KISS_CLOCK),
        m_jump_target(0),
        m_jump_insn(0),
        m_phys_ipg(-1),
        m_virt_ipg(-1),
        m_prev_pc(g_exception_vector[EX_RESET]),
        m_next_pc(g_exception_vector[EX_RESET]),
        m_version(OR1KISS_VERSION),
        m_version2(OR1KISS_CPU_VERSION),
        m_avr(OR1KISS_ARCH_VERSION),
        m_dccfgr(0),
        m_iccfgr(0),
        m_unit(UPR_TTP | UPR_PICP | UPR_MP | UPR_UP |
               UPR_DMP | UPR_IMP  | UPR_PMP),
        m_cpucfg(CPUCFGR_OB32S  | CPUCFGR_OF32S |
                 CPUCFGR_AECSRP | CPUCFGR_AVRP),
        m_fpcfg(0),
        m_status(SR_FO | SR_SM),
        m_insn(0),
        m_aecr(0),
        m_aesr(0),
        m_exsr(0),
        m_expc(0),
        m_exea(0),
        m_shadow(),
        m_mac(),
        m_fmac(),
        m_pmr(),
        m_allow_sleep(true),
        m_pic_mr(OR1KISS_PIC_NMI),
        m_pic_sr(0),
        m_core_id(0),
        m_num_cores(1),
        m_excl_addr((u32)-1),
        m_excl_data(0),
        m_num_excl_read(0),
        m_num_excl_write(0),
        m_num_excl_failed(0),
        m_tick_update(0),
        m_tick(),
        m_dmmu(MMUCFG_NTS128 | MMUCFG_NTW4 | MMUCFG_CRI | MMUCFG_HTR |
               MMUCFG_TEIRI, port),
        m_immu(MMUCFG_NTS128 | MMUCFG_NTW4 | MMUCFG_CRI | MMUCFG_HTR |
               MMUCFG_TEIRI, port),
        m_port(port),
        m_ireq(),
        m_dreq(),
        m_imem_ptr(NULL),
        m_imem_start(0),
        m_imem_end(0),
        m_imem_cycles(0),
        m_dmem_ptr(NULL),
        m_dmem_start(0),
        m_dmem_end(0),
        m_dmem_cycles(0),
        m_breakpoints(),
        m_watchpoints_r(),
        m_watchpoints_w(),
        m_trace_enabled(false),
        m_trace_addr(0),
        m_user_trace_stream(NULL),
        m_file_trace_stream(NULL),
        m_fp_round_mode(0),
        GPR()
    {
        m_ireq.set_read();
        m_ireq.set_imem();
        m_ireq.data = &m_insn;
        m_ireq.size = SIZE_WORD;

        m_dreq.set_dmem();

        // Setup decode table
        m_decode_table[ORBIS32_NOP   ] = &or1k::decode_orbis32_nop;
        m_decode_table[ORBIS32_MFSPR ] = &or1k::decode_orbis32_mfspr;
        m_decode_table[ORBIS32_MTSPR ] = &or1k::decode_orbis32_mtspr;
        m_decode_table[ORBIS32_MOVHI ] = &or1k::decode_orbis32_movhi;

        m_decode_table[ORBIS32_BF    ] = &or1k::decode_orbis32_bf;
        m_decode_table[ORBIS32_BNF   ] = &or1k::decode_orbis32_bnf;
        m_decode_table[ORBIS32_J     ] = &or1k::decode_orbis32_j;
        m_decode_table[ORBIS32_JR    ] = &or1k::decode_orbis32_jr;
        m_decode_table[ORBIS32_JAL   ] = &or1k::decode_orbis32_jal;
        m_decode_table[ORBIS32_JALR  ] = &or1k::decode_orbis32_jalr;

        m_decode_table[ORBIS32_LWA   ] = &or1k::decode_orbis32_lwa;
        m_decode_table[ORBIS32_LD    ] = &or1k::decode_na;
        m_decode_table[ORBIS32_LWZ   ] = &or1k::decode_orbis32_lwz;
        m_decode_table[ORBIS32_LWS   ] = &or1k::decode_orbis32_lws;
        m_decode_table[ORBIS32_LHZ   ] = &or1k::decode_orbis32_lhz;
        m_decode_table[ORBIS32_LHS   ] = &or1k::decode_orbis32_lhs;
        m_decode_table[ORBIS32_LBZ   ] = &or1k::decode_orbis32_lbz;
        m_decode_table[ORBIS32_LBS   ] = &or1k::decode_orbis32_lbs;
        m_decode_table[ORBIS32_SWA   ] = &or1k::decode_orbis32_swa;
        m_decode_table[ORBIS32_SD    ] = &or1k::decode_na;
        m_decode_table[ORBIS32_SW    ] = &or1k::decode_orbis32_sw;
        m_decode_table[ORBIS32_SH    ] = &or1k::decode_orbis32_sh;
        m_decode_table[ORBIS32_SB    ] = &or1k::decode_orbis32_sb;

        m_decode_table[ORBIS32_EXTWZ ] = &or1k::decode_orbis32_extwz;
        m_decode_table[ORBIS32_EXTWS ] = &or1k::decode_orbis32_extws;
        m_decode_table[ORBIS32_EXTHZ ] = &or1k::decode_orbis32_exthz;
        m_decode_table[ORBIS32_EXTHS ] = &or1k::decode_orbis32_exths;
        m_decode_table[ORBIS32_EXTBZ ] = &or1k::decode_orbis32_extbz;
        m_decode_table[ORBIS32_EXTBS ] = &or1k::decode_orbis32_extbs;

        m_decode_table[ORBIS32_ADD   ] = &or1k::decode_orbis32_add;
        m_decode_table[ORBIS32_ADDC  ] = &or1k::decode_orbis32_addc;
        m_decode_table[ORBIS32_SUB   ] = &or1k::decode_orbis32_sub;
        m_decode_table[ORBIS32_AND   ] = &or1k::decode_orbis32_and;
        m_decode_table[ORBIS32_OR    ] = &or1k::decode_orbis32_or;
        m_decode_table[ORBIS32_XOR   ] = &or1k::decode_orbis32_xor;
        m_decode_table[ORBIS32_CMOV  ] = &or1k::decode_orbis32_cmov;
        m_decode_table[ORBIS32_FF1   ] = &or1k::decode_orbis32_ff1;
        m_decode_table[ORBIS32_FL1   ] = &or1k::decode_orbis32_fl1;
        m_decode_table[ORBIS32_SLL   ] = &or1k::decode_orbis32_sll;
        m_decode_table[ORBIS32_SRL   ] = &or1k::decode_orbis32_srl;
        m_decode_table[ORBIS32_SRA   ] = &or1k::decode_orbis32_sra;
        m_decode_table[ORBIS32_ROR   ] = &or1k::decode_orbis32_ror;
        m_decode_table[ORBIS32_MUL   ] = &or1k::decode_orbis32_mul;
        m_decode_table[ORBIS32_MULU  ] = &or1k::decode_orbis32_mulu;
        m_decode_table[ORBIS32_MULD  ] = &or1k::decode_orbis32_muld;
        m_decode_table[ORBIS32_MULDU ] = &or1k::decode_orbis32_muldu;
        m_decode_table[ORBIS32_DIV   ] = &or1k::decode_orbis32_div;
        m_decode_table[ORBIS32_DIVU  ] = &or1k::decode_orbis32_divu;

        m_decode_table[ORBIS32_ADDI  ] = &or1k::decode_orbis32_addi;
        m_decode_table[ORBIS32_ADDIC ] = &or1k::decode_orbis32_addic;
        m_decode_table[ORBIS32_ANDI  ] = &or1k::decode_orbis32_andi;
        m_decode_table[ORBIS32_ORI   ] = &or1k::decode_orbis32_ori;
        m_decode_table[ORBIS32_XORI  ] = &or1k::decode_orbis32_xori;
        m_decode_table[ORBIS32_SLLI  ] = &or1k::decode_orbis32_slli;
        m_decode_table[ORBIS32_SRLI  ] = &or1k::decode_orbis32_srli;
        m_decode_table[ORBIS32_SRAI  ] = &or1k::decode_orbis32_srai;
        m_decode_table[ORBIS32_RORI  ] = &or1k::decode_orbis32_rori;
        m_decode_table[ORBIS32_MULI  ] = &or1k::decode_orbis32_muli;

        m_decode_table[ORBIS32_SFEQ  ] = &or1k::decode_orbis32_sfeq;
        m_decode_table[ORBIS32_SFNE  ] = &or1k::decode_orbis32_sfne;
        m_decode_table[ORBIS32_SFGTU ] = &or1k::decode_orbis32_sfgtu;
        m_decode_table[ORBIS32_SFGEU ] = &or1k::decode_orbis32_sfgeu;
        m_decode_table[ORBIS32_SFLTU ] = &or1k::decode_orbis32_sfltu;
        m_decode_table[ORBIS32_SFLEU ] = &or1k::decode_orbis32_sfleu;
        m_decode_table[ORBIS32_SFGTS ] = &or1k::decode_orbis32_sfgts;
        m_decode_table[ORBIS32_SFGES ] = &or1k::decode_orbis32_sfges;
        m_decode_table[ORBIS32_SFLTS ] = &or1k::decode_orbis32_sflts;
        m_decode_table[ORBIS32_SFLES ] = &or1k::decode_orbis32_sfles;

        m_decode_table[ORBIS32_SFEQI ] = &or1k::decode_orbis32_sfeqi;
        m_decode_table[ORBIS32_SFNEI ] = &or1k::decode_orbis32_sfnei;
        m_decode_table[ORBIS32_SFGTUI] = &or1k::decode_orbis32_sfgtui;
        m_decode_table[ORBIS32_SFGEUI] = &or1k::decode_orbis32_sfgeui;
        m_decode_table[ORBIS32_SFLTUI] = &or1k::decode_orbis32_sfltui;
        m_decode_table[ORBIS32_SFLEUI] = &or1k::decode_orbis32_sfleui;
        m_decode_table[ORBIS32_SFGTSI] = &or1k::decode_orbis32_sfgtsi;
        m_decode_table[ORBIS32_SFGESI] = &or1k::decode_orbis32_sfgesi;
        m_decode_table[ORBIS32_SFLTSI] = &or1k::decode_orbis32_sfltsi;
        m_decode_table[ORBIS32_SFLESI] = &or1k::decode_orbis32_sflesi;

        m_decode_table[ORBIS32_MAC   ] = &or1k::decode_orbis32_mac;
        m_decode_table[ORBIS32_MACU  ] = &or1k::decode_orbis32_macu;
        m_decode_table[ORBIS32_MSB   ] = &or1k::decode_orbis32_msb;
        m_decode_table[ORBIS32_MSBU  ] = &or1k::decode_orbis32_msbu;
        m_decode_table[ORBIS32_MACI  ] = &or1k::decode_orbis32_maci;
        m_decode_table[ORBIS32_MACRC ] = &or1k::decode_orbis32_macrc;

        m_decode_table[ORBIS32_CUST1 ] = &or1k::decode_na;
        m_decode_table[ORBIS32_CUST2 ] = &or1k::decode_na;
        m_decode_table[ORBIS32_CUST3 ] = &or1k::decode_na;
        m_decode_table[ORBIS32_CUST4 ] = &or1k::decode_na;
        m_decode_table[ORBIS32_CUST5 ] = &or1k::decode_na;
        m_decode_table[ORBIS32_CUST6 ] = &or1k::decode_na;
        m_decode_table[ORBIS32_CUST7 ] = &or1k::decode_na;
        m_decode_table[ORBIS32_CUST8 ] = &or1k::decode_na;

        m_decode_table[ORBIS32_SYS   ] = &or1k::decode_orbis32_sys;
        m_decode_table[ORBIS32_TRAP  ] = &or1k::decode_orbis32_trap;
        m_decode_table[ORBIS32_CSYNC ] = &or1k::decode_orbis32_csync;
        m_decode_table[ORBIS32_MSYNC ] = &or1k::decode_orbis32_msync;
        m_decode_table[ORBIS32_PSYNC ] = &or1k::decode_orbis32_psync;
        m_decode_table[ORBIS32_RFE   ] = &or1k::decode_orbis32_rfe;

        m_decode_table[ORFPX32_ADD   ] = &or1k::decode_orfpx32_add;
        m_decode_table[ORFPX32_SUB   ] = &or1k::decode_orfpx32_sub;
        m_decode_table[ORFPX32_MUL   ] = &or1k::decode_orfpx32_mul;
        m_decode_table[ORFPX32_DIV   ] = &or1k::decode_orfpx32_div;
        m_decode_table[ORFPX32_ITOF  ] = &or1k::decode_orfpx32_itof;
        m_decode_table[ORFPX32_FTOI  ] = &or1k::decode_orfpx32_ftoi;
        m_decode_table[ORFPX32_MADD  ] = &or1k::decode_orfpx32_madd;
        m_decode_table[ORFPX32_REM   ] = &or1k::decode_orfpx32_rem;
        m_decode_table[ORFPX32_SFEQ  ] = &or1k::decode_orfpx32_sfeq;
        m_decode_table[ORFPX32_SFNE  ] = &or1k::decode_orfpx32_sfne;
        m_decode_table[ORFPX32_SFGT  ] = &or1k::decode_orfpx32_sfgt;
        m_decode_table[ORFPX32_SFGE  ] = &or1k::decode_orfpx32_sfge;
        m_decode_table[ORFPX32_SFLT  ] = &or1k::decode_orfpx32_sflt;
        m_decode_table[ORFPX32_SFLE  ] = &or1k::decode_orfpx32_sfle;

        m_decode_table[ORFPX64_ADD   ] = &or1k::decode_orfpx64_add;
        m_decode_table[ORFPX64_SUB   ] = &or1k::decode_orfpx64_sub;
        m_decode_table[ORFPX64_MUL   ] = &or1k::decode_orfpx64_mul;
        m_decode_table[ORFPX64_DIV   ] = &or1k::decode_orfpx64_div;
        m_decode_table[ORFPX64_ITOF  ] = &or1k::decode_orfpx64_itof;
        m_decode_table[ORFPX64_FTOI  ] = &or1k::decode_orfpx64_ftoi;
        m_decode_table[ORFPX64_MADD  ] = &or1k::decode_orfpx64_madd;
        m_decode_table[ORFPX64_REM   ] = &or1k::decode_orfpx64_rem;
        m_decode_table[ORFPX64_SFEQ  ] = &or1k::decode_orfpx64_sfeq;
        m_decode_table[ORFPX64_SFNE  ] = &or1k::decode_orfpx64_sfne;
        m_decode_table[ORFPX64_SFGT  ] = &or1k::decode_orfpx64_sfgt;
        m_decode_table[ORFPX64_SFGE  ] = &or1k::decode_orfpx64_sfge;
        m_decode_table[ORFPX64_SFLT  ] = &or1k::decode_orfpx64_sflt;
        m_decode_table[ORFPX64_SFLE  ] = &or1k::decode_orfpx64_sfle;

        m_decode_table[ORFPX32_CUST1 ] = &or1k::decode_na;
        m_decode_table[ORFPX64_CUST1 ] = &or1k::decode_na;
    }

    or1k::~or1k()
    {
        if (m_file_trace_stream != NULL)
            delete m_file_trace_stream;
    }

    step_result or1k::step(unsigned int& cycles)
    {
        // advance may overshoot the cycle budget or it might return early in
        // case a breakpoint was hit or an exit request (nop 0x1) was issued.
        // Therefore, we report back how many cycles we actually ran.
        step_result sr = advance(cycles);
        cycles += m_cycles - m_limit;
        return sr;
    }

    step_result or1k::run(unsigned int quantum)
    {
        step_result sr = STEP_OK;
        while (sr == STEP_OK)
            sr = advance(quantum);
        return sr;
    }

    void or1k::insert_breakpoint(u32 addr)
    {
        if (!stl_contains(m_breakpoints, addr))
            m_breakpoints.push_back(addr);
    }

    void or1k::remove_breakpoint(u32 addr)
    {
        if (stl_contains(m_breakpoints, addr))
            stl_remove_erase(m_breakpoints, addr);
    }

    void or1k::insert_watchpoint_r(u32 addr)
    {
        if (!stl_contains(m_watchpoints_r, addr))
            m_watchpoints_r.push_back(addr);
    }

    void or1k::remove_watchpoint_r(u32 addr)
    {
        if (stl_contains(m_watchpoints_r, addr))
            stl_remove_erase(m_watchpoints_r, addr);
    }

    void or1k::insert_watchpoint_w(u32 addr)
    {
        if (!stl_contains(m_watchpoints_w, addr))
            m_watchpoints_w.push_back(addr);
    }

    void or1k::remove_watchpoint_w(u32 addr)
    {
        if (stl_contains(m_watchpoints_w, addr))
            stl_remove_erase(m_watchpoints_w, addr);
    }

    void or1k::trace(std::ostream& os)
    {
        if (m_user_trace_stream != NULL)
            OR1KISS_ERROR("trace stream already specified");
        m_user_trace_stream = &os;
        m_trace_enabled = true;
    }

    void or1k::trace(const std::string& filename)
    {
        if (m_user_trace_stream != NULL)
            OR1KISS_ERROR("trace stream already specified");
        m_file_trace_stream = new std::ofstream(filename.c_str());
        m_user_trace_stream = m_file_trace_stream;
        m_trace_enabled = true;
    }

}
