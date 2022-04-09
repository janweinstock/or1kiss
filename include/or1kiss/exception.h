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

#ifndef OR1KISS_EXCEPTION_H
#define OR1KISS_EXCEPTION_H

#include "or1kiss/includes.h"
#include "or1kiss/types.h"

#define OR1KISS_ERROR(...)                                         \
    do {                                                           \
        throw or1kiss::exception(__FILE__, __LINE__, __VA_ARGS__); \
    } while (0)

namespace or1kiss {

class exception : public std::exception
{
private:
    int m_code;
    string m_file;
    int m_line;
    string m_text;
    string m_what;

public:
    exception() = delete;
    exception(const exception&);
    exception(const char*, int, const char*, ...);

    virtual ~exception() noexcept = default;

    exception& operator=(const exception&);

    inline int get_exit_code() const { return m_code; }
    inline const char* get_msg() const { return m_text.c_str(); }
    inline const char* get_file_name() const { return m_file.c_str(); }
    inline int get_line_number() const { return m_line; }

    virtual const char* what() const throw() { return m_what.c_str(); }
};

} // namespace or1kiss

#endif
