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

#ifndef OR1KISS_ENDIAN_H
#define OR1KISS_ENDIAN_H

#include "or1kiss/includes.h"
#include "or1kiss/types.h"

namespace or1kiss {

enum endian { ENDIAN_LITTLE = 0, ENDIAN_BIG, ENDIAN_UNKNOWN };

inline endian host_endian() {
    u32 test = 1;
    u8* p    = reinterpret_cast<u8*>(&test);
    if (p[0] == 1)
        return ENDIAN_LITTLE;
    if (p[3] == 1)
        return ENDIAN_BIG;
    return ENDIAN_UNKNOWN;
}

// byte swapping used for endian conversion
template <typename T>
inline T byte_swap(const T&);

template <>
inline u16 byte_swap<u16>(const u16& x) {
    return __bswap_16(x);
}

template <>
inline u32 byte_swap<u32>(const u32& x) {
    return __bswap_32(x);
}

template <>
inline u64 byte_swap<u64>(const u64& x) {
    return __bswap_64(x);
}

// copy memory and swap bytes
inline void* memcpyswp(void* to, const void* from, unsigned int size) {
    switch (size) {
    case 1:
        *(u8*)to = *(u8*)from;
        break;
    case 2:
        *(u16*)to = byte_swap(*(u16*)from);
        break;
    case 4:
        *(u32*)to = byte_swap(*(u32*)from);
        break;
    case 8:
        *(u64*)to = byte_swap(*(u64*)from);
        break;
    default: {
        u32* src = (u32*)from;
        u32* dst = (u32*)to;
        while (size > 3) {
            *dst++ = byte_swap(*src++);
            size -= 4;
        }
    }
    }

    return to;
}

} // namespace or1kiss

#endif
