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

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR or1k)
set(CMAKE_EXECUTABLE_SUFFIX ".elf")

set(TOOLCHAIN_PREFIX or1k-elf-)
set(CMAKE_C_COMPILER   ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++)

set(CMAKE_C_FLAGS "-O3 -mhard-mul -mhard-div -mhard-float -mdouble-float")

set(CMAKE_C_FLAGS_${CMAKE_BUILD_TYPE} "" CACHE STRING "" FORCE)
set(CMAKE_EXE_LINKER_FLAGS_${CMAKE_BUILD_TYPE} "" CACHE STRING "" FORCE)
set(CMAKE_SHARED_LINKER_FLAGS_${CMAKE_BUILD_TYPE} "" CACHE STRING "" FORCE)

if(CMAKE_BUILD_TYPE MATCHES "ASAN")
    string(APPEND ENVVARS "ASAN_OPTIONS=suppressions=${CMAKE_CURRENT_SOURCE_DIR}/sanitizer/asan.cfg:detect_leaks=1:fast_unwind_on_malloc=0;")
    string(APPEND ENVVARS "LSAN_OPTIONS=suppressions=${CMAKE_CURRENT_SOURCE_DIR}/sanitizer/lsan.cfg;")
elseif(CMAKE_BUILD_TYPE MATCHES "TSAN")
    string(APPEND ENVVARS "TSAN_OPTIONS=suppressions=${CMAKE_CURRENT_SOURCE_DIR}/sanitizer/tsan.cfg;")
elseif(CMAKE_BUILD_TYPE MATCHES "UBSAN")
    string(APPEND ENVVARS "UBSAN_OPTIONS=suppressions=${CMAKE_CURRENT_SOURCE_DIR}/sanitizer/ubsan.cfg:print_stacktrace=1;")
endif()

add_subdirectory(coremark)
add_subdirectory(dhrystone)
add_subdirectory(whetstone)
