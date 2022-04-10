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

cmake_minimum_required(VERSION 3.11)
project(or1kiss VERSION 2022.04.10 LANGUAGES C CXX)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(GenVersionInfo)
include(Sanitizer)

find_package(LibELF REQUIRED)

option(OR1KISS_BUILD_SIM "Build the standalone simulator" ON)
option(OR1KISS_BUILD_SW "Build sample software, requires or1k-elf-gcc" OFF)
set(OR1KISS_LINTER "" CACHE STRING "Code linter to use")

set(src ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(inc ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(gen ${CMAKE_CURRENT_BINARY_DIR}/gen)

configure_file(${src}/or1kiss/version.h.in
               ${gen}/or1kiss/version.h @ONLY)

add_library(libor1kiss STATIC
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

target_compile_options(libor1kiss PRIVATE -Wall -Werror)
target_compile_features(libor1kiss PRIVATE cxx_std_17)
target_include_directories(libor1kiss PUBLIC ${inc})
target_include_directories(libor1kiss PUBLIC ${gen})
target_include_directories(libor1kiss PRIVATE ${src})
target_include_directories(libor1kiss PUBLIC ${LIBELF_INCLUDE_DIRS})
target_link_libraries(libor1kiss PUBLIC ${LIBELF_LIBRARIES})

set_target_properties(libor1kiss PROPERTIES DEBUG_POSTFIX "d")
set_target_properties(libor1kiss PROPERTIES CXX_CLANG_TIDY "${OR1KISS_LINTER}")
set_target_properties(libor1kiss PROPERTIES VERSION "${OR1KISS_VERSION}")
set_target_properties(libor1kiss PROPERTIES SOVERSION "${OR1KISS_VERSION_MAJOR}")

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    install(TARGETS libor1kiss DESTINATION lib)
    install(DIRECTORY ${inc}/ DESTINATION include)
    install(DIRECTORY ${gen}/ DESTINATION include)

    if(OR1KISS_BUILD_SIM)
        add_executable(or1kiss ${src}/main.cpp ${src}/memory.cpp)
        target_link_libraries(or1kiss libor1kiss -pthread)
        set_target_properties(or1kiss PROPERTIES CXX_CLANG_TIDY "${OR1KISS_LINTER}")
        set_target_properties(or1kiss PROPERTIES VERSION "${OR1KISS_VERSION}")
        set_target_properties(or1kiss PROPERTIES SOVERSION "${OR1KISS_VERSION_MAJOR}")
        install(TARGETS or1kiss DESTINATION bin)
    endif()

    if(OR1KISS_BUILD_SW)
        enable_testing()
        add_subdirectory(sw)
    endif()
endif()
