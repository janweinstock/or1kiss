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

#include "or1kiss/env.h"

namespace or1kiss {

env::env(endian e):
    m_endian(e),
    m_data_ptr(NULL),
    m_data_start(0),
    m_data_end(0),
    m_data_cycles(0),
    m_insn_ptr(NULL),
    m_insn_start(0),
    m_insn_end(0),
    m_insn_cycles(0),
    m_excl_addr(-1),
    m_excl_data() {
    // nothing to do
}

env::~env() {
    // nothing to do
}

response env::exclusive_access(unsigned char* ptr, request& req) {
    if (req.is_read()) {
        m_excl_addr = req.addr;
        memcpy(req.data, ptr, req.size);
        memcpy(&m_excl_data, req.data, req.size);
    } else {
        if (req.addr != m_excl_addr)
            return RESP_FAILED;
        u32* val = (u32*)req.data;
        if (!cas(ptr, m_excl_data, *val))
            return RESP_FAILED;
    }

    return RESP_SUCCESS;
}

response env::convert_and_transact(request& req) {
    void* tmp = NULL;
    void* org = req.data;

    response resp = RESP_SUCCESS;

    bool conversion_necessary = (req.size > 1) &&
                                (req.get_endian() != m_endian);

    u64 buf = 0;

    // Convert host endian to simulation endian
    if (conversion_necessary) {
        tmp      = (req.size > sizeof(buf)) ? malloc(req.size) : &buf;
        req.data = memcpyswp(tmp, org, req.size);
    }

    // Send request to the simulation system
    unsigned char* ptr = direct_memory_ptr(req);
    if (ptr != NULL) {
        req.cycles += m_data_cycles;
        if (req.is_exclusive())
            resp = exclusive_access(ptr, req);
        else if (req.is_read())
            memcpy(req.data, ptr, req.size);
        else
            memcpy(ptr, req.data, req.size);
    } else {
        endian e = req.get_endian();
        req.set_endian(m_endian);
        resp = transact(req);
        req.set_endian(e);
    }

    // Convert simulation endian to host endian
    if (conversion_necessary) {
        req.data = memcpyswp(org, req.data, req.size);
        if (req.size > sizeof(buf))
            free(tmp);
    }

    return resp;
}

} // namespace or1kiss
