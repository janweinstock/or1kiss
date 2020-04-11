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

#ifndef OR1KISS_TICK_H
#define OR1KISS_TICK_H

#include "or1kiss/includes.h"
#include "or1kiss/types.h"
#include "or1kiss/exception.h"
#include "or1kiss/bitops.h"

namespace or1kiss {

    enum timer_mode {
        TM_IP = 1 << 28, /* Interrupt Pending */
        TM_IE = 1 << 29, /* Interrupt Enabled */
        TM_D  = 0 << 30, /* Timer Disabled */
        TM_RS = 1 << 30, /* Timer Mode Restart */
        TM_OS = 2 << 30, /* Timer Mode One-Shot */
        TM_CT = 3 << 30  /* Timer Mode Continue */
    };

    class tick
    {
    private:
        bool m_done;
        u32 m_ttmr;
        u32 m_ttcr;

    public:
        u32 get_ttmr() const { return m_ttmr; }
        u32 get_ttcr() const { return m_ttcr; }

        void set_ttmr(u32 v) {
            m_ttmr = v;
            update(0);
        }

        void set_ttcr(u32 v) {
            m_ttcr = v;
            m_done = false;
            update(0);
        }

        bool enabled()     const { return m_ttmr >> 30; }
        bool irq_enabled() const { return m_ttmr & TM_IE; }
        bool irq_pending() const { return m_ttmr & TM_IP; }

        u32 limit()     const { return bits32(m_ttmr, 27, 0); }
        u32 current()   const { return bits32(m_ttcr, 27, 0); }
        u64 next_tick() const {
            if (current() < limit())
                return limit() - current();
            else
                return 0x0fffffffull - current() + limit() + 1ull;
        }

        tick();
        virtual ~tick();

        void update(u64 delta = 1);
    };

}

#endif
