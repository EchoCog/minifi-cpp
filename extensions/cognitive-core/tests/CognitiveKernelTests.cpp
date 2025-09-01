/**
 * @file CognitiveKernelTests.cpp
 * @brief Test suite for Cognitive Kernel Foundation Layer
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "utils/CognitiveKernel.h"
#include "processors/CognitiveEchoProcessor.h"
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <memory>

using namespace org::apache::nifi::minifi::cognitive;
using namespace org::apache::nifi::minifi::cognitive::processors;

TEST_CASE("CognitiveMemory stores and retrieves data", "[cognitive][memory]") {
  CognitiveMemory memory;
  
  REQUIRE(memory.size() == 0);
  REQUIRE_FALSE(memory.exists("test_key"));
  REQUIRE(memory.retrieve("test_key").empty());
  
  memory.store("test_key", "test_value");
  
  REQUIRE(memory.size() == 1);
  REQUIRE(memory.exists("test_key"));
  REQUIRE(memory.retrieve("test_key") == "test_value");
  
  memory.clear();
  REQUIRE(memory.size() == 0);
  REQUIRE_FALSE(memory.exists("test_key"));
}

TEST_CASE("DeepTreeEcho identity structure", "[cognitive][echo]") {
  DeepTreeEcho echo;
  
  REQUIRE(echo.identity_signature.empty());
  REQUIRE(echo.resonance_frequency == 0);
  REQUIRE(echo.echo_patterns.empty());
  REQUIRE(echo.cognitive_weight == 1.0);
  
  DeepTreeEcho echo_with_signature("test_signature");
  REQUIRE(echo_with_signature.identity_signature == "test_signature");
  REQUIRE(echo_with_signature.resonance_frequency == 0);
  REQUIRE(echo_with_signature.cognitive_weight == 1.0);
}

TEST_CASE("CognitiveKernel initialization and state management", "[cognitive][kernel]") {
  CognitiveKernel kernel;
  
  REQUIRE(kernel.get_cognitive_state() == CognitiveState::DORMANT);
  REQUIRE(kernel.get_memory().size() == 0);
  
  DeepTreeEcho echo_identity("test_cognitive_identity");
  echo_identity.resonance_frequency = 12345;
  echo_identity.echo_patterns = {"pattern1", "pattern2", "cognitive"};
  echo_identity.cognitive_weight = 1.5;
  
  kernel.initialize(echo_identity);
  
  REQUIRE(kernel.get_cognitive_state() == CognitiveState::AWAKENING);
  REQUIRE(kernel.get_echo_identity().identity_signature == "test_cognitive_identity");
  REQUIRE(kernel.get_echo_identity().resonance_frequency == 12345);
  REQUIRE(kernel.get_echo_identity().cognitive_weight == 1.5);
  
  // Check that cognitive memory was initialized with echo identity
  REQUIRE(kernel.get_memory().size() > 0);
  REQUIRE(kernel.get_memory().exists("deep_tree_echo.identity"));
  REQUIRE(kernel.get_memory().retrieve("deep_tree_echo.identity") == "test_cognitive_identity");
}

TEST_CASE("CognitiveKernel resonance calculation", "[cognitive][kernel][resonance]") {
  CognitiveKernel kernel;
  
  DeepTreeEcho echo_identity("test_identity");
  echo_identity.echo_patterns = {"cognitive", "test"};
  echo_identity.cognitive_weight = 1.0;
  kernel.initialize(echo_identity);
  
  // Test resonance calculation
  double resonance1 = kernel.calculate_resonance("test cognitive data");
  double resonance2 = kernel.calculate_resonance("unrelated information");
  
  REQUIRE(resonance1 > 0.0);
  REQUIRE(resonance2 > 0.0);
  
  // Data with matching patterns should have higher resonance
  REQUIRE(resonance1 > resonance2);
  
  // Empty data should have zero resonance
  REQUIRE(kernel.calculate_resonance("") == 0.0);
}

TEST_CASE("CognitiveKernel signal processing", "[cognitive][kernel][processing]") {
  CognitiveKernel kernel;
  
  DeepTreeEcho echo_identity("processing_test");
  echo_identity.echo_patterns = {"signal"};
  kernel.initialize(echo_identity);
  
  REQUIRE(kernel.get_cognitive_state() == CognitiveState::AWAKENING);
  
  kernel.process_cognitive_signal("test signal data");
  
  REQUIRE(kernel.get_cognitive_state() == CognitiveState::REFLECTING);
  REQUIRE(kernel.get_memory().exists("last_signal"));
  REQUIRE(kernel.get_memory().retrieve("last_signal") == "test signal data");
  REQUIRE(kernel.get_memory().exists("last_resonance"));
  REQUIRE(kernel.get_memory().exists("process_count"));
}

TEST_CASE("CognitiveKernel state transitions", "[cognitive][kernel][state]") {
  CognitiveKernel kernel;
  
  REQUIRE(kernel.get_cognitive_state() == CognitiveState::DORMANT);
  
  kernel.set_cognitive_state(CognitiveState::LEARNING);
  REQUIRE(kernel.get_cognitive_state() == CognitiveState::LEARNING);
  
  kernel.set_cognitive_state(CognitiveState::REASONING);
  REQUIRE(kernel.get_cognitive_state() == CognitiveState::REASONING);
  
  kernel.set_cognitive_state(CognitiveState::CREATING);
  REQUIRE(kernel.get_cognitive_state() == CognitiveState::CREATING);
}