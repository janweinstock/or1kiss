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

#ifndef OR1KISS_UTILS_H
#define OR1KISS_UTILS_H

#include "or1kiss/includes.h"
#include "or1kiss/types.h"

namespace or1kiss {

    // helper for code generation
    inline int likely(int x) {
        return __builtin_expect(!!(x), 1);
    }

    // helper for code generation
    inline int unlikely(int x) {
        return __builtin_expect(!!(x), 0);
    }

    // compare-and-swap for exclusive memory access
    inline bool cas(void* ptr, u32 oldval, u32 newval) {
        return __sync_bool_compare_and_swap((u32*)ptr, oldval, newval);
    }

    // checksum computes the 8bit checksum of a given string
    // by summing up the character values of the string and truncating
    // the result to a 8bit value.
    inline int checksum(const char* str) {
        int result = 0;
        while (str && *str)
            result += static_cast<int>(*str++);
        return result & 0xff;
    }

    // char2int returns the integer value for a given character.
    // All hexadecimal characters can be translated.
    inline int char2int(char c) {
        return ((c >= 'a') && (c <= 'f')) ? c - 'a' + 10 :
               ((c >= 'A') && (c <= 'F')) ? c - 'A' + 10 :
               ((c >= '0') && (c <= '9')) ? c - '0' :
               (c == '\0') ? 0 : -1;
    }

    // str2int reads the first n characters of string s and converts
    // them to integer using hex as base.
    inline int str2int(const char* s, int n) {
        int val = 0;
        for (const char* c = s + n - 1; c >= s; c--) {
            val <<= 4;
            val |= char2int(*c);
        }
        return val;
    }

    // int2char converts the given integer to its hexadecimal
    // character representation. Only the last 4 bits are converted.
    inline char int2char(int h) {
        static const char hexchars[] =
            {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
        return hexchars[h & 0xf];
    }

    // helper for gdb's write binary function that converts char data to
    // u8 and also does unescaping for special chars.
    inline u8 char_unescape(const char*& s) {
        u8 result = *s++;
        if (result == '}')
            result = *s++ ^ 0x20;
        return result;
    }

    // stl_contains returns true, if the stl container v contains the
    // element e of type E at least once.
    template <class V, class E>
    inline bool stl_contains(const V& v, const E& e) {
        return std::find(v.begin(), v.end(), e) != v.end();
    }

    // stl_remove_erase implements the remove-erase idiom the deletes
    // all occurrences of the element e in the stl container v.
    template <class V, class E>
    inline void stl_remove_erase(V& v, const E& e) {
        v.erase(std::remove(v.begin(), v.end(), e), v.end());
    }

    template <class V, class PRED>
    inline void stl_remove_erase_if(V& v, PRED p) {
        v.erase(std::remove_if(v.begin(), v.end(), p), v.end());
    }

    // stl_make_str builds a std::string object from a printf like
    // variable length argument list.
    inline std::string stl_make_str(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        std::size_t sz = std::vsnprintf(NULL, 0, fmt, args) + 1;
        va_end(args);

        char* str = new char[sz];
        va_start(args, fmt);
        std::vsnprintf(str, sz, fmt, args);
        va_end(args);

        std::string result(str);
        delete [] str;
        return result;
    }

}

#endif
