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

#include "memory.h"

memory::memory(uint64_t size):
    or1kiss::port(or1kiss::ENDIAN_BIG),
    m_size(size),
    m_memory(new unsigned char [size]()) {
    set_data_ptr(m_memory, 0, size - 1, 1);
    set_insn_ptr(m_memory, 0, size - 1, 0);
}

memory::~memory() {
    delete [] m_memory;
}

bool memory::load(const char* filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open())
        return false;

    uint64_t fsize = file.tellg();
    fsize = std::min(fsize, m_size);
    file.seekg(0, std::ios::beg);
    file.read((char*)m_memory, fsize);
    return true;
}

or1kiss::response memory::transact(const or1kiss::request& req) {
    if ((req.addr + req.size) > m_size) {
        if (!req.is_debug()) {
            fprintf(stderr,
                    "(memory) bus error at address 0x%08" PRIx32 "\n",
                    req.addr);
            fflush(stderr);
            std::exit(-1);
        }

        return or1kiss::RESP_ERROR;
    }

    if (req.is_write()) {
        switch (req.size) {
        case or1kiss::SIZE_BYTE:
            *(uint8_t*)(m_memory + req.addr) = *(uint8_t*)req.data;
            break;

        case or1kiss::SIZE_HALFWORD:
            *(uint16_t*)(m_memory + req.addr) = *(uint16_t*)req.data;
            break;

        case or1kiss::SIZE_WORD:
            *(uint32_t*)(m_memory + req.addr) = *(uint32_t*)req.data;
            break;

        default:
            std::memcpy(m_memory + req.addr, req.data, req.size);
            break;
        }
    } else {
        switch (req.size) {
        case or1kiss::SIZE_BYTE:
            *(uint8_t* )req.data = *(uint8_t* )(m_memory + req.addr);
            break;

        case or1kiss::SIZE_HALFWORD:
            *(uint16_t*)req.data = *(uint16_t*)(m_memory + req.addr);
            break;

        case or1kiss::SIZE_WORD:
            *(uint32_t*)req.data = *(uint32_t*)(m_memory + req.addr);
            break;

        default:
            std::memcpy(req.data, m_memory + req.addr, req.size);
            break;
        }
    }

    if (!req.is_debug())
        req.cycles = 1;
    return or1kiss::RESP_SUCCESS;
}
