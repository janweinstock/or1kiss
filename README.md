# OpenRISC 1000 Instruction Set Simulator (or1kiss)

This repository contains the source code of `or1kiss`. It is an instruction
set simulator (ISS) for the OpenRISC 1000 architecture that supports the
`ORBIS32`, `ORFPX32` and `ORFPX64` instruction sets. The ISS employs a
decode cache to accelerate its regular interpretative execution and usually
achieves around 10-50 MIPS. While the repository also provides a standalone
simulator, `or1kiss` has been primarily designed to be embedded into system
level simulators, e.g. SystemC and TLM based virtual platforms.

[![Build Status](https://github.com/janweinstock/or1kiss/actions/workflows/cmake.yml/badge.svg)](https://github.com/janweinstock/or1kiss/actions/workflows/cmake.yml)
[![Sanitizer Status](https://github.com/janweinstock/or1kiss/actions/workflows/asan.yml/badge.svg)](https://github.com/janweinstock/or1kiss/actions/workflows/asan.yml)
[![Lint Status](https://github.com/janweinstock/or1kiss/actions/workflows/lint.yml/badge.svg)](https://github.com/janweinstock/or1kiss/actions/workflows/lint.yml)
[![Style Check](https://github.com/janweinstock/or1kiss/actions/workflows/style.yml/badge.svg)](https://github.com/janweinstock/or1kiss/actions/workflows/style.yml)

----
## Build & Installation
Currently, only Linux builds are supported. OSX builds may be possible, but
will require some manual work. Windows is currently not supported.

1. Install prerequisites `cmake` and `libelf`:
    ```
    sudo apt-get install cmake libelf-dev  # Ubuntu
    sudo yum install cmake3 libelf-dev     # Centos
    ```

2. Choose directories for building and deployment:
    ```
    <source-dir>  location of your repo copy,     e.g. /home/jan/or1kiss
    <build-dir>   location to store object files, e.g. /home/jan/or1kiss/BUILD
    <install-dir> output directory for binaries,  e.g. /opt/or1kiss
    ```

3. Configure and build the project using `cmake`. Two config options exist:
    * `-DOR1KISS_BUILD_SIM=[ON|OFF]`: build the standalone simulator (default: `ON`)
    * `-DOR1KISS_BUILD_SW=[ON|OFF]` : build sample software (default: `OFF`,
                                      requires `or1k-elf-gcc`)

   Release and debug build configurations are controlled via the regular
   parameters:
    ```
    mkdir -p <build-dir>
    cd <build-dir>
    cmake -DCMAKE_INSTALL_PREFIX=<install-dir> -DCMAKE_BUILD_TYPE=RELEASE -DOR1KISS_BUILD_SIM=ON <source-dir>
    make
    sudo make install
    ```

4. After installation, the following new files should be present:
    ```
    <install-dir>/bin/or1kiss        # standalone simulator
    <install-dir>/lib/libor1kiss.a   # simulator library
    <install-dir>/include/or1kiss.h  # header files
    <install-dir>/sw                 # sample applications (optional)
    ```

5. Update your environment so that other projects can reference your build:
    ```
    export OR1KISS_HOME=<install-dir>
    ```

----
## Maintaining Multiple Builds
Debug builds (i.e. `-DCMAKE_BUILD_TYPE=DEBUG`) are intended for developers
that use `or1kiss` within their own simulator and want to track down bugs.
However, these builds operate 4x slower than optimized release builds and
should therefore not be used in "productive" environments. To maintain both
builds from a single source repository, try the following:
```
git clone https://github.com/janweinstock/or1kiss.git && cd or1kiss
home=$PWD
for type in "DEBUG" "RELEASE"; do
    install="$home/BUILD/$type"
    build="$home/BUILD/$type/BUILD"
    mkdir -p $build && cd $build
    cmake -DCMAKE_BUILD_TYPE=$type -DCMAKE_INSTALL_PREFIX=$install $home
    make install
done
```
Afterwards, you can use an environment variable (e.g. `OR1KISS_HOME`) to point
to the build you want to use:
* `export OR1KISS_HOME=(...)/or1kiss/BUILD/DEBUG` for the debug build or
* `export OR1KISS_HOME=(...)/or1kiss/BUILD/RELEASE` for the release build

----
## Testing the Standalone Simulator
If you have built the sample software (`-DOR1KISS_BUILD_SW=ON`), you use it to test the
standalone simulator, e.g.:
```
$OR1KISS_HOME/bin/or1kiss -e $OR1KISS_HOME/sw/dhrystone.elf
$OR1KISS_HOME/bin/or1kiss -e $OR1KISS_HOME/sw/whetstone.elf
$OR1KISS_HOME/bin/or1kiss -e $OR1KISS_HOME/sw/coremark.elf
```
For example, the output for `coremark` should look similar to this:
```
2K performance run parameters for coremark.
CoreMark Size    : 666
Total ticks      : 20140
Total time (secs): 20.140000
Iterations/Sec   : 165.491559
Iterations       : 3333
Compiler version : GCC4.9.2
Compiler flags   : -mhard-mul -mhard-div -mhard-float -O3 
Memory location  : RAM
seedcrc          : 0xe9f5
[0]crclist       : 0xe714
[0]crcmatrix     : 0x1fd7
[0]crcstate      : 0x8e3a
[0]crcfinal      : 0x0737
Correct operation validated. See readme.txt for run and reporting rules.
CoreMark 1.0 : 165.491559 / GCC4.9.2 -mhard-mul -mhard-div -mhard-float -O3  / Heap
(or1kiss) silent exit(0)
simulation exit
# cycles       : 2018536256
# instructions : 1454515386
# dcc hit rate : 0.999995
# sim duration : 20.1854 seconds
# sim speed    : 58.1492 MIPS
# time taken   : 25.0135 seconds
```
Everything but the last six lines is output generated by the software running
in the simulator. When the simulation ends (e.g. via `l.nop 0x1` or `l.nop 0xc`
instructions), the simulator prints some statistics before exiting:
* **cycles**: cycles executed before program exit. Currently, each instruction
is assumed to consume one cycle, unless it loads from or stores to memory. In that
case, the memory subsystem adds one extra cycles.
* **instructions**: total number of instructions executed before encountering
the exit operation.
* **dcc hit rate**: the decode cache hit rate states the rate that the ISS was
able to skip instruction fetch and decode stages, because it has already
encountered a given instruction previously (e.g. in a loop).
* **sim duration**: simulated time assuming the processor runs at 100MHz.
* **sim speed**: stated in millions of simulated instructions per wall-clock
second (MIPS).
* **time taken**: wall-clock time in seconds from simulator start to exit.

----
## Debugging Target Software
The standalone simulator offers a simplistic gdb stub that can be used for
debugging target software. To run the simulator as a gdb target, use the
following as an example:

```
$OR1KISS_HOME/bin/or1kiss -e $OR1KISS_HOME/sw/dhrystone.elf -p 44044
```
This opens a local TCP port on `localhost:44044` that GDB can connect to:
```
or1k-elf-gdb $OR1KISS_HOME/sw/dhrystone.elf
(gdb) target remote :44044
(gdb) run
```

----
## Tracing Target Software
Additionally, debugging can also be done via instruction tracing. In order to
get a transaction trace for the first <N> instructions, invoke `or1kiss` like
this:
```
$OR1KISS_HOME/bin/or1kiss -e $OR1KISS_HOME/sw/dhrystone.elf -t trace.txt -i <N>
```
The generated instruction trace can be compared to `or1ksim` traces and should
look like this:
```
cat trace.txt
...
S 00010ea0: b4600001 l.mfspr r3,r0,0x1       r3         = 00000739  flag: 0
S 00010ea4: a4830004 l.andi  r4,r3,0x4       r4         = 00000000  flag: 0
S 00010ea8: e4040000 l.sfeq  r4,r0                                  flag: 1
S 00010eac: 10000021 l.bf    0x21                                   flag: 1
S 00010eb0: 15000000 l.nop   0                                      flag: 1
...
```

These traces can grow huge quite quickely, so make sure to limit the number of
instructions using the `-i <N>` command line switch. Tracing can also be
toggled on and off by the target software itself. This can be useful if code
needs tracing that executes only after some time. To toggle runtime tracing
from software, you can use the following code:
```
static inline void trace_enable() {
    asm volatile ("l.nop 0x8");
}

static inline void trace_disable() {
    asm volatile ("l.nop 0x9");
}
```

----
## License

This project is licensed under the Apache-2.0 license - see the
[LICENSE](LICENSE) file for details.