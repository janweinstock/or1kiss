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

#include "or1kiss/rsp.h"

namespace or1kiss {

    rsp::rsp():
        m_trace(false),
        m_socket(-1),
        m_connection(-1),
        m_addr() {
        char* env = getenv("OR1KISS_TRACE_RSP");
        if ((env != 0) && (*env != '0')) {
            m_trace = true;
            std::cout << "(or1kiss::rsp) tracing enabled" << std::endl;
        }
    }

    rsp::rsp(unsigned short port):
        m_trace(false),
        m_socket(-1),
        m_connection(-1),
        m_addr() {
        char* env = getenv("OR1KISS_TRACE_RSP");
        if ((env != 0) && (*env != '0')) {
            m_trace = true;
            std::cout << "(or1kiss::rsp) tracing enabled" << std::endl;
        }

        open(port);
    }

    rsp::~rsp() {
        close();
    }

    void rsp::open(unsigned short port) {
        // Close any existing connections
        if (is_open())
            close();

        // Open socket
        if ((m_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
            OR1KISS_ERROR("Cannot open socket (%s)", strerror(errno));

        // Set SO_REUSEADDR on m_socket
        int yes = 1;
        socklen_t sz = sizeof(yes);
        if (setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sz) == -1)
            OR1KISS_ERROR("setsockopt failed (%s)", strerror(errno));

        // Bind port to socket
        m_addr.sin_family = AF_INET;
        m_addr.sin_addr.s_addr = INADDR_ANY;
        m_addr.sin_port = htons(port);
        if (bind(m_socket, (struct sockaddr*) &m_addr, sizeof(m_addr)) == -1)
            OR1KISS_ERROR("Cannot bind socket (%s)", strerror(errno));

        // Start listening
        if (listen(m_socket, 1) == -1)
            OR1KISS_ERROR("Listening for connections failed (%s)");

        // Show output
        std::cout << "or1kiss: listening on port " << port << "... "
                  << std::flush;

        // Accept client on socket
        socklen_t addrlen = sizeof(struct sockaddr_in);
        struct sockaddr* paddr = reinterpret_cast<struct sockaddr*>(&m_addr);
        if ((m_connection = accept(m_socket, paddr, &addrlen)) == -1)
            OR1KISS_ERROR("Error connecting to client (%s)", strerror(errno));


        // Tell TCP not to delay packets to speed up interactive response
        socklen_t nodelay = 1;
        if (setsockopt(m_connection, IPPROTO_TCP, TCP_NODELAY, (char*)&nodelay,
                       sizeof(nodelay)) == -1) {
            OR1KISS_ERROR("setsockopt failed (%s)", strerror(errno));
        }

        std::cout << "connected" << std::endl;
    }

    void rsp::close() {
        if (is_open()) {
            ::close(m_connection);
            ::close(m_socket);
            m_connection = -1;
            m_socket = -1;
        }
    }

    bool rsp::peek() {
        fd_set in;
        struct timeval timeout;

        FD_ZERO(&in);
        FD_SET(m_connection, &in);

        timeout.tv_sec = 0;
        timeout.tv_usec = 0;

        return select(m_connection + 1, &in, NULL, NULL, &timeout) > 0;
    }

    char rsp::recv_char() {
        char c;
        switch (::recv(m_connection, &c, sizeof(c), 0)) {
        case -1: OR1KISS_ERROR("Error receiving data (%d)", strerror(errno));
        case  0: close(); return 0;
        default: return c;
        }
    }

    void rsp::send_char(char c) {
        switch (::send(m_connection, &c, sizeof(c), 0)) {
        case -1: OR1KISS_ERROR("Error sending data (%s)", strerror(errno));
        case  0: close(); break;
        default: break;
        }
    }

    string rsp::recv() {
        unsigned int idx = 0, checksum = 0, check = 0;
        char packet[OR1KISS_RSP_MAX_PACKET_SIZE];
        string cmd;

        // Collect packet data
        while (idx < OR1KISS_RSP_MAX_PACKET_SIZE) {
            char ch = recv_char();
            if (!is_open())
                return "";

            switch (ch) {
            case '$':
                checksum = 0;
                idx = 0;
                break;

            case '#':
                packet[idx++] = 0;
                check |= char2int(recv_char()) << 4;
                check |= char2int(recv_char()) << 0;
                send_char((check == checksum) ? '+' : '-');
                cmd = string(packet, packet + idx);
                if (m_trace)
                    std::cout << "(or1kiss::rsp) << " << cmd << std::endl;
                return cmd;

            default:
                checksum = (checksum + ch) & 0xff;
                packet[idx++] = ch;
                break;
            }
        }

        // Packet size exceeded
        OR1KISS_ERROR("Buffer overflow in RSP");
    }

    void rsp::send(const string& s) {
        if (m_connection == -1)
            OR1KISS_ERROR("No socket connection established");

        int len = 0;
        int sum = checksum(s.c_str());
        char* packet = new char [s.length() + 5];

        // Set initial command symbol
        packet[len++] = '$';

        // Copy message
        for (unsigned int i = 0; i < s.length(); i++)
            packet[len++] = s[i];

        // Set checksum
        packet[len++] = '#';
        packet[len++] = int2char((sum >> 4) & 0xf);
        packet[len++] = int2char((sum >> 0) & 0xf);
        packet[len++] = '\0';

        if (m_trace)
            std::cout << "(or1kiss::rsp) >> " << packet << std::endl;

        // Send packet
        ::send(m_connection, packet, len - 1, 0);
        delete [] packet;
    }

    void rsp::send(const char* fmt, ...) {
        if (m_connection == -1)
            OR1KISS_ERROR("No socket connection established");

        va_list args;
        va_start(args, fmt);
        int len = std::vsnprintf(NULL, 0, fmt, args);
        va_end(args);

        char* packet = new char[len + 5];
        packet[0] = '$';

        va_start(args, fmt);
        std::vsnprintf(packet + 1, len + 1, fmt, args);
        va_end(args);

        int sum = checksum(packet + 1);
        packet[len + 1] = '#';
        packet[len + 2] = int2char((sum >> 4) & 0xf);
        packet[len + 3] = int2char((sum >> 0) & 0xf);
        packet[len + 4] = '\0';

        if (m_trace)
            std::cout << "(or1kiss::rsp) >> " << packet << std::endl;

        // Send packet
        ::send(m_connection, packet, len + 4, 0);

        delete [] packet;
    }

}

