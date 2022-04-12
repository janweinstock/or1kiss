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

#include <memory>
#include <iostream>
#include <exception>
#include <time.h>
#include <sys/time.h>
#include <or1kiss.h>

#include "memory.h"

void usage(const char* name) {
    fprintf(stderr, "Usage: %s [-e file] [-b file] ", name);
    fprintf(stderr, "[-t file] [-p port] [-m size] [-i num] [-w] [-x]\n");
    fprintf(stderr, "Arguments:\n");
    fprintf(stderr, "  -e <file>   elf binary to load into memory\n");
    fprintf(stderr, "  -b <file>   raw binary image to load into memory\n");
    fprintf(stderr, "  -t <file>   trace file to store trace information\n");
    fprintf(stderr, "  -p <port>   port number for debugger connection\n");
    fprintf(stderr, "  -m <size>   simulated memory size (in bytes)\n");
    fprintf(stderr, "  -i <n>      number of instructions to simulate\n");
    fprintf(stderr, "  -w          show warnings from debugger\n");
    fprintf(stderr, "  -z          disable instruction decode caching\n");
}

int main(int argc, char** argv) {
    char* elffile                   = NULL;
    char* binary                    = NULL;
    char* tracefile                 = NULL;
    unsigned short debugport        = 0;
    unsigned int memsize            = 0x08000000; // 128MB
    unsigned int ninsns             = 0;
    bool show_warn                  = false;
    or1kiss::decode_cache_size dcsz = or1kiss::DECODE_CACHE_SIZE_8M;

    int c; // parse command line
    while ((c = getopt(argc, argv, "e:b:t:p:m:i:vwxz")) != -1) {
        switch (c) {
        case 'e':
            elffile = optarg;
            break;
        case 'b':
            binary = optarg;
            break;
        case 't':
            tracefile = optarg;
            break;
        case 'p':
            debugport = atoi(optarg);
            break;
        case 'm':
            memsize = atoi(optarg);
            break;
        case 'i':
            ninsns = atoi(optarg);
            break;
        case 'v':
            puts("CTEST_FULL_OUTPUT");
            break;
        case 'w':
            show_warn = !show_warn;
            break;
        case 'z':
            dcsz = or1kiss::DECODE_CACHE_OFF;
            break;
        case 'h':
            usage(argv[0]);
            return EXIT_SUCCESS;
        default:
            usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    // Check if we got a program to simulate
    if ((elffile == NULL) && (binary == NULL) && (debugport == 0)) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    try {
        memory mem(memsize);
        or1kiss::or1k sim(&mem, dcsz);

        std::shared_ptr<or1kiss::elf> elf;
        if (elffile) {
            elf = std::make_shared<or1kiss::elf>(elffile);
            elf->load(&mem);
        }

        if (binary)
            mem.load(binary);

        if (tracefile)
            sim.trace(tracefile);

        timeval t1, t2;
        gettimeofday(&t1, NULL);

        if (debugport == 0) {
            if (ninsns > 0)
                sim.step(ninsns);
            else
                sim.run();
        } else {
            or1kiss::gdb debugger(sim, debugport);
            if (elf)
                debugger.set_elf(elf.get());
            debugger.show_warnings(show_warn);
            if (ninsns > 0)
                debugger.step(ninsns);
            else
                debugger.run();
        }

        gettimeofday(&t2, NULL);
        double t = (t2.tv_sec - t1.tv_sec) + (t2.tv_usec - t1.tv_usec) * 1e-6;
        double mips     = sim.get_num_instructions() / t / 1e6;
        double duration = sim.get_num_cycles() / (double)sim.get_clock();

        printf("simulation exit\n");
        printf("# cycles       : %" PRId64 "\n", sim.get_num_cycles());
        printf("# instructions : %" PRId64 "\n", sim.get_num_instructions());
        printf("# dcc hit rate : %f\n", sim.get_decode_cache_hit_rate());
        printf("# sim duration : %.4f seconds\n", duration);
        printf("# sim speed    : %.4f MIPS\n", mips);
        printf("# time taken   : %.4f seconds\n", t);

        return EXIT_SUCCESS;

    } catch (std::exception& ex) {
        fputs(ex.what(), stderr);
        fputs("\n", stderr);
        return EXIT_FAILURE;
    }
}
