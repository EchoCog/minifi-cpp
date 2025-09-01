#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#

include(${CMAKE_SOURCE_DIR}/extensions/ExtensionHeader.txt)

file(GLOB SOURCES "processors/*.cpp" "controllers/*.cpp" "utils/*.cpp")

add_minifi_library(minifi-cognitive-core SHARED ${SOURCES})
target_include_directories(minifi-cognitive-core PUBLIC "${CMAKE_SOURCE_DIR}/extensions/cognitive-core")

include(RangeV3)
target_link_libraries(minifi-cognitive-core ${LIBMINIFI} Threads::Threads range-v3)

if(BUILD_TESTS)
    file(GLOB TESTS "tests/*.cpp")
    if(TESTS)
        get_filename_component(test_target "cognitive-core-tests" NAME_WE)
        add_minifi_executable("${test_target}" "${TESTS}")
        target_include_directories(${test_target} PRIVATE BEFORE "${CMAKE_SOURCE_DIR}/extensions/cognitive-core")
        target_link_libraries("${test_target}" minifi-cognitive-core ${LIBMINIFI} Catch2::Catch2 Catch2::Catch2WithMain)
        add_test(NAME "${test_target}" COMMAND "${test_target}")
        set_property(TARGET "${test_target}" PROPERTY RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    endif()
endif()

register_extension(minifi-cognitive-core "COGNITIVE CORE" COGNITIVE-CORE-EXTENSIONS "This enables cognitive processing capabilities" "extensions/cognitive-core/tests/")