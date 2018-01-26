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

#ifndef OR1KISS_ELF_H
#define OR1KISS_ELF_H

#include "or1kiss/includes.h"
#include "or1kiss/types.h"
#include "or1kiss/exception.h"
#include "or1kiss/env.h"

namespace or1kiss {

    class elf_symbol;
    class elf_section;
    class elf;

    enum elf_sym_type {
        ELF_SYM_OBJECT = 0,
        ELF_SYM_FUNCTION,
        ELF_SYM_UNKNOWN
    };

    class elf_symbol
    {
        friend class elf;
    private:
        u64    m_virt_addr;
        u64    m_phys_addr;
        string m_name;
        elf_sym_type m_type;

        // Disabled
        elf_symbol();
        elf_symbol(const elf_symbol&);

    public:
        inline u64 get_virt_addr() const { return m_virt_addr; }
        inline u64 get_phys_addr() const { return m_phys_addr; }

        inline const string& get_name() const { return m_name; }
        inline elf_sym_type  get_type() const { return m_type; }

        inline bool is_function() const { return m_type == ELF_SYM_FUNCTION; }
        inline bool is_object()   const { return m_type == ELF_SYM_OBJECT; }

        elf_symbol(const char* name, Elf32_Sym* symbol);
        virtual ~elf_symbol();
    };

    class elf_section
    {
    private:
        std::string    m_name;
        unsigned char* m_data;
        unsigned int   m_size;

        u64 m_virt_addr;
        u64 m_phys_addr;

        bool m_flag_alloc;
        bool m_flag_write;
        bool m_flag_exec;

        // Disable
        elf_section();
        elf_section(const elf_section&);

    public:
        inline bool needs_alloc   () const { return m_flag_alloc; }
        inline bool is_writeable  () const { return m_flag_write; }
        inline bool is_executable () const { return m_flag_exec;  }

        inline const std::string& get_name() const { return m_name; }
        inline void*              get_data() const { return m_data; }
        inline unsigned int       get_size() const { return m_size; }

        inline u64 get_virt_addr() const { return m_virt_addr; }
        inline u64 get_phys_addr() const { return m_phys_addr; }

        inline bool contains(u64 addr) const {
            return (addr >= m_virt_addr) && (addr < (m_virt_addr + m_size));
        }

        inline u64 offset(u64 addr) const {
            return addr - m_virt_addr;
        }

        inline u64 to_phys(u64 addr) const {
            return offset(addr) + m_phys_addr;
        }

        elf_section(Elf* elf, Elf_Scn* section);
        virtual ~elf_section();

        void load(env* e, bool verbose = false);
    };

    class elf
    {
    private:
        string m_filename;
        endian m_endianess;
        u64    m_entry;

        vector<elf_section*> m_sections;
        vector<elf_symbol*>  m_symbols;

        // Disabled
        elf();
        elf(const elf&);

    public:
        inline const string& get_filename() const { return m_filename; }

        inline endian get_endianess()   const { return m_endianess; }
        inline u64    get_entry_point() const { return m_entry; }

        inline size_t get_num_sections() const { return m_sections.size(); }
        inline size_t get_num_symbols()  const { return m_symbols.size(); }

        elf(const string& filename);
        virtual ~elf();

        u64 to_phys(u64 virt_addr) const;

        void load(env* e, bool verbose = false);

        void dump();

        elf_section* find_section(const string& name) const;
        elf_section* find_section(u64 virt_addr) const;

        elf_symbol* find_symbol(const string& name) const;
        elf_symbol* find_symbol(u64 virt_addr) const;

        elf_symbol* find_function(u64 virt_addr) const;

        inline const vector<elf_section*>& get_sections() const {
            return m_sections;
        }

        inline const vector<elf_symbol*>& get_symbols() const {
            return m_symbols;
        }

        inline vector<elf_symbol*> get_functions() const {
            vector<elf_symbol*> functions;
            for (unsigned int i = 0; i < m_symbols.size(); i++)
                if (m_symbols[i]->is_function())
                    functions.push_back(m_symbols[i]);
            return functions;
        }

        inline vector<elf_symbol*> get_objects() const {
            vector<elf_symbol*> objects;
            for (unsigned int i = 0; i < m_symbols.size(); i++)
                if (m_symbols[i]->is_object())
                    objects.push_back(m_symbols[i]);
            return objects;
        }
    };

}

#endif
