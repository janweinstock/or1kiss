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
project(or1kiss)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror")

set(src "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(inc "${CMAKE_CURRENT_SOURCE_DIR}/include")

set(sources
    "${src}/or1kiss/decode.cpp"
    "${src}/or1kiss/disasm.cpp"
    "${src}/or1kiss/execute.cpp"
    "${src}/or1kiss/insn.cpp"
    "${src}/or1kiss/spr.cpp"
    "${src}/or1kiss/mmu.cpp"
    "${src}/or1kiss/tick.cpp"
    "${src}/or1kiss/port.cpp"
    "${src}/or1kiss/or1k.cpp"
    "${src}/or1kiss/elf.cpp"
    "${src}/or1kiss/rsp.cpp"
    "${src}/or1kiss/gdb.cpp"
    "${src}/or1kiss/exception.cpp"
    "${src}/or1kiss/tracing.cpp")

add_library(or1kiss ${sources})
target_include_directories(or1kiss PUBLIC ${inc})

#add_executable(or1kisssim ${src}/main.cpp ${src}/memory.cpp)

install(TARGETS or1kiss DESTINATION lib)
install(DIRECTORY ${inc}/ DESTINATION include)
