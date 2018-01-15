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

#include "or1kiss/tick.h"

namespace or1kiss {

    tick::tick():
        m_done(false),
        m_ttmr(0),
        m_ttcr(0) {

    }

    tick::~tick() {
        /* Nothing to do */
    }

    void tick::update(u64 _delta) {
        timer_mode mode = static_cast<timer_mode>(m_ttmr & 0xc0000000);
        if (mode == TM_D)
            return;

        u32 delta = static_cast<u32>(_delta);
        u32 limit = bits32(m_ttmr, 27, 0);
        u32 count = bits32(m_ttcr, 27, 0);

        bool irq_enabled = m_ttmr & TM_IE;
        bool irq_set = false;

        switch (mode) {
        case TM_RS:
            printf("TIMER MODE RESET\n");
            if ((count < limit) && ((count + delta) >= limit)) {
                irq_set = true;
                m_ttcr = 0;
            } else
                m_ttcr += delta;
            break;

        case TM_OS:
            printf("TIMER MODE ONESHOT\n");
            if (!m_done) {
                if ((count < limit) && ((count + delta) >= limit)) {
                    irq_set = true;
                    m_ttcr = limit;
                    m_done = true;
                } else
                    m_ttcr += delta;
            }
            break;

        case TM_CT:
            //if (irq_enabled && (delta > limit)) {
            //    printf("Warning: delta exceeds limit (0x%08x > 0x%08x; 0x%08x)\n", delta, limit, count);
            //    printf("         next tick 0x%08llx\n", next_tick());
            //}

            if ((count < limit) && ((count + delta) >= limit))
                irq_set = true;
            m_ttcr += delta;
            //if (irq_enabled && (count > limit))
            //    printf("Warning: TIMER COUNT > LIMIT (0x%08x > 0x%08x)\n", count, limit);
            break;

        default:
            OR1KISS_ERROR("Invalid tick timer mode (%d)", mode);
        }

        if (irq_enabled && irq_set)
            m_ttmr |= TM_IP;
    }

}
