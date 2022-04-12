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

add_executable(whetstone whetstone.c gettimeofday.c)
target_compile_definitions(whetstone PRIVATE -DPRINTOUT)
target_link_libraries(whetstone m)

add_test(NAME whetstone COMMAND $<TARGET_FILE:or1kiss-sim> -e $<TARGET_FILE:whetstone>)
set_tests_properties(whetstone PROPERTIES ENVIRONMENT "${ENVVARS}")
set_tests_properties(whetstone PROPERTIES TIMEOUT 300)

install(TARGETS whetstone DESTINATION sw)
