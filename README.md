# OpenRISC 1000 Instruction Set Simulator (or1kiss)

----
## Build & Installation

1. Make sure these environment variables are set before the build:

        SYSTEMC_HOME (point to SystemC install directory)
        TARGET_ARCH  (linux for 32bit build)
        VCL_HOME     (point to VCL install directory)
    
Note that the environment variable `VCL_HOME` is optional and only needed if
you want to build the SoC simulator (e.g., for running linux).

2. Create a directory for building:

        mkdir -p build/debug
        cd build/debug  

3. Configure and build the project. Use `-DBUILD_SOC=OFF` if you only need the
standalone simulator (works with all bare-metal applications, but not linux).

        cmake ../../ \
              -DCMAKE_BUILD_TYPE=[DEBUG|RELEASE] \
              -DCMAKE_INSTALL_PREFIX=install \
              -DBUILD_SOC=[ON|OFF]
        make
        make install

4. Build the example bare-metal applications:

        mkdir -p build/apps
        cd build/apps
        cmake ../../sim/app
              -DCMAKE_TOOLCHAIN_FILE=../../sim/app/or1k-elf-toolchain.cmake
              -DSIM_COMMAND=../../build/debug/install/bin/sim # path to sim
        make
        make test

----
## Contact

Jan Henrik Weinstock (jan.weinstock@ice.rwth-aachen.de) -- Apr. 2015