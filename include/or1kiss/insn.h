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

#ifndef OR1KISS_INSN_H
#define OR1KISS_INSN_H

#include "or1kiss/includes.h"
#include "or1kiss/types.h"
#include "or1kiss/exception.h"

namespace or1kiss {

    class or1k;

    typedef void (or1k::*execute_function)(struct _instruction*);
    typedef void (or1k::*decode_function)(struct _instruction*);

    typedef struct _instruction {
        execute_function exec;
        u32  insn;
        u32  addr;
        u32* dest;
        u32* src1;
        u32* src2;
        u32  imm;
    } instruction;

    enum decode_cache_size {
        DECODE_CACHE_OFF       =  0, /* Decode cache disabled */
        DECODE_CACHE_SIZE_1K   = 10, /* Cache holding   1k entries */
        DECODE_CACHE_SIZE_2K   = 11, /* Cache holding   2k entries */
        DECODE_CACHE_SIZE_4K   = 12, /* Cache holding   4k entries */
        DECODE_CACHE_SIZE_8K   = 13, /* Cache holding   8k entries */
        DECODE_CACHE_SIZE_16K  = 14, /* Cache holding  16k entries */
        DECODE_CACHE_SIZE_32K  = 15, /* Cache holding  32k entries */
        DECODE_CACHE_SIZE_64K  = 16, /* Cache holding  64k entries */
        DECODE_CACHE_SIZE_128K = 17, /* Cache holding 128k entries */
        DECODE_CACHE_SIZE_256K = 18, /* Cache holding 256k entries */
        DECODE_CACHE_SIZE_512K = 19, /* Cache holding 512k entries */
        DECODE_CACHE_SIZE_1M   = 20, /* Cache holding   1M entries */
        DECODE_CACHE_SIZE_2M   = 21, /* Cache holding   2M entries */
        DECODE_CACHE_SIZE_4M   = 22, /* Cache holding   4M entries */
        DECODE_CACHE_SIZE_8M   = 23, /* Cache holding   8M entries */
        DECODE_CACHE_SIZE_16M  = 24, /* Cache holding  16M entries */
        DECODE_CACHE_SIZE_32M  = 25, /* Cache holding  32M entries */
        DECODE_CACHE_SIZE_64M  = 26, /* Cache holding  64M entries */
        DECODE_CACHE_SIZE_128M = 27, /* Cache holding 128M entries */
        DECODE_CACHE_SIZE_256M = 28  /* Cache holding 256M entries */
    };

    class decode_cache
    {
    private:
        decode_cache_size m_size;
        unsigned int      m_mask;
        unsigned int      m_count;
        instruction*      m_cache;

    public:
        inline decode_cache_size get_size() const { return m_size; }
        inline bool is_enabled() const { return m_size == DECODE_CACHE_OFF; }

        decode_cache(decode_cache_size size);
        virtual ~decode_cache();

        inline instruction& lookup(u32 addr);

        inline void invalidate(u32 addr);
        inline void invalidate_block(u32 addr, u32 size);
        inline void invalidate_all();
    };

    inline instruction& decode_cache::lookup(u32 addr) {
        return m_cache[(addr >> 2) & m_mask];
    }

    inline void decode_cache::invalidate(u32 addr) {
        instruction& insn = lookup(addr);
        if (insn.addr == addr)
            insn.addr = ~0;
    }

    inline void decode_cache::invalidate_block(u32 addr, u32 size) {
        for (unsigned int off = 0; off < size; off += 4)
            invalidate(addr + off);
    }

    inline void decode_cache::invalidate_all() {
        memset(m_cache, 0xff, m_count * sizeof(instruction));
    }

}

#endif
