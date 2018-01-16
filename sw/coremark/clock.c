/*********************************************************************
 *                                                                   *
 *                    COPYRIGHT(c) 2012, 2016                        *
 *   INSTITUTE FOR COMMUNICATION TECHNOLOGIES AND EMBEDDED SYSTEMS   *
 *                         RWTH AACHEN                               *
 *                           GERMANY                                 *
 *                                                                   *
 * This confidential and proprietary software may be used, copied,   *
 * modified, merged, published or distributed according to the       *
 * permissions and/or limitations granted by an authorizing license  *
 * agreement.                                                        *
 *                                                                   *
 * The above copyright notice and this permission notice shall be    *
 * included in all copies or substantial portions of the Software.   *
 *                                                                   *
 * Author: Jan Henrik Weinstock (jan.weinstock@ice.rwth-aachen.de)   *
 *********************************************************************/

#include <time.h>

#define HZ 100000000 /* 100MHz */

#define MSEC_PER_SEC    1000ull
#define USEC_PER_MSEC   1000ull
#define NSEC_PER_USEC   1000ull
#define USEC_PER_SEC    USEC_PER_MSEC * MSEC_PER_SEC
#define NSEC_PER_SEC    USEC_PER_SEC * NSEC_PER_USEC

int clock_gettime(clockid_t clk_id, struct timespec *tp)
{
    unsigned long long cycles;
    register volatile unsigned long lo asm ("r11") = 0;
    register volatile unsigned long hi asm ("r12") = 0;

    asm volatile ("l.nop 0x6" : : : "r11", "r12");
    cycles = (unsigned long long)hi << 32 | lo;

    tp->tv_sec  = cycles / HZ;

#if (HZ < NSEC_PER_SEC)
    tp->tv_nsec = (cycles * (NSEC_PER_SEC / HZ)) % NSEC_PER_SEC;
#else
    tp->tv_nsec = (cycles / (HZ / NSEC_PER_SEC)) % NSEC_PER_SEC;
#endif

    return 0;
}
