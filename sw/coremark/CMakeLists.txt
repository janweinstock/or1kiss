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

add_definitions(-DSEED_METHOD=SEED_VOLATILE
                -DPERFORMANCE_RUN=1
                -DITERATIONS=100
                -DFLAGS_STR=\"${CMAKE_C_FLAGS}\"
                -Wno-implicit-function-declaration)
add_executable(coremark core_list_join.c core_main.c core_matrix.c
                        core_portme.c core_state.c core_util.c clock.c)

add_test(NAME coremark COMMAND ../../or1kiss -e coremark.elf)
set_tests_properties(coremark PROPERTIES TIMEOUT 30)

install(TARGETS coremark DESTINATION sw)
