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

#include "or1kiss/gdb.h"

namespace or1kiss {

    static void* gdb_thread_func(void* arg) {
        gdb* handle = (gdb*)arg;
        handle->listen();
        return NULL;
    }

    void gdb::warning(const char* text, ...) {
        if (!m_show_warnings)
            return;

        char buffer[80];
        va_list ap;
        va_start(ap, text);
        vsnprintf(buffer, sizeof(buffer), text, ap);
        va_end(ap);

        fprintf(stderr, "(gdb) warning: %s\n", buffer);
        fflush(stderr);
    }

    void gdb::mem_read(u32 phys_addr, void* ptr, unsigned int size) {
        request req;
        req.set_dmem();
        req.set_read();
        req.set_debug();
        req.set_big_endian();
        req.addr = phys_addr;
        req.data = ptr;
        req.size = size;

        if (m_port->convert_and_transact(req) != RESP_SUCCESS)
            OR1KISS_ERROR("cannot read memory at 0x%08"PRIx32" (%d bytes)",
                          phys_addr, size);
    }

    void gdb::mem_write(u32 phys_addr, void* ptr, unsigned int size) {
        request req;
        req.set_dmem();
        req.set_write();
        req.set_debug();
        req.set_big_endian();
        req.set_addr_and_data(phys_addr, ptr, size);

        if (m_port->convert_and_transact(req) != RESP_SUCCESS)
            OR1KISS_ERROR("cannot write memory at 0x%08"PRIx32" (%d bytes)",
                          phys_addr, size);
    }

    bool gdb::translate(u32& addr) {
        // Nothing to do, if no MMUs are active
        if (!m_iss.is_dmmu_active() && !m_iss.is_immu_active()) {
            if (m_elf && addr >= 0xc0000000) // apply ELF section mapping
                addr = m_elf->to_phys(addr);
            return true;
        }

        request req;
        req.set_dmem();
        req.set_read();
        req.set_debug();
        req.addr = addr;

        // Translate using MMU
        if (m_iss.is_dmmu_active() &&
            m_iss.get_dmmu()->translate(req) == MMU_OKAY) {
            addr = req.addr;
            return true;
        }

        if (m_iss.is_immu_active() &&
            m_iss.get_immu()->translate(req) == MMU_OKAY) {
            addr = req.addr;
            return true;
        }

        // If the MMUs failed to translate for us, it means that the page
        // our data resides on has been swapped. We could let the ISS execute
        // a page fault handler, but that would also alter simulation state...
#if 0
        u32 pc = m_iss.get_spr(SPR_NPC, true);
        m_iss.trigger_tlb_miss(addr);
        do {
            m_iss.step(1);
        } while (m_iss.get_spr(SPR_NPC, true) != pc);

        if (m_iss.get_dmmu()->translate(req) == MMU_OKAY) {
            addr = req.addr;
            return true;
        }
#endif
        return false;
    }

    u32 gdb::to_phys(u32 addr) {
        u32 off = OR1KISS_PAGE_OFFSET(addr);
        u32 vpn = OR1KISS_PAGE_NUMBER(addr);
        u32 ppn = m_tlb[vpn];

        // Check if we already translated the address
        if (ppn)
            return OR1KISS_MKADDR(ppn, off);

        // Translate address and cache result
        if (!translate(addr)) {
            OR1KISS_ERROR("translation of address 0x%08"PRIx32" failed", addr);
            return addr;
        }

        m_tlb[vpn] = OR1KISS_PAGE_NUMBER(addr);
        return addr;
    }

    void gdb::init() {
        m_handler['q'] = &gdb::handle_query;

        m_handler['s'] = &gdb::handle_step;
        m_handler['c'] = &gdb::handle_continue;
        m_handler['D'] = &gdb::handle_detach;
        m_handler['k'] = &gdb::handle_kill;

        m_handler['p'] = &gdb::handle_reg_read;
        m_handler['P'] = &gdb::handle_reg_write;
        m_handler['g'] = &gdb::handle_reg_read_all;
        m_handler['G'] = &gdb::handle_reg_write_all;

        m_handler['m'] = &gdb::handle_mem_read;
        m_handler['M'] = &gdb::handle_mem_write;
        m_handler['X'] = &gdb::handle_mem_write_bin;

        m_handler['Z'] = &gdb::handle_breakpoint_set;
        m_handler['z'] = &gdb::handle_breakpoint_delete;

        m_handler['H'] = &gdb::handle_thread;
        m_handler['v'] = &gdb::handle_vcont;
        m_handler['?'] = &gdb::handle_exception;

        if (pthread_create(&m_thread, NULL, &gdb_thread_func, this))
            OR1KISS_ERROR("failed to spawn gdb thread");
    }

    void gdb::process_commands() {
        while (m_mode == GDB_MODE_HALTED) {
            std::string command = m_rsp.recv();
            if (!m_rsp.is_open()) {
                handle_detach(NULL);
                return;
            }

            gdb_handler handler = m_handler[command[0]];
            if (handler == NULL) {
                warning("command '%s' ignored\n", command.c_str());
                m_rsp.send(""); // empty packet means 'command not supported'
            } else try {
                (this->*handler)(command.c_str());
            } catch (exception& ex) {
                warning(ex.get_msg());
                m_rsp.send("E%02x", OR1KISS_GDB_ERR_INTERNAL);
            }
        }
    }

    void gdb::handle_step(const char* command) {
        m_tlb.clear();
        m_mode = GDB_MODE_STEPPING;
    }

    void gdb::handle_continue(const char* command) {
        m_tlb.clear();
        m_mode = GDB_MODE_RUNNING;
    }

    void gdb::handle_detach(const char* command) {
        m_tlb.clear();
        m_mode = GDB_MODE_RUNNING;
        m_detached = true;
        m_rsp.send("OK");
    }

    void gdb::handle_kill(const char* command) {
        m_mode = GDB_MODE_KILLED;
    }

    void gdb::handle_query(const char* command) {
        if (strncmp(command, "qSupported", strlen("qSupported")) == 0)
            m_rsp.send("PacketSize=%x", OR1KISS_RSP_MAX_PACKET_SIZE);
        else if (strncmp(command, "qAttached", strlen("qAttached")) == 0)
            m_rsp.send("1");
        else if (strncmp(command, "qOffsets", strlen("qOffsets")) == 0)
            m_rsp.send("Text=0;Data=0;Bss=0");
        else if (strncmp(command, "qRcmd", strlen("qRcmd")) == 0)
            handle_rcmd(command);
        else
            m_rsp.send("");
    }

    void gdb::handle_rcmd(const char* command) {
        const char* cmd = command + strlen("qRcmd,");
        int len = strlen(cmd);
        if (len & 0x1)
            OR1KISS_ERROR("RCMD length not a multiple of 2 (%d)", len);

        int len2 = len / 2;
        char s[len2 + 1];
        for (int i = 0; i < len2; i++)
            s[i] = (char2int(cmd[2 * i]) << 4) | char2int(cmd[2 * i + 1]);
        s[len2] = '\0';

        u32 reg;
        if (sscanf(s, "readspr %4x", &reg) == 1) {
            u32 val = m_iss.get_spr(reg, true);
            char buf[9]; char resp[17];
            sprintf(buf, "%8x", val);
            for (int i = 0; buf[i] != '\0'; i++) {
                resp[2 * i + 0] = int2char(buf[i] >> 4);
                resp[2 * i + 1] = int2char(buf[i] & 0xf);
            }
            resp[16] = 0;
            m_rsp.send(resp);
            return;
        }

        u32 val;
        if (sscanf(s, "writespr %4x %8x", &reg, &val) == 2) {
            m_iss.set_spr(reg, val, true);
            m_rsp.send("OK");
            return;
        }

        OR1KISS_ERROR("unknown remote command '%s'", s);
    }

    void gdb::handle_reg_read(const char* command) {
        unsigned int reg, val;
        if (sscanf(command, "p%x", &reg) != 1)
            OR1KISS_ERROR("error parsing command '%s'\n", command);

        if (reg > 34)
            OR1KISS_ERROR("register index %d out of range\n", reg);

        switch (reg) {
        case 32: val = m_iss.get_spr(SPR_PPC, true); break;
        case 33: val = m_iss.get_spr(SPR_NPC, true); break;
        case 34: val = m_iss.get_spr(SPR_SR,  true); break;
        default: val = m_iss.GPR[reg]; break;
        }

        m_rsp.send("%08x", val);
    }

    void gdb::handle_reg_write(const char* command) {
        u32 reg, val;
        if (sscanf(command, "P%x=%x", &reg, &val) != 2)
            OR1KISS_ERROR("error parsing command '%s'\n", command);

        if (reg > 34)
            OR1KISS_ERROR("register index %d out of range\n", reg);

        switch (reg) {
        case 32: m_iss.set_spr(SPR_PPC, val, true); break;
        case 33: m_iss.set_spr(SPR_NPC, val, true); break;
        case 34: m_iss.set_spr(SPR_SR,  val, true); break;
        default: m_iss.GPR[reg] = val; break;
        }

        m_rsp.send("OK");
    }

    void gdb::handle_reg_read_all(const char* command) {
        std::stringstream ss;
        ss << std::hex << std::setfill('0');

        for (unsigned int i = 0; i < 32; i++)
            ss << std::setw(8) << m_iss.GPR[i];

        ss << std::setw(8) << m_iss.get_spr(SPR_PPC, true);
        ss << std::setw(8) << m_iss.get_spr(SPR_NPC, true);
        ss << std::setw(8) << m_iss.get_spr(SPR_SR,  true);

        m_rsp.send(ss.str().c_str());
    }

    void gdb::handle_reg_write_all(const char* command) {
        const char* str = command + 1;
        for (unsigned int i = 0; i < 32; i++)
            m_iss.GPR[i] = str2int(str + i * 8, 8);

        m_iss.set_spr(SPR_PPC, str2int(str + 32 * 8, 8), true);
        m_iss.set_spr(SPR_NPC, str2int(str + 33 * 8, 8), true);
        m_iss.set_spr(SPR_SR,  str2int(str + 34 * 8, 8), true);

        m_rsp.send("OK");
    }

    void gdb::handle_mem_read(const char* command) {
        unsigned int addr, length;
        if (sscanf(command, "m%x,%x", &addr, &length) != 2)
            OR1KISS_ERROR("error parsing command '%s'\n", command);

        std::stringstream ss;
        ss << std::hex << std::setfill('0');

        u8 buffer[OR1KISS_GDB_RDBUF_SIZE];
        while (length > 0) {
            // Translate address
            u32 phys_addr = to_phys(addr);

            u32 num_bytes = length;
            if (num_bytes > OR1KISS_GDB_RDBUF_SIZE)
                num_bytes = OR1KISS_GDB_RDBUF_SIZE;

            // Make sure we do not cross read into another page frame
            u32 page_remaining = OR1KISS_PAGE_SIZE -
                                      OR1KISS_PAGE_OFFSET(phys_addr);
            if (num_bytes > page_remaining)
                num_bytes = page_remaining;

            // Perform reading, then update address and length
            mem_read(phys_addr, buffer, num_bytes);

            addr += num_bytes;
            length -= num_bytes;

            // Append values to string and convert endianess
            for (unsigned int i = 0; i < num_bytes; i++)
                ss << std::setw(2) << (int)buffer[i];
        }

        // Send response string
        m_rsp.send(ss.str());
    }

    void gdb::handle_mem_write(const char* command) {
        unsigned int addr, length;
        if (sscanf(command, "M%x,%x", &addr, &length) != 2)
            OR1KISS_ERROR("error parsing command '%s'\n", command);

        if (length % 4)
            OR1KISS_ERROR("cannot handle length argument: %d", length);

        // Find end of command prologue
        const char* data = strchr(command, ':');
        if (data == NULL)
            OR1KISS_ERROR("error parsing command '%s'\n", command);

        data++; // Let data point to the first data element.

        u8 buffer[OR1KISS_GDB_RDBUF_SIZE];
        while (length > 0) {
            // Translate address
            u32 phys_addr = to_phys(addr);

            u32 num_bytes = length;
            if (num_bytes > OR1KISS_GDB_RDBUF_SIZE)
                num_bytes = OR1KISS_GDB_RDBUF_SIZE;

            // Make sure we do not cross read into another page frame
            u32 page_remaining = OR1KISS_PAGE_SIZE -
                                      OR1KISS_PAGE_OFFSET(phys_addr);
            if (num_bytes > page_remaining)
                num_bytes = page_remaining;

            // Convert byte values, respect endianess
            for (unsigned int i = 0; i < num_bytes; i++)
                buffer[i] = str2int(data++, 2);

            // Write buffer to memory
            mem_write(phys_addr, buffer, num_bytes);

            addr += num_bytes;
            length -= num_bytes;
        }

        // Send response
        m_rsp.send("OK");
    }

    void gdb::handle_mem_write_bin(const char* command) {
        unsigned int addr, length;
        if (sscanf(command, "X%x,%x:", &addr, &length) != 2)
            OR1KISS_ERROR("error parsing command '%s'\n", command);

        if (length % 4)
            OR1KISS_ERROR("cannot handle length argument: %d", length);

        // Empty load to test if binary write is supported.
        if (length == 0) {
            m_rsp.send("OK");
            return;
        }

        // Find the end of the command prologue.
        const char* data = strchr(command, ':');
        if (data == NULL)
            OR1KISS_ERROR("error parsing command '%s'\n", command);

        data++; // Let data point to the first data element.

        u8 buffer[OR1KISS_GDB_RDBUF_SIZE];
        while (length > 0) {
            // Translate address
            u32 phys_addr = to_phys(addr);

            u32 num_bytes = length;
            if (num_bytes > OR1KISS_GDB_RDBUF_SIZE)
                num_bytes = OR1KISS_GDB_RDBUF_SIZE;

            // Make sure we do not cross read into another page frame
            u32 page_remaining = OR1KISS_PAGE_SIZE -
                                 OR1KISS_PAGE_OFFSET(phys_addr);
            if (num_bytes > page_remaining)
                num_bytes = page_remaining;

            // Convert byte values
            for (unsigned int i = 0; i < num_bytes; i++)
                buffer[i] = char_unescape(data);

            // Write buffer to memory
            mem_write(phys_addr, buffer, num_bytes);

            addr += num_bytes;
            length -= num_bytes;
        }

        // Send response
        m_rsp.send("OK");
    }

    void gdb::handle_breakpoint_set(const char* command) {
        unsigned int type, addr, length;
        if (sscanf(command, "Z%x,%x,%x", &type, &addr, &length) != 3)
            OR1KISS_ERROR("error parsing command '%s'\n", command);

        switch (type) {
        case 0: insert_breakpoint(addr); break; // software breakpoint
        case 1: insert_breakpoint(addr); break; // hardware breakpoint
        case 2: insert_watchpoint(addr); break; // write watchpoint
        case 3: insert_watchpoint(addr); break; // read watchpoint
        case 4: insert_watchpoint(addr); break; // access watchpoint
        default:
            OR1KISS_ERROR("invalid breakpoint type %d\n", type);
            return;
        }

        m_rsp.send("OK");
    }

    void gdb::handle_breakpoint_delete(const char* command) {
        unsigned int type, addr, length;
        if (sscanf(command, "z%x,%x,%x", &type, &addr, &length) != 3)
            OR1KISS_ERROR("error parsing command '%s'\n", command);

        switch (type) {
        case 0: remove_breakpoint(addr); break; // software breakpoint
        case 1: remove_breakpoint(addr); break; // hardware breakpoint
        case 2: remove_watchpoint(addr); break; // write watchpoint
        case 3: remove_watchpoint(addr); break; // read watchpoint
        case 4: remove_watchpoint(addr); break; // access watchpoint
        default:
            OR1KISS_ERROR("invalid breakpoint type %d\n", type);
            return;
        }

        m_rsp.send("OK");
    }

    void gdb::handle_exception(const char* command) {
        // Currently always S05 (Trap Exception)
        m_rsp.send("S%02u", SIGTRAP);
    }

    void gdb::handle_thread(const char* command) {
        // Currently no difference if step/continue or other future
        // operations apply to all threads.
        m_rsp.send("OK");
    }

    void gdb::handle_vcont(const char* command) {
        // vcont is not supported
        m_rsp.send("");
    }

    void gdb::check_signals() {
        if (!m_rsp.peek())
            return;

        char c = m_rsp.recv_char();
        switch (c) {
        case 0x00: // Terminate request
            m_mode = GDB_MODE_KILLED;
            break;

        case 0x03: // Interrupt request
            m_mode = GDB_MODE_HALTED;
            m_rsp.send("S%02u", SIGINT);
            break;
        }
    }

    gdb::gdb(or1k& iss, unsigned short port):
        m_iss(iss),
        m_port(iss.get_port()),
        m_elf(NULL),
        m_rsp(port),
        m_detached(false),
        m_show_warnings(false),
        m_mode(GDB_MODE_HALTED),
        m_thread(NULL),
        m_tlb(),
        m_handler() {
        init();
    }

    gdb::gdb(or1k& iss, elf* e, unsigned short port):
        m_iss(iss),
        m_port(iss.get_port()),
        m_elf(e),
        m_rsp(port),
        m_detached(false),
        m_show_warnings(false),
        m_mode(GDB_MODE_HALTED),
        m_thread(NULL),
        m_tlb(),
        m_handler() {
        init();
    }

    gdb::~gdb() {
        /* Nothing to do */
    }

    void gdb::insert_breakpoint(u32 addr) {
        m_iss.insert_breakpoint(addr);
    }

    void gdb::remove_breakpoint(u32 addr) {
        m_iss.remove_breakpoint(addr);
    }

    void gdb::insert_watchpoint(u32 addr) {
        m_iss.insert_watchpoint_r(addr);
        m_iss.insert_watchpoint_w(addr);
    }

    void gdb::remove_watchpoint(u32 addr) {
        m_iss.remove_watchpoint_r(addr);
        m_iss.remove_watchpoint_w(addr);
    }

    step_result gdb::step(unsigned int& cycles) {
        gdb_mode mode = m_mode;
        if (mode == GDB_MODE_KILLED)
            return m_iss.step(cycles);

        if (mode == GDB_MODE_STEPPING)
            cycles = 1;
        if (mode == GDB_MODE_HALTED)
            cycles = 0;

        switch (m_iss.step(cycles)) {
        case STEP_EXIT:
            m_rsp.send("W%02x", m_iss.GPR[3]);
            return STEP_EXIT;

        case STEP_BREAKPOINT:
            m_mode = GDB_MODE_HALTED;
            m_rsp.send("S%02u", SIGTRAP);
            break;

        case STEP_OK:
            if (mode == GDB_MODE_STEPPING) {
                m_mode = GDB_MODE_HALTED;
                m_rsp.send("S%02u", SIGTRAP);
            }
            break;

        default:
            assert(0);
            break;
        }

        return STEP_OK;
    }

    step_result gdb::run(unsigned int quantum) {
        step_result sr = STEP_OK;
        unsigned int steps;
        while (sr == STEP_OK) {
            steps = quantum;
            sr = step(steps);
        }

        return sr;
    }

    void gdb::listen() {
        while (true) {
            switch (m_mode) {
            case GDB_MODE_RUNNING:
                check_signals();
                break;

            case GDB_MODE_STEPPING:
            case GDB_MODE_HALTED:
                process_commands();
                break;

            case GDB_MODE_KILLED:
                return;

            default:
                assert(0);
            }
        }
    }

}
