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
#include "or1kiss/mmu.h"

namespace or1kiss {

    static inline mmu_access access_mask(const request& req) {
        if (req.is_debug())
            return (mmu_access)(MMU_SRE | MMU_SWE | MMU_URE | MMU_UWE);
        if (req.is_imem())
            return (req.is_supervisor()) ? MMU_SXE : MMU_UXE;
        if (req.is_write())
            return (req.is_supervisor()) ? MMU_SWE : MMU_UWE;
        return (req.is_supervisor()) ? MMU_SRE : MMU_URE;
    }

    int mmu::find_empty_way(int set) const {
        // First we try to find a slot that is not in use.
        for (unsigned int way = 0; way < m_num_ways; way++) {
            u32 match = m_tlb[OR1KISS_TLB_MR(way, set)];
            if (!(match & MMUM_V))
                return way;
        }

        // If we did not find a free spot, replace the oldest entry
        u32 select = 0, oldest = 0;
        for (unsigned int way = 0; way < m_num_ways; way++) {
            u32 match = m_tlb[OR1KISS_TLB_MR(way, set)];
            u32 age = match & MMUM_LRU3;
            if (age >= oldest) {
                oldest = age;
                select = way;
            }
        }

        return select;
    }

    mmu::mmu(u32 config, env* e):
        m_cfg(config),
        m_ctrl(0),
        m_prot(0),
        m_num_sets(1 << bits32(config, 4, 2)),
        m_num_ways(1  + bits32(config, 1, 0)),
        m_set_mask(m_num_sets - 1),
        m_tlb(),
        m_env(e) {
        // Check that we have a busport if user wants hardware
        // TLB refill enabled
        if ((e == NULL) && (config & MMUCFG_HTR))
            OR1KISS_ERROR("Hardware TLB refill impossible, no memory access");
    }

    mmu::~mmu() {
        /* Nothing to do */
    }

    void mmu::set_cr(u32 val) {
        if ((m_cfg & MMUCFG_TEIRI) && ((val & MMUCR_DTF) || (val & MMUCR_ITF)))
            flush_tlb();

        m_ctrl = val & ~(MMUCR_DTF | MMUCR_ITF);
    }

    void mmu::set_pr(u32 val) {
        m_prot = val;
    }

    u32 mmu::get_atb(u32 reg) const {
        std::cout << "Warning (mmu): ATB not supported" << std::endl;
        return 0;
    }

    u32 mmu::get_tlb(u32 reg) const {
        u32 way = reg >> 8;
        u32 set = reg & 0x7f;

        if ((way >= m_num_ways) || (set >= m_num_sets))
            return 0;

        return m_tlb[reg];
    }

    void mmu::set_atb(u32 reg, u32 val) {
        std::cout << "Warning (mmu): ATB not supported" << std::endl;
    }

    void mmu::set_tlb(u32 reg, u32 val) {
        u32 way = reg >> 8;
        u32 set = reg & 0x7f;

        if ((way >= m_num_ways) || (set >= m_num_sets))
            return;

        m_tlb[reg] = val;
    }

    void mmu::flush_tlb() {
        std::memset(m_tlb, 0, sizeof(m_tlb));
    }

    void mmu::flush_tlb_entry(u32 ea) {
        u32 vpg = OR1KISS_PAGE_ALIGN(ea);
        u32 set = OR1KISS_PAGE_NUMBER(ea) & m_set_mask;

        for (unsigned int way = 0; way < m_num_ways; way++) {
            u32* match = m_tlb + OR1KISS_TLB_MR(way, set);
            if ((OR1KISS_PAGE_COMPARE(vpg, *match)))
                *match &= ~MMUM_V;
        }
    }

    mmu_result mmu::translate(request& req) {
        u32 vpg = OR1KISS_PAGE_ALIGN(req.addr);
        u32 set = OR1KISS_PAGE_NUMBER(req.addr) & m_set_mask;

        if (!req.is_debug()) { // Increase LRU counter for valid entries
            for (unsigned int way = 0; way < m_num_ways; way++) {
                u32* match = m_tlb + OR1KISS_TLB_MR(way, set);
                if (*match & MMUM_V) {
                    *match = (*match & ~MMUM_LRU3) |
                             ((*match & MMUM_LRU3) + MMUM_LRU1);
                }
            }
        }

        // Look for matching entry in TLB
        for (unsigned int way = 0; way < m_num_ways; way++) {
            u32* match = m_tlb + OR1KISS_TLB_MR(way, set);
            u32* trans = m_tlb + OR1KISS_TLB_TR(way, set);
            if ((*match & MMUM_V) && (OR1KISS_PAGE_COMPARE(vpg, *match))) {
                if (!req.is_debug()) {
                    // Check access rights
                    if (!(*trans & access_mask(req)))
                        return MMU_PAGE_FAULT;

                    // Update access_mask and dirty flags
                    *trans |= MMUPTE_A;
                    if (req.is_write())
                        *trans |= MMUPTE_D;

                    // Update LRU
                    *match &= ~MMUM_LRU3;
                }

                // Access rights okay, translate address and return
                u32 ppg = OR1KISS_PAGE_ALIGN(*trans);
                u32 off = OR1KISS_PAGE_OFFSET(req.addr);
                req.addr = ppg | off;

                // Sync request flags to page flags
                req.set_cache_coherent(*trans & MMUPTE_CC);
                req.set_cache_inhibit(*trans & MMUPTE_CI);
                req.set_cache_writeback(*trans & MMUPTE_WBC);
                req.set_weakly_ordered(*trans & MMUPTE_WOM);

                return MMU_OKAY;
            }
        }

        // Nothing found in TLB, if HW reload is disabled and we are not
        // doing a debug access, we have to stop now.
        if (!(m_cfg & MMUCFG_HTR) && !req.is_debug())
            return MMU_TLB_MISS;

        // Need to do a table walk
        u32 pte1, pte2;
        u32 pl1idx = bits32(req.addr, 31, 24);
        u32 pl2idx = bits32(req.addr, 23, 13);
        u32 page_directory = m_ctrl & MMUCR_PGD;

        // Check if a directory pointer is provided, if not fall back to
        // handling the TLB miss in software.
        if (page_directory == 0)
            return MMU_TLB_MISS;

        request mmureq(req);
        mmureq.set_host_endian();
        mmureq.set_dmem();
        mmureq.set_read();
        mmureq.cycles = 0;

        // Get the first page table entry from the L1 page directory. Its
        // base address is stored in the control register.
        mmureq.set_addr_and_data(page_directory + (pl1idx << 2), pte1);
        if (m_env->convert_and_transact(mmureq) != RESP_SUCCESS)
            return MMU_TLB_MISS;

        if (!pte1)
            return MMU_TLB_MISS; // MMU_PAGE_FAULT;

        u32 page_table = OR1KISS_PAGE_ALIGN(pte1);
        mmureq.set_addr_and_data(page_table + (pl2idx << 2), pte2);
        if (m_env->convert_and_transact(mmureq) != RESP_SUCCESS)
            return MMU_TLB_MISS;

        if (!pte2)
            return MMU_TLB_MISS; // MMU_PAGE_FAULT;

        // No need to check translation and put it into the TLB if we are
        // only doing a debug access.
        if (req.is_debug())
            return MMU_OKAY;

        // Need to put the entry also into TLB
        u32 match = vpg | MMUM_LRU0 | MMUM_V;
        u32 trans = pte2 | MMUPTE_CC;

        // Linux uses bit 10 to mark a page executable (see asm/pgtable.h). So
        // if this bit is set, we must set SXE and UXE accordingly.
        if ((req.is_imem()) && (pte2 & MMUPTE_EXEC))
            trans |= (MMU_SXE | MMU_UXE);

        // Check access rights
        if (!(trans & access_mask(req)))
            return MMU_PAGE_FAULT;

        // Update access_mask and dirty flags
        trans |= MMUPTE_A;
        if (req.is_write())
            trans |= MMUPTE_D;

        // Finish address translation
        u32 ppg = OR1KISS_PAGE_ALIGN(trans);
        u32 off = OR1KISS_PAGE_OFFSET(req.addr);
        req.addr = ppg | off;

        // Account for the extra lookup time
        req.cycles += mmureq.cycles;

        // Find an empty location and store in TLB
        int way = find_empty_way(set);
        m_tlb[OR1KISS_TLB_MR(way, set)] = match;
        m_tlb[OR1KISS_TLB_TR(way, set)] = trans;

        // Done!
        return MMU_OKAY;
    }

}
