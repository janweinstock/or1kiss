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

#ifndef OR1KISS_RSP_H
#define OR1KISS_RSP_H

#include "or1kiss/includes.h"
#include "or1kiss/utils.h"
#include "or1kiss/exception.h"

#define OR1KISS_RSP_MAX_PACKET_SIZE  0x4000u

namespace or1kiss {

    class rsp
    {
    private:
        bool m_trace;
        int  m_socket;
        int  m_connection;
        struct sockaddr_in m_addr;

        // Disabled
        rsp(const rsp&);

    public:
        inline bool is_open() const {
            return m_socket != -1;
        }

        inline bool is_connected() const {
            return m_connection != -1;
        }

        inline unsigned short get_port() const {
            return ntohs(m_addr.sin_port);
        }

        rsp();
        rsp(unsigned short);
        virtual ~rsp();

        void open(unsigned short);
        void close();

        bool peek();

        char recv_char();
        void send_char(char);

        std::string recv();
        void send(const std::string&);
        void send(const char* fmt, ...);
    };

}

#endif
