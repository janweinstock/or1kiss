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

#ifndef OR1KISS_TYPES_H
#define OR1KISS_TYPES_H

#include "or1kiss/includes.h"

namespace or1kiss {

    typedef std::int8_t  s8;
    typedef std::int16_t s16;
    typedef std::int32_t s32;
    typedef std::int64_t s64;

    typedef std::uint8_t  u8;
    typedef std::uint16_t u16;
    typedef std::uint32_t u32;
    typedef std::uint64_t u64;

    using std::min;
    using std::max;
    using std::numeric_limits;

    using std::pair;
    using std::string;
    using std::vector;

    using std::ostream;
    using std::istream;
    using std::fstream;
    using std::ifstream;
    using std::ofstream;

    using std::stringstream;

}

#endif
