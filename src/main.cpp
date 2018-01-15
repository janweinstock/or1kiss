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

#include <iostream>
#include <exception>
#include <time.h>
#include <sys/time.h>

#include "or1kiss.h"
#include "memory.h"

void usage()
{
    puts("Usage: sim -e binary [-b filename] [-t tracefile] ");
    puts("[-p port] [-m memsize] [-c cycles] [-w] [-x]");
    puts("Arguments:");
    puts("  -e <filename> Elf binary to load and simulate");
    puts("  -b <filename> Raw binary image to load into memory");
    puts("  -t <filename> Trace file to store trace information");
    puts("  -p <port>     Port number for debugger connection");
    puts("  -m <memsize>  Simulated memory size (in bytes)");
    puts("  -c <cycles>   Number of cycles to simulate");
    puts("  -w            Show warnings from debugger");
    puts("  -x            Do not use pointers for memory access");
    puts("  -z            Disable instruction decode caching");
}

int main(int argc, char** argv)
{
    char* elffile = NULL;
    char* binary = NULL;
    char* tracefile = NULL;
    unsigned short debugport = 0;
    unsigned int memsize = 0x08000000; // 128MB
    unsigned int cycles = 0;
    bool show_warn = false;
    bool use_ptrs = true;
    or1kiss::decode_cache_size dcsz = or1kiss::DECODE_CACHE_SIZE_8M;

    int c; // parse command line
    while ((c = getopt(argc, argv, "e:b:t:p:m:c:vwxz")) != -1) {
        switch(c) {
        case 'e': elffile   = optarg; break;
        case 'b': binary    = optarg; break;
        case 't': tracefile = optarg; break;
        case 'p': debugport = atoi(optarg); break;
        case 'm': memsize   = atoi(optarg); break;
        case 'c': cycles    = atoi(optarg); break;
        case 'v': puts("CTEST_FULL_OUTPUT"); break;
        case 'w': show_warn = !show_warn; break;
        case 'x': use_ptrs  = !use_ptrs; break;
        case 'z': dcsz      = or1kiss::DECODE_CACHE_OFF; break;
        case 'h': usage(); return EXIT_SUCCESS;
        default : usage(); return EXIT_FAILURE;
        }
    }

    // Check if we got a program to simulate
    if ((elffile == NULL) && (binary == NULL) && (debugport == 0)) {
        usage();
        return EXIT_FAILURE;
    }

    try {
        memory mem(memsize);
        or1kiss::or1k sim(&mem, dcsz);

        if (use_ptrs) {
            sim.set_insn_ptr(mem.get_ptr(), 0, mem.get_size() - 1, 0);
            sim.set_data_ptr(mem.get_ptr(), 0, mem.get_size() - 1, 1);
        }

        or1kiss::elf* elf = NULL;
        if (elffile) {
            elf = new or1kiss::elf(elffile);
            elf->load(&mem);
        }

        if (binary)
            mem.load(binary);

        if (tracefile)
            sim.trace(tracefile);

        timeval t1, t2;
        gettimeofday(&t1, NULL);

        if (debugport == 0)
            if (cycles > 0)
                sim.step(cycles);
            else
                sim.run();
        else {
            or1kiss::gdb debugger(sim, debugport);
            if (elf)
                debugger.set_elf(elf);
            debugger.warnings(show_warn);
            if (cycles > 0)
                debugger.step(cycles);
            else
                debugger.run();
        }

        gettimeofday(&t2, NULL);
        double t = (t2.tv_sec  - t1.tv_sec) + (t2.tv_usec - t1.tv_usec) * 1e-6;
        double mips = sim.get_num_instructions() / t / 1e6;
        double duration = sim.get_num_cycles() / (double)sim.get_clock();

        printf("simulation exit\n");
        printf("# cycles       : %"PRId64"\n", sim.get_num_cycles());
        printf("# instructions : %"PRId64"\n", sim.get_num_instructions());
        printf("# jit hit rate : %f\n", sim.get_decode_cache_hit_rate());
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
