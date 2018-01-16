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

#include "or1kiss/elf.h"

namespace or1kiss {

    static bool elf_symbol_compare(const elf_symbol* a, const elf_symbol* b) {
        return a->get_phys_addr() < b->get_phys_addr();
    }

    elf_symbol::elf_symbol(const char* name, Elf32_Sym* symbol):
        m_virt_addr(symbol->st_value),
        m_phys_addr(symbol->st_value),
        m_name(name),
        m_type() {
        unsigned char type = ELF32_ST_TYPE(symbol->st_info);
        switch (type) {
        case STT_OBJECT : m_type = ELF_SYM_OBJECT;   break;
        case STT_FUNC   : m_type = ELF_SYM_FUNCTION; break;
        default         : m_type = ELF_SYM_UNKNOWN;  break;
        }
    }

    elf_symbol::~elf_symbol() {
        // Nothing to do
    }

    elf_section::elf_section(Elf* elf, Elf_Scn* scn):
        m_name(),
        m_data(),
        m_size(),
        m_virt_addr(),
        m_phys_addr(),
        m_flag_alloc(),
        m_flag_write(),
        m_flag_exec() {
        Elf32_Ehdr* ehdr = elf32_getehdr(elf);
        Elf32_Phdr* phdr = elf32_getphdr(elf);
        Elf32_Shdr* shdr = elf32_getshdr(scn);

        size_t shstrndx = 0;
        if (elf_getshdrstrndx (elf, &shstrndx) != 0)
            OR1KISS_ERROR("Call to elf_getshdrstrndx failed (%s)",
                                     elf_errmsg(-1));

        char* name = elf_strptr(elf, shstrndx, shdr->sh_name);
        if (name == NULL)
            OR1KISS_ERROR("Call to elfstrptr failed (%s)",
                                     elf_errmsg(-1));

        m_name = std::string(name);
        m_size = shdr->sh_size;
        m_data = new unsigned char[m_size];

        m_virt_addr = shdr->sh_addr;
        m_phys_addr = shdr->sh_addr;

        m_flag_alloc = shdr->sh_flags & SHF_ALLOC;
        m_flag_write = shdr->sh_flags & SHF_WRITE;
        m_flag_exec  = shdr->sh_flags & SHF_EXECINSTR;

        // Check program headers for physical address
        for (unsigned int i = 0; i < ehdr->e_phnum; i++) {
            u64 start = phdr[i].p_offset;
            u64 end   = phdr[i].p_offset + phdr[i].p_memsz;
            if ((start != 0) &&
                (shdr->sh_offset >= start) &&
                (shdr->sh_offset < end))
                m_phys_addr = phdr[i].p_paddr +
                              (shdr->sh_addr - phdr[i].p_vaddr);
        }

        Elf_Data* data = NULL;
        unsigned int copied = 0;
        while ((copied < shdr->sh_size) && (data = elf_getdata(scn, data))) {
            if (data->d_buf)
                std::memcpy(m_data + copied, data->d_buf, data->d_size);
        }
    }

    elf_section::~elf_section() {
        delete [] m_data;
    }

    void elf_section::load(port* p, bool verbose) {
        if (!m_flag_alloc)
            return;

        if (verbose)
            fprintf(stderr, "loading section '%s'... ", m_name.c_str());

        request req;
        req.set_write();
        req.set_debug();
        req.addr = m_phys_addr;
        req.data = m_data;
        req.size = m_size;

        if (m_flag_exec)
            req.set_imem();
        else
            req.set_dmem();

        req.set_big_endian();

        if (p->convert_and_transact(req) != RESP_SUCCESS) {
            fprintf(stderr, "warning: cannot load section '%s' to memory [0x%08" PRIx64 " - 0x%08" PRIx64 "]",
                    m_name.c_str(), m_phys_addr, m_phys_addr + m_size);
        }

        if (verbose) {
            fprintf(stderr, "OK [0x%08" PRIx64 "- 0x%08" PRIx64 "]\n",
                    m_phys_addr, m_phys_addr + m_size);
        }
    }

    elf::elf(const string& fname):
        m_filename(fname),
        m_endianess(),
        m_entry(),
        m_sections(),
        m_symbols() {
        if (elf_version(EV_CURRENT) == EV_NONE)
            OR1KISS_ERROR(elf_errmsg(-1));

        int fd = open(m_filename.c_str(), O_RDONLY, 0);
        if (fd < 0) {
            OR1KISS_ERROR(strerror(errno));
        }

        Elf* e = elf_begin(fd, ELF_C_READ, NULL);
        if (e == NULL)
            OR1KISS_ERROR(elf_errmsg(-1));

        if (elf_kind(e) != ELF_K_ELF) {
            OR1KISS_ERROR("File '%s' is not an ELF object\n", fname.c_str());
        }

        // Get executable header (will fail when a 64 bit binary is loaded)
        Elf32_Ehdr* ehdr = elf32_getehdr(e);
        if (ehdr == NULL)
            OR1KISS_ERROR(elf_errmsg(-1));

        switch (ehdr->e_ident[EI_DATA]) {
        case ELFDATA2LSB: m_endianess = ENDIAN_LITTLE; break;
        case ELFDATA2MSB: m_endianess = ENDIAN_BIG;    break;
        default:
            OR1KISS_ERROR("Invalid endianess specified in ELF header");
            break;
        }

        // Traverse all sections
        Elf_Scn* scn = NULL;
        while ((scn = elf_nextscn(e, scn)) != 0) {
            Elf32_Shdr* shdr = elf32_getshdr(scn);
            if (shdr == NULL)
                OR1KISS_ERROR(elf_errmsg(-1));

            // Section is symbol table, so try to get a list of symbols first
            if (shdr->sh_type == SHT_SYMTAB) {
                Elf_Data* data = elf_getdata(scn, NULL);
                unsigned int num_symbols = shdr->sh_size / shdr->sh_entsize;
                Elf32_Sym* symbols = reinterpret_cast<Elf32_Sym*>(data->d_buf);
                for (unsigned int i = 0; i < num_symbols; i++) {
                    char* name = elf_strptr(e, shdr->sh_link,
                                            symbols[i].st_name);
                    if (name == NULL)
                        OR1KISS_ERROR(elf_errmsg(-1));

                    elf_symbol* sym = new elf_symbol(name, symbols + i);
                    m_symbols.push_back(sym);
                }
            }

            // Add to our section list
            elf_section* sec = new elf_section(e, scn);
            m_sections.push_back(sec);
        }

        // Update physical addresses of symbols once all sections are loaded.
        for (unsigned int i = 0; i < m_symbols.size(); i++)
            m_symbols[i]->m_phys_addr = to_phys(m_symbols[i]->m_virt_addr);

        // Sort symbols for easier lookup
        std::sort(m_symbols.begin(), m_symbols.end(), &elf_symbol_compare);

        // Get program entry address and endianess from header
        m_entry = to_phys(ehdr->e_entry);

        // Close file and elf library
        elf_end(e);
        close(fd);
    }

    elf::~elf() {
        for (unsigned int i = 0; i < m_symbols.size(); i++)
            delete m_symbols[i];

        for (unsigned int i = 0; i < m_sections.size(); i++)
            delete m_sections[i];
    }

    u64 elf::to_phys(u64 virt_addr) const {
        for (unsigned int i = 0; i < m_sections.size(); i++) {
            elf_section* section = m_sections[i];
            if (section->contains(virt_addr))
                return section->to_phys(virt_addr);
        }

        return virt_addr;
    }

    void elf::load(port* p, bool verbose) {
        if (verbose)
            fprintf(stderr, "loading elf from '%s'\n", m_filename.c_str());

        for (unsigned int i = 0; i < m_sections.size(); i++)
            m_sections[i]->load(p, verbose);

        if (m_entry != 0x100)
            fprintf(stderr, "invalid entry point 0x%08" PRIx64 " ignored\n",
                    m_entry);

        if (verbose)
            fprintf(stderr, "loading elf done\n");
    }

    void elf::dump() {
        fprintf(stderr, "%s has %zd sections:\n", m_filename.c_str(),
                m_sections.size());

        static const char* endstr[] = { "little", "big", "unknown" };

        fprintf(stderr, "name     : %s\n", m_filename.c_str());
        fprintf(stderr, "entry    : 0x%08" PRIx64 "\n", m_entry);
        fprintf(stderr, "endian   : %s\n", endstr[m_endianess]);
        fprintf(stderr, "sections : %zd\n", m_sections.size());
        fprintf(stderr, "symbols  : %zd\n", m_symbols.size());

        fprintf(stderr, "\nsections:\n");
        fprintf(stderr, "[nr] vaddr      paddr      size       name\n");

        for (unsigned int i = 0; i < m_sections.size(); i++) {
            elf_section* sec = m_sections[i];
            fprintf(stderr, "[%2d] ", i);
            fprintf(stderr, "0x%08" PRIx64 " ", sec->get_virt_addr());
            fprintf(stderr, "0x%08" PRIx64 " ", sec->get_phys_addr());
            fprintf(stderr, "0x%08" PRIx32 " ", sec->get_size());
            fprintf(stderr, "%s\n", sec->get_name().c_str());
        }

        fprintf(stderr, "\nsymbols:\n");
        fprintf(stderr, "[   nr] vaddr      paddr      type     name\n");

        static const char* typestr[] = { "OBJECT  ", "FUNCTION", "UNKNOWN " };
        for (unsigned int i = 0; i < m_symbols.size(); i++) {
            elf_symbol* sym = m_symbols[i];
            if (sym->get_type() != ELF_SYM_FUNCTION)
                continue;

            fprintf(stderr, "[%5d] ", i);
            fprintf(stderr, "0x%08" PRIx64 " ", sym->get_virt_addr());
            fprintf(stderr, "0x%08" PRIx64 " ", sym->get_phys_addr());
            fprintf(stderr, "%s ", typestr[sym->get_type()]);
            fprintf(stderr, "%s\n", sym->get_name().c_str());
        }
    }

    elf_section* elf::find_section(const string& name) const {
        for (unsigned int i = 0; i < m_sections.size(); i++)
            if (m_sections[i]->get_name() == name)
                return m_sections[i];
        return NULL;
    }

    elf_section* elf::find_section(u64 virt_addr) const {
        for (unsigned int i = 0; i < m_sections.size(); i++)
            if (m_sections[i]->get_virt_addr() == virt_addr)
                return m_sections[i];
        return NULL;
    }

    elf_symbol* elf::find_symbol(const string& name) const {
        for (unsigned int i = 0; i < m_symbols.size(); i++)
            if (m_symbols[i]->get_name() == name)
                return m_symbols[i];
        return NULL;
    }

    elf_symbol* elf::find_symbol(u64 virt_addr) const {
        for (unsigned int i = 0; i < m_symbols.size(); i++)
            if (m_symbols[i]->get_virt_addr() == virt_addr)
                return m_symbols[i];
        return NULL;
    }

    elf_symbol* elf::find_function(u64 virt_addr) const {
        // Symbols are sorted by virtual address, so the last one with a
        // smaller address than what we are looking for is our match.
        elf_symbol* result = NULL;
        for (unsigned int i = 0; i < m_symbols.size(); i++) {
            if (!m_symbols[i]->is_function())
                continue;
            if (m_symbols[i]->get_virt_addr() > virt_addr)
                break;
            result = m_symbols[i];
        }

        return result;
    }

}
