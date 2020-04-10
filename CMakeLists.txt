 ##############################################################################
 #                                                                            #
 # Copyright 2018 Jan Henrik Weinstock                                        #
 #                                                                            #
 # Licensed under the Apache License, Version 2.0 (the "License");            #
 # you may not use this file except in compliance with the License.           #
 # You may obtain a copy of the License at                                    #
 #                                                                            #
 #     http://www.apache.org/licenses/LICENSE-2.0                             #
 #                                                                            #
 # Unless required by applicable law or agreed to in writing, software        #
 # distributed under the License is distributed on an "AS IS" BASIS,          #
 # WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   #
 # See the License for the specific language governing permissions and        #
 # limitations under the License.                                             #
 #                                                                            #
 ##############################################################################

cmake_minimum_required(VERSION 3.5)
project(or1kiss LANGUAGES C CXX)

option(OR1KISS_BUILD_SIM "Build the standalone simulator" ON)
option(OR1KISS_BUILD_SW "Build sample software, requires or1k-elf-gcc" OFF)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
find_package(LibElf REQUIRED)

set(CMAKE_CXX_STANDARD 11)

set(src ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(inc ${CMAKE_CURRENT_SOURCE_DIR}/include)

set(sources
    ${src}/or1kiss/decode.cpp
    ${src}/or1kiss/disasm.cpp
    ${src}/or1kiss/execute.cpp
    ${src}/or1kiss/insn.cpp
    ${src}/or1kiss/spr.cpp
    ${src}/or1kiss/env.cpp
    ${src}/or1kiss/mmu.cpp
    ${src}/or1kiss/tick.cpp
    ${src}/or1kiss/or1k.cpp
    ${src}/or1kiss/elf.cpp
    ${src}/or1kiss/rsp.cpp
    ${src}/or1kiss/gdb.cpp
    ${src}/or1kiss/exception.cpp
    ${src}/or1kiss/tracing.cpp)

add_library(or1kiss ${sources})
target_include_directories(or1kiss PUBLIC ${inc})
target_include_directories(or1kiss PUBLIC ${LIBELF_INCLUDE_DIRS})
target_link_libraries(or1kiss PUBLIC ${LIBELF_LIBRARIES})

install(TARGETS or1kiss DESTINATION lib)
install(DIRECTORY ${inc}/ DESTINATION include)

if(OR1KISS_BUILD_SIM)
    add_executable(sim ${src}/main.cpp ${src}/memory.cpp)
    target_link_libraries(sim or1kiss ${LIBELF_LIBRARIES} -pthread)
    set_target_properties(sim PROPERTIES OUTPUT_NAME "or1kiss")
    install(TARGETS sim DESTINATION bin)
endif()

if(OR1KISS_BUILD_SW)
    enable_testing()
    add_subdirectory(sw)
endif()
