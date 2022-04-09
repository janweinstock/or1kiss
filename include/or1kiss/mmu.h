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

#ifndef OR1KISS_MMU_H
#define OR1KISS_MMU_H

#include "or1kiss/includes.h"
#include "or1kiss/utils.h"
#include "or1kiss/exception.h"
#include "or1kiss/env.h"

#define OR1KISS_PAGE_BITS         (13)
#define OR1KISS_PAGE_SIZE         (1 << OR1KISS_PAGE_BITS)
#define OR1KISS_PAGE_MASK         (OR1KISS_PAGE_SIZE - 1)
#define OR1KISS_PAGE_NUMBER(addr) ((addr) >> OR1KISS_PAGE_BITS)
#define OR1KISS_PAGE_OFFSET(addr) ((addr)&OR1KISS_PAGE_MASK)
#define OR1KISS_PAGE_ALIGN(addr)  ((addr) & ~OR1KISS_PAGE_MASK)
#define OR1KISS_PAGE_BOUNDARY(addr) \
    (OR1KISS_PAGE_ALIGN((addr) + OR1KISS_PAGE_SIZE))
#define OR1KISS_PAGE_COMPARE(a, b) (!OR1KISS_PAGE_ALIGN((a) ^ (b)))
#define OR1KISS_MKADDR(pn, off)    (((pn) << OR1KISS_PAGE_BITS) | (off))

#define OR1KISS_TLB_MAX_WAYS (4)
#define OR1KISS_TLB_MAX_SETS (128)
#define OR1KISS_TLB_MAX_REGS (2 * OR1KISS_TLB_MAX_SETS * OR1KISS_TLB_MAX_WAYS)

#define OR1KISS_TLB_MR(way, set) ((way)*OR1KISS_TLB_MAX_SETS * 2 + set)
#define OR1KISS_TLB_TR(way, set) \
    (OR1KISS_TLB_MR(way, set) + OR1KISS_TLB_MAX_SETS)

namespace or1kiss {

enum mmu_result {
    MMU_OKAY       = 0, // Translation successful
    MMU_TLB_MISS   = 1, // TLB miss occurred
    MMU_PAGE_FAULT = 2, // Page fault occurred
};

enum mmu_config {
    MMUCFG_NTW1 = 0 << 0, // Number of ways
    MMUCFG_NTW2 = 1 << 0,
    MMUCFG_NTW3 = 2 << 0,
    MMUCFG_NTW4 = 3 << 0,

    MMUCFG_NTS1   = 0 << 2, // Number of entries per way
    MMUCFG_NTS2   = 1 << 2,
    MMUCFG_NTS4   = 2 << 2,
    MMUCFG_NTS8   = 3 << 2,
    MMUCFG_NTS16  = 4 << 2,
    MMUCFG_NTS32  = 5 << 2,
    MMUCFG_NTS64  = 6 << 2,
    MMUCFG_NTS128 = 7 << 2,

    MMUCFG_NAE0 = 0 << 5, // Number of ATB entries
    MMUCFG_NAE1 = 1 << 5,
    MMUCFG_NAE2 = 2 << 5,
    MMUCFG_NAE3 = 3 << 5,
    MMUCFG_NAE4 = 4 << 5,
    MMUCFG_NAE5 = 5 << 5,

    MMUCFG_CRI   = 1 << 8,  // Control register present
    MMUCFG_PRI   = 1 << 9,  // Protection register present
    MMUCFG_TEIRI = 1 << 10, // TLB entry invalidate register present
    MMUCFG_HTR   = 1 << 11, // Hardware TLB reload
};

enum mmu_control {
    MMUCR_DTF = 1 << 0,     // Data TLB flush
    MMUCR_ITF = 1 << 0,     // Instruction TLB flush
    MMUCR_PGD = 0xfffffc00, // Mask for the page directory pointer
};

enum mmu_pte {
    MMUPTE_CC   = 1 << 0, // Cache Coherency
    MMUPTE_CI   = 1 << 1, // Cache Inhibit
    MMUPTE_WBC  = 1 << 2, // Write-Back Cache
    MMUPTE_WOM  = 1 << 3, // Weakly-Ordered Memory
    MMUPTE_A    = 1 << 4, // Accessed
    MMUPTE_D    = 1 << 5, // Dirty
    MMUPTE_PPI1 = 1 << 6, // Page Protection Index
    MMUPTE_PPI2 = 2 << 6,
    MMUPTE_PPI3 = 3 << 6,
    MMUPTE_PPI4 = 4 << 6,
    MMUPTE_PPI5 = 5 << 6,
    MMUPTE_PPI6 = 6 << 6,
    MMUPTE_PPI7 = 7 << 6,
    MMUPTE_L    = 1 << 7,  // Last/Linked
    MMUPTE_EXEC = 1 << 10, // not in spec, but enforced by linux
};

enum mmu_access {
    MMU_URE = 1 << 6, // User-mode read enable
    MMU_UWE = 1 << 7, // User-mode write enable
    MMU_UXE = 1 << 7, // User-mode execute enable
    MMU_SRE = 1 << 8, // Supervisor-mode read enable
    MMU_SWE = 1 << 9, // Supervisor-mode write enable
    MMU_SXE = 1 << 6, // Supervisor-mode execute enable
};

enum mmu_match {
    MMUM_V    = 1 << 0,  // Valid
    MMUM_PL1  = 1 << 1,  // Page Level 1
    MMUM_CID  = 15 << 2, // Context ID
    MMUM_LRU0 = 0 << 6,  // Most recently used
    MMUM_LRU1 = 1 << 6,
    MMUM_LRU2 = 2 << 6,
    MMUM_LRU3 = 3 << 6, // Least recently used
};

class mmu
{
private:
    u32 m_cfg;
    u32 m_ctrl;
    u32 m_prot;
    u32 m_num_sets;
    u32 m_num_ways;
    u32 m_set_mask;
    u32 m_tlb[OR1KISS_TLB_MAX_REGS];
    env* m_env;

    int find_empty_way(int set) const;

public:
    mmu(u32, env*);
    virtual ~mmu();

    mmu()           = delete;
    mmu(const mmu&) = delete;

    u32 get_num_ways() const { return m_num_ways; }
    u32 get_num_sets() const { return m_num_sets; }

    u32 get_cfgr() const { return m_cfg; }
    u32 get_cr() const { return m_ctrl; }
    u32 get_pr() const { return m_prot; }

    void set_cr(u32 val);
    void set_pr(u32 val);

    u32 get_atb(u32 regno) const;
    u32 get_tlb(u32 regno) const;

    void set_atb(u32 regno, u32 val);
    void set_tlb(u32 regno, u32 val);

    void flush_tlb();
    void flush_tlb_entry(u32 idx);

    mmu_result translate(request& req);
};

} // namespace or1kiss

#endif
