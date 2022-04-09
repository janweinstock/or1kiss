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

#ifndef MEMORY_H
#define MEMORY_H

#include <cstdlib>
#include <cstdio>

#include "or1kiss.h"

class memory : public or1kiss::env
{
private:
    uint64_t m_size;
    unsigned char* m_memory;

    // Disabled
    memory();
    memory(const memory&);

public:
    memory(uint64_t size);
    virtual ~memory();

    inline unsigned char* get_ptr() const { return m_memory; }
    inline uint64_t get_size() const { return m_size; }

    bool load(const char*);

    virtual or1kiss::response transact(const or1kiss::request& reg);
};

#endif
