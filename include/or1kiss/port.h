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

#ifndef OR1KISS_PORT_H
#define OR1KISS_PORT_H

#include "or1kiss/includes.h"
#include "or1kiss/types.h"
#include "or1kiss/utils.h"
#include "or1kiss/exception.h"
#include "or1kiss/bitops.h"
#include "or1kiss/endian.h"

namespace or1kiss {

    enum response {
        RESP_SUCCESS =  0, /* OK response */
        RESP_FAILED  =  1, /* access failed: not atomic */
        RESP_ERROR   = -1  /* access failed: bus error */
    };

    enum word_size {
        SIZE_BYTE       = 1 << 0,
        SIZE_HALFWORD   = 1 << 1,
        SIZE_WORD       = 1 << 2,
        SIZE_DOUBLEWORD = 1 << 3,
    };

    class request
    {
    private:
        bool m_read;
        bool m_imem;
        bool m_excl;
        bool m_super;
        bool m_debug;

        // Flags set by MMU
        bool m_cache_coherent;
        bool m_cache_inhibit;
        bool m_cache_writeback;
        bool m_weakly_ordered;

        endian m_endian;

    public:
        mutable u64 cycles;
        u32 addr;

        void* data;
        unsigned int size;

        inline bool is_read()  const { return m_read; }
        inline bool is_write() const { return !m_read; }

        inline bool is_imem() const { return m_imem; }
        inline bool is_dmem() const { return !m_imem; }

        inline bool is_exclusive()  const { return m_excl; }
        inline bool is_supervisor() const { return m_super; }
        inline bool is_debug()      const { return m_debug; }

        inline void set_read()  { m_read = true; }
        inline void set_write() { m_read = false; }

        inline void set_imem() { m_imem = true; }
        inline void set_dmem() { m_imem = false; }

        inline void set_exclusive(bool set = true)  { m_excl = set; }
        inline void set_supervisor(bool set = true) { m_super = set; }
        inline void set_debug(bool set = true)      { m_debug = set; }

        inline bool is_cache_coherent()  const { return m_cache_coherent; }
        inline bool is_cache_inhibit()   const { return m_cache_inhibit; }
        inline bool is_cache_writeback() const { return m_cache_writeback; }
        inline bool is_weakly_ordered()  const { return m_weakly_ordered; }

        inline void set_cache_coherent(bool set)  { m_cache_coherent = set; }
        inline void set_cache_inhibit(bool set)   { m_cache_inhibit = set; }
        inline void set_cache_writeback(bool set) { m_cache_writeback = set; }
        inline void set_weakly_ordered(bool set)  { m_weakly_ordered = set; }

        inline endian get_endian() const { return m_endian; }
        inline void set_endian(endian e) { m_endian = e; }

        inline void set_host_endian() { m_endian = host_endian(); }

        inline void set_little_endian() { m_endian = ENDIAN_LITTLE; }
        inline void set_big_endian()    { m_endian = ENDIAN_BIG; }

        inline bool is_little_endian() const {
            return m_endian == ENDIAN_LITTLE;
        }

        inline bool is_big_endian() const {
            return m_endian == ENDIAN_BIG;
        }

        inline bool is_aligned() const {
            return or1kiss::is_aligned(addr, size);
        }

        inline void set_addr_and_data(u32 _addr, void* _data,
                                      unsigned int _size) {
            addr = _addr;
            data = _data;
            size = _size;
        }

        template <typename T>
        inline void set_addr_and_data(u32 _addr, T& _data) {
            addr = _addr;
            data = &_data;
            size = sizeof(_data);
        }

        template <typename T>
        inline void set_data(u32 _addr, T* ptr) {
            addr = _addr;
            data = ptr;
            size = sizeof(T);
        }

        inline request();
        inline request(const request& other);
        virtual ~request() { /* Nothing to do */ };
    };

    inline request::request():
        m_read(false),
        m_imem(false),
        m_excl(false),
        m_super(false),
        m_debug(false),
        m_cache_coherent(false),
        m_cache_inhibit(false),
        m_cache_writeback(false),
        m_weakly_ordered(false),
        m_endian(host_endian()),
        cycles(0),
        addr(0),
        data(NULL),
        size(0){
        /* Nothing to do */
    }

    inline request::request(const request& other):
        m_read(other.m_read),
        m_imem(other.m_imem),
        m_excl(other.m_excl),
        m_super(other.m_super),
        m_debug(other.m_debug),
        m_cache_coherent(other.m_cache_coherent),
        m_cache_inhibit(other.m_cache_inhibit),
        m_cache_writeback(other.m_cache_writeback),
        m_weakly_ordered(other.m_weakly_ordered),
        m_endian(other.m_endian),
        cycles(other.cycles),
        addr(other.addr),
        data(other.data),
        size(other.size) {
        /* Nothing to do */
    }

    class port
    {
    private:
        endian         m_endian;

        unsigned char* m_data_ptr;
        u32       m_data_start;
        u32       m_data_end;
        u64       m_data_cycles;

        unsigned char* m_insn_ptr;
        u32       m_insn_start;
        u32       m_insn_end;
        u64       m_insn_cycles;

        u32       m_excl_addr;
        u32       m_excl_data;

        response exclusive_access(unsigned char* ptr, request& req);

        // Disabled
        port(const port&);

    public:
        inline endian get_system_endian() const {
            return m_endian;
        }

        inline void set_data_ptr(unsigned char* ptr,
                                 u32 addr_start = 0x00000000,
                                 u32 addr_end   = 0xffffffff,
                                 u64 cycles = 0);

        inline void set_insn_ptr(unsigned char* ptr,
                                 u32 addr_start = 0x00000000,
                                 u32 addr_end   = 0xffffffff,
                                 u64 cycles = 0);

        inline unsigned char* get_data_ptr(u32 addr) const;
        inline unsigned char* get_insn_ptr(u32 addr) const;

        inline unsigned char* direct_memory_ptr(request& req) const;

        /* Specify the endianess in which you want to receive data from the ISS
         * and you will return back to the ISS. Generally this will be the
         * native endianess of the system (big endian), the same as in the
         * binaries that you load. Since the ISS is working with host endian,
         * the port will handle all the necessary endianess conversion for you.
         */
        port(endian e);
        virtual ~port();

        virtual u64 sleep(u64 cycles) {
            return 0;
        }

        response convert_and_transact(request& req);

        template <typename T>
        inline bool read(u32 addr, T& val);

        template <typename T>
        inline bool read(u32 addr, T& val, u64& cycles);

        template <typename T>
        inline bool read_dbg(u32 addr, T& val);

        template <typename T>
        inline bool write(u32 addr, const T& val);

        template <typename T>
        inline bool write(u32 addr, const T& val, u64& cycles);

        template <typename T>
        inline bool write_dbg(u32 addr, const T& val);

    protected:
        virtual response transact(const request& req) = 0;
    };

    inline void port::set_data_ptr(unsigned char* ptr, u32 start,
                                   u32 end, u64 cycles) {
        if (start > end)
            OR1KISS_ERROR("invalid range specified %u..%u", start, end);
        m_data_ptr = ptr;
        m_data_start = start;
        m_data_end = end;
        m_data_cycles = cycles;
    }

    inline void port::set_insn_ptr(unsigned char* ptr, u32 start,
                                   u32 end, u64 cycles) {
        if (start > end)
            OR1KISS_ERROR("invalid range specified %u..%u", start, end);
        m_insn_ptr = ptr;
        m_insn_start = start;
        m_insn_end = end;
        m_insn_cycles = cycles;
    }

    inline unsigned char* port::get_data_ptr(u32 addr) const {
        if ((m_data_ptr == NULL) || (addr < m_data_start) || (addr > m_data_end))
            return NULL; // address outside bounds
        return m_data_ptr + addr - m_data_start;
    }

    inline unsigned char* port::get_insn_ptr(u32 addr) const {
        if ((m_insn_ptr == NULL) || (addr < m_insn_start) || (addr > m_insn_end))
            return NULL; // address outside bounds
        return m_insn_ptr + addr - m_insn_start;
    }

    inline unsigned char* port::direct_memory_ptr(request& req) const {
        if (req.is_dmem()) {
            req.cycles += m_data_cycles;
            return get_data_ptr(req.addr);
        } else {
            req.cycles += m_insn_cycles;
            return get_insn_ptr(req.addr);
        }
    }

    template <typename T>
    inline bool port::read(u32 addr, T& val) {
        u64 cycles = 0;
        return read(addr, val, cycles);
    }

    template <typename T>
    inline bool port::read(u32 addr, T& val, u64& cycles) {
        request req;
        req.set_dmem();
        req.set_read();
        req.addr = addr;
        req.data = &val;
        req.size = sizeof(val);
        req.cycles = 0;
        response rs = convert_and_transact(req);
        cycles += req.cycles;
        return rs == RESP_SUCCESS;
    }

    template <typename T>
    inline bool port::write(u32 addr, const T& val) {
        u64 cycles = 0;
        return write(addr, val, cycles);
    }

    template <typename T>
    inline bool port::read_dbg(u32 addr, T& val) {
        request req;
        req.set_dmem();
        req.set_read();
        req.set_debug();
        req.addr = addr;
        req.data = &val;
        req.size = sizeof(val);
        req.cycles = 0;
        response rs = convert_and_transact(req);
        return rs == RESP_SUCCESS;
    }

    template <typename T>
    inline bool port::write(u32 addr, const T& val, u64& cycles) {
        request req;
        req.set_dmem();
        req.set_write();
        req.addr = addr;
        req.data = &val;
        req.size = sizeof(val);
        req.cycles = 0;
        response rs = convert_and_transact(req);
        cycles += req.cycles;
        return rs == RESP_SUCCESS;
    }

    template <typename T>
    inline bool port::write_dbg(u32 addr, const T& val) {
        request req;
        req.set_dmem();
        req.set_write();
        req.set_debug();
        req.addr = addr;
        req.data = &val;
        req.size = sizeof(val);
        req.cycles = 0;
        response rs = convert_and_transact(req);
        return rs == RESP_SUCCESS;
    }

}

#endif
