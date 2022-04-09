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

#include "or1kiss/exception.h"

namespace or1kiss {

exception::exception(const exception& copy):
    std::exception(),
    m_code(copy.m_code),
    m_file(copy.m_file),
    m_line(copy.m_line),
    m_text(copy.m_text),
    m_what(copy.m_what) {
    // nothing to do
}

exception::exception(const char* file, int line, const char* fmt, ...):
    std::exception(),
    m_code(EXIT_FAILURE),
    m_file(file),
    m_line(line),
    m_text(),
    m_what() {
    va_list args;
    va_start(args, fmt);
    std::size_t sz = std::vsnprintf(NULL, 0, fmt, args) + 1;
    va_end(args);

    char* str = new char[sz];
    va_start(args, fmt);
    std::vsnprintf(str, sz, fmt, args);
    va_end(args);

    m_text = std::string(str);
    delete[] str;

    std::stringstream ss;
    ss << "or1kiss exception at " << m_file << ":" << line << " '" << m_text
       << "'";
    m_what = ss.str();
}

exception& exception::operator=(const exception& other) {
    m_code = other.m_code;
    m_file = other.m_file;
    m_line = other.m_line;
    m_text = other.m_text;
    m_what = other.m_what;

    return *this;
}

} // namespace or1kiss
