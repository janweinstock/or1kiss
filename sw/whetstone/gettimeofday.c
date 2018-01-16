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

#include <sys/time.h>
#include <sys/reent.h>

#define HZ 100000000ull /* 100MHz */

#define MSEC_PER_SEC    1000ull
#define USEC_PER_MSEC   1000ull
#define NSEC_PER_USEC   1000ull
#define USEC_PER_SEC    USEC_PER_MSEC * MSEC_PER_SEC

int gettimeofday_sim(struct timeval* tp) {
    unsigned long long cycles;
    register volatile unsigned long lo asm ("r11") = 0;
    register volatile unsigned long hi asm ("r12") = 0;

    asm volatile ("l.nop 0x6" : : : "r11", "r12");
    cycles = (unsigned long long)hi << 32 | lo;

    tp->tv_sec  = (cycles / HZ);
    tp->tv_usec = (cycles / (HZ / USEC_PER_SEC)) % USEC_PER_SEC;

    return 0;
}

int gettimeofday_host(struct timeval* tp) {
    unsigned long long ms = 0;
    register volatile unsigned long lo asm ("r11") = 0;
    register volatile unsigned long hi asm ("r12") = 0;

    asm volatile ("l.nop 0xd" : : : "r11", "r12");
    ms = (unsigned long long)hi << 32 | lo;

    tp->tv_sec  = ms / MSEC_PER_SEC;
    tp->tv_usec = (ms * USEC_PER_MSEC) % USEC_PER_SEC;

    return 0;
}

int _gettimeofday_r(struct _reent *reent, struct timeval* tp, void* tzp) {
    //return gettimeofday_sim(tp);
    return gettimeofday_host(tp);
}
