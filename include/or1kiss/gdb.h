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

#ifndef OR1KISS_GDB_H
#define OR1KISS_GDB_H

#include "or1kiss/includes.h"
#include "or1kiss/types.h"
#include "or1kiss/or1k.h"
#include "or1kiss/elf.h"
#include "or1kiss/rsp.h"


#define OR1KISS_GDB_ERR_COMMAND  1 /* gdb rsp command was malformed */
#define OR1KISS_GDB_ERR_PARAM    2 /* command parameter has invalid value */
#define OR1KISS_GDB_ERR_INTERNAL 3 /* internal error during execution */
#define OR1KISS_GDB_ERR_UNKNOWN  4 /* used for all other kinds of errors */

#define OR1KISS_GDB_RDBUF_SIZE   (OR1KISS_RSP_MAX_PACKET_SIZE >> 2)

namespace or1kiss {

    class gdb;

    typedef void (gdb::*gdb_handler)(const char*);

    enum gdb_mode {
        GDB_MODE_HALTED,
        GDB_MODE_STEPPING,
        GDB_MODE_RUNNING,
        GDB_MODE_KILLED
    };

    class gdb
    {
    private:
        or1k&     m_iss;
        port*     m_port;
        elf*      m_elf;
        rsp       m_rsp;
        bool      m_detached;
        bool      m_show_warnings;
        gdb_mode  m_mode;
        pthread_t m_thread;

        std::map<u32, u32> m_tlb;
        std::map<char, gdb_handler> m_handler;

        void warning(const char* text, ...);

        void mem_read  (u32 phys_addr, void* ptr, unsigned int size);
        void mem_write (u32 phys_addr, void* ptr, unsigned int size);

        bool translate(u32& addr);
        u32 to_phys(u32 addr);

        void init();
        void process_commands();

        void handle_query(const char*);
        void handle_rcmd(const char*);
        void handle_step(const char*);
        void handle_continue(const char*);
        void handle_detach(const char*);
        void handle_kill(const char*);

        void handle_reg_read(const char*);
        void handle_reg_write(const char*);
        void handle_reg_read_all(const char*);
        void handle_reg_write_all(const char*);
        void handle_mem_read(const char*);
        void handle_mem_write(const char*);
        void handle_mem_write_bin(const char*);

        void handle_breakpoint_set(const char*);
        void handle_breakpoint_delete(const char*);

        void handle_exception(const char*);
        void handle_thread(const char*);
        void handle_vcont(const char*);

        void check_signals();

        // Disabled
        gdb();
        gdb(const gdb&);

    public:
        gdb(or1k& sim, unsigned short port);
        gdb(or1k& sim, elf* e, unsigned short port);
        virtual ~gdb();

        inline bool is_connected() const { return m_rsp.is_open(); }

        inline elf* get_elf()       { return m_elf; }
        inline void set_elf(elf* e) { m_elf = e; }

        inline void warnings(bool b) { m_show_warnings = b; }
        inline void show_warnings()  { m_show_warnings = true; }
        inline void hide_warnings()  { m_show_warnings = false; }

        inline gdb_mode get_mode() const { return m_mode; }

        inline       std::vector<u32>  get_breakpoints();
        inline       std::vector<u32>  get_watchpoints_r();
        inline       std::vector<u32>  get_watchpoints_w();

        void insert_breakpoint(u32);
        void remove_breakpoint(u32);
        void insert_watchpoint(u32);
        void remove_watchpoint(u32);

        step_result step(unsigned int& n);
        step_result run(unsigned int quantum = ~0u);

        void listen();
    };

    inline std::vector<u32> gdb::get_breakpoints() {
        return m_iss.get_breakpoints();
    }

    inline std::vector<u32> gdb::get_watchpoints_r() {
        return m_iss.get_watchpoints_r();
    }

    inline std::vector<u32> gdb::get_watchpoints_w() {
        return m_iss.get_watchpoints_w();
    }

}

#endif
