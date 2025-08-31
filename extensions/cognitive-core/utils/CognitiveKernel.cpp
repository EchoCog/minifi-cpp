/**
 * @file CognitiveKernel.cpp
 * @brief Foundation Layer: Cognitive Kernel Genesis Implementation
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
#include "core/ProcessContext.h"
#include "core/ProcessSession.h"
#include <algorithm>
#include <cmath>
#include <functional>

namespace org::apache::nifi::minifi::cognitive {

// CognitiveMemory Implementation
void CognitiveMemory::store(const std::string& key, const std::string& value) {
  std::lock_guard<std::mutex> lock(memory_mutex_);
  memory_store_[key] = value;
}

std::string CognitiveMemory::retrieve(const std::string& key) const {
  std::lock_guard<std::mutex> lock(memory_mutex_);
  auto it = memory_store_.find(key);
  return (it != memory_store_.end()) ? it->second : "";
}

bool CognitiveMemory::exists(const std::string& key) const {
  std::lock_guard<std::mutex> lock(memory_mutex_);
  return memory_store_.find(key) != memory_store_.end();
}

void CognitiveMemory::clear() {
  std::lock_guard<std::mutex> lock(memory_mutex_);
  memory_store_.clear();
}

size_t CognitiveMemory::size() const {
  std::lock_guard<std::mutex> lock(memory_mutex_);
  return memory_store_.size();
}

// CognitiveKernel Implementation
CognitiveKernel::CognitiveKernel() 
  : current_state_(CognitiveState::DORMANT), process_counter_(0) {
}

void CognitiveKernel::initialize(const DeepTreeEcho& echo_identity) {
  std::lock_guard<std::mutex> lock(kernel_mutex_);
  echo_identity_ = echo_identity;
  set_cognitive_state(CognitiveState::AWAKENING);
  
  // Initialize cognitive memory with echo identity
  cognitive_memory_.store("deep_tree_echo.identity", echo_identity.identity_signature);
  cognitive_memory_.store("deep_tree_echo.frequency", std::to_string(echo_identity.resonance_frequency));
  cognitive_memory_.store("deep_tree_echo.weight", std::to_string(echo_identity.cognitive_weight));
  
  // Store echo patterns
  for (size_t i = 0; i < echo_identity.echo_patterns.size(); ++i) {
    cognitive_memory_.store("deep_tree_echo.pattern." + std::to_string(i), 
                           echo_identity.echo_patterns[i]);
  }
}

void CognitiveKernel::process_cognitive_signal(const std::string& signal_data) {
  auto old_state = current_state_.load();
  set_cognitive_state(CognitiveState::PROCESSING);
  
  try {
    // Calculate resonance with current echo identity
    double resonance = calculate_resonance(signal_data);
    
    // Store processing metadata
    cognitive_memory_.store("last_signal", signal_data);
    cognitive_memory_.store("last_resonance", std::to_string(resonance));
    cognitive_memory_.store("process_count", std::to_string(process_counter_.fetch_add(1) + 1));
    
    // Allow derived classes to process
    on_cognitive_process(signal_data);
    
  } catch (...) {
    set_cognitive_state(old_state);
    throw;
  }
  
  set_cognitive_state(CognitiveState::REFLECTING);
}

void CognitiveKernel::set_cognitive_state(CognitiveState new_state) {
  auto old_state = current_state_.exchange(new_state);
  if (old_state != new_state) {
    on_state_transition(old_state, new_state);
  }
}

CognitiveState CognitiveKernel::get_cognitive_state() const {
  return current_state_.load();
}

double CognitiveKernel::calculate_resonance(const std::string& input_data) const {
  if (input_data.empty() || echo_identity_.identity_signature.empty()) {
    return 0.0;
  }
  
  // Simple resonance calculation based on string similarity and echo patterns
  std::hash<std::string> hasher;
  size_t input_hash = hasher(input_data);
  size_t identity_hash = hasher(echo_identity_.identity_signature);
  
  // Calculate base resonance
  double base_resonance = 1.0 / (1.0 + std::abs(static_cast<double>(input_hash) - 
                                                static_cast<double>(identity_hash)) / 
                                      std::max(input_hash, identity_hash));
  
  // Apply echo patterns influence
  double pattern_influence = 1.0;
  for (const auto& pattern : echo_identity_.echo_patterns) {
    if (input_data.find(pattern) != std::string::npos) {
      pattern_influence *= 1.2; // Boost resonance for matching patterns
    }
  }
  
  return base_resonance * pattern_influence * echo_identity_.cognitive_weight;
}

// CognitiveProcessor Implementation
CognitiveProcessor::CognitiveProcessor(core::ProcessorMetadata metadata)
  : ProcessorImpl(std::move(metadata)) {
}

void CognitiveProcessor::initialize() {
  ProcessorImpl::initialize();
  
  // Initialize Deep Tree Echo identity for this processor
  DeepTreeEcho echo_identity(getName() + "_cognitive_identity");
  echo_identity.resonance_frequency = std::hash<std::string>{}(getUUIDStr().c_str());
  echo_identity.echo_patterns = {"cognitive", "process", "echo", getName()};
  echo_identity.cognitive_weight = 1.0;
  
  cognitive_kernel_.initialize(echo_identity);
}

void CognitiveProcessor::onTrigger(core::ProcessContext& context, core::ProcessSession& session) {
  try {
    // Set kernel to processing state
    cognitive_kernel_.set_cognitive_state(CognitiveState::PROCESSING);
    
    // Delegate to cognitive processing
    process_with_cognition(context, session, cognitive_kernel_);
    
    // Transition to reflection state
    cognitive_kernel_.set_cognitive_state(CognitiveState::REFLECTING);
    
  } catch (const std::exception& ex) {
    logger_->log_error("Cognitive processing failed: {}", ex.what());
    cognitive_kernel_.set_cognitive_state(CognitiveState::DORMANT);
    throw;
  }
}

}  // namespace org::apache::nifi::minifi::cognitive