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

    void tick::update(u64 delta) {
        timer_mode mode = static_cast<timer_mode>(m_ttmr & 0xc0000000);
        if (mode == TM_D || m_done)
            return;

        u64 lim = limit();
        u64 cur = current();

        bool irq_enabled = m_ttmr & TM_IE;
        bool irq_set = false;

        switch (mode) {
        case TM_RS:
            if (cur < lim && cur + delta >= lim) {
                irq_set = true;
                m_ttcr = 0;
            } else {
                m_ttcr += delta;
            }
            break;

        case TM_OS:
            if (cur < lim && cur + delta >= lim) {
                m_done = irq_set = true;
                m_ttcr = lim;
            } else {
                m_ttcr += delta;
            }
            break;

        case TM_CT:
            irq_set = cur < lim && cur + delta >= lim;
            m_ttcr += delta;
            break;

        default:
            OR1KISS_ERROR("Invalid tick timer mode (%d)", mode);
        }

        if (irq_enabled && irq_set)
            m_ttmr |= TM_IP;
    }

}
