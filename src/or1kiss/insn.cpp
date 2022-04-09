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

#include "or1kiss/insn.h"

namespace or1kiss {

decode_cache::decode_cache(decode_cache_size size):
    m_size(size), m_mask((1 << size) - 1), m_count(1 << size), m_cache(NULL) {
    m_cache = new instruction[m_count];

    invalidate_all();
}

decode_cache::~decode_cache() {
    delete[] m_cache;
}

} // namespace or1kiss
