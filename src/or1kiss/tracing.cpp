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

#include "or1kiss/or1k.h"
#include "or1kiss/disasm.h"

#define OR1KISS_TRACE_MAXLEN 80

namespace or1kiss {

void or1k::do_trace(const instruction* insn) {
    int n = 0;
    char buffer[OR1KISS_TRACE_MAXLEN];
    char trace_mode = (m_status & SR_SM) ? 'S' : 'U';

    u64 trace_addr = m_next_pc;
    u32 trace_insn = insn->insn;

    n += snprintf(buffer + n, sizeof(buffer) - n,
                  "%c %08" PRIx64 ": %08" PRIx32 " %-24s", trace_mode,
                  trace_addr, trace_insn, disassemble(trace_insn).c_str());

    switch (decode(trace_insn)) {
    case ORBIS32_MTSPR: {
        u32 regnum = *insn->dest | insn->imm;
        u32 regval = get_spr(regnum, true);
        n += snprintf(buffer + n, sizeof(buffer) - n, "SPR[%04x]  = %08x ",
                      regnum, regval);
        break;
    }
    case ORBIS32_SW:
        n += snprintf(buffer + n, sizeof(buffer) - n,
                      "[%08" PRIx32 "] = %08" PRIx32 "%1s", m_trace_addr,
                      *insn->src2, " ");
        break;
    case ORBIS32_SH:
        n += snprintf(buffer + n, sizeof(buffer) - n,
                      "[%08" PRIx32 "] = %04" PRIx32 "%5s", m_trace_addr,
                      *insn->src2, " ");
        break;
    case ORBIS32_SB:
        n += snprintf(buffer + n, sizeof(buffer) - n,
                      "[%08" PRIx32 "] = %02" PRIx32 "%7s", m_trace_addr,
                      *insn->src2, " ");
        break;
    default:
        if ((insn->dest >= gpr) && (insn->dest < (gpr + sizeof(gpr)))) {
            n += snprintf(buffer + n, sizeof(buffer) - n,
                          "r%-10" PRIdPTR "= %08" PRIx32 " ", insn->dest - gpr,
                          *insn->dest);
        } else
            n += snprintf(buffer + n, sizeof(buffer) - n, "%22s", " ");
        break;
    }

    sprintf(buffer + n, " flag: %c", (m_status & SR_F) ? '1' : '0');
    if (!m_user_trace_stream)
        m_user_trace_stream = &std::cerr;
    *m_user_trace_stream << buffer << std::endl;
}

} // namespace or1kiss
