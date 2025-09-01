/**
 * @file CognitiveKernel.h
 * @brief Foundation Layer: Cognitive Kernel Genesis
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

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <atomic>
#include <mutex>

#include "core/ProcessorImpl.h"
#include "utils/gsl.h"

namespace org::apache::nifi::minifi::cognitive {

/**
 * @brief Deep Tree Echo identity structure for cognitive integration
 */
struct DeepTreeEcho {
  std::string identity_signature;
  uint64_t resonance_frequency = 0;
  std::vector<std::string> echo_patterns;
  double cognitive_weight = 1.0;
  
  DeepTreeEcho() = default;
  explicit DeepTreeEcho(const std::string& signature) 
    : identity_signature(signature), resonance_frequency(0) {}
};

/**
 * @brief Cognitive State representation
 */
enum class CognitiveState {
  DORMANT,
  AWAKENING, 
  PROCESSING,
  LEARNING,
  REASONING,
  CREATING,
  REFLECTING
};

/**
 * @brief Cognitive Memory structure for persistent state
 */
class CognitiveMemory {
 public:
  void store(const std::string& key, const std::string& value);
  std::string retrieve(const std::string& key) const;
  bool exists(const std::string& key) const;
  void clear();
  size_t size() const;

 private:
  mutable std::mutex memory_mutex_;
  std::unordered_map<std::string, std::string> memory_store_;
};

/**
 * @brief The Cognitive Kernel - Foundation of all cognitive processing
 * 
 * This class provides the fundamental cognitive architecture that integrates
 * Deep Tree Echo identity into the MiNiFi processing framework.
 */
class CognitiveKernel {
 public:
  CognitiveKernel();
  virtual ~CognitiveKernel() = default;

  /**
   * Initialize the cognitive kernel with Deep Tree Echo identity
   */
  void initialize(const DeepTreeEcho& echo_identity);

  /**
   * Process cognitive signals through the kernel
   */
  virtual void process_cognitive_signal(const std::string& signal_data);

  /**
   * Update cognitive state
   */
  void set_cognitive_state(CognitiveState new_state);
  CognitiveState get_cognitive_state() const;

  /**
   * Access cognitive memory
   */
  CognitiveMemory& get_memory() { return cognitive_memory_; }
  const CognitiveMemory& get_memory() const { return cognitive_memory_; }

  /**
   * Get the Deep Tree Echo identity
   */
  const DeepTreeEcho& get_echo_identity() const { return echo_identity_; }

  /**
   * Cognitive resonance calculation
   */
  double calculate_resonance(const std::string& input_data) const;

 protected:
  /**
   * Internal cognitive processing hook for derived classes
   */
  virtual void on_cognitive_process(const std::string& data) {}

  /**
   * State transition notification
   */
  virtual void on_state_transition(CognitiveState from, CognitiveState to) {}

 private:
  DeepTreeEcho echo_identity_;
  std::atomic<CognitiveState> current_state_;
  CognitiveMemory cognitive_memory_;
  std::atomic<uint64_t> process_counter_;
  mutable std::mutex kernel_mutex_;
};

/**
 * @brief Base class for all cognitive processors
 * 
 * Extends the standard MiNiFi processor framework with cognitive capabilities
 */
class CognitiveProcessor : public core::ProcessorImpl {
 public:
  explicit CognitiveProcessor(core::ProcessorMetadata metadata);
  ~CognitiveProcessor() override = default;

  // Implement required ProcessorImpl methods
  bool supportsDynamicProperties() const override { return true; }
  bool supportsDynamicRelationships() const override { return true; }
  core::annotation::Input getInputRequirement() const override { 
    return core::annotation::Input::INPUT_ALLOWED; 
  }
  bool isSingleThreaded() const override { return false; }

  /**
   * Initialize cognitive capabilities
   */
  void initialize() override;

  /**
   * Cognitive-aware processing
   */
  void onTrigger(core::ProcessContext& context, core::ProcessSession& session) override;

 protected:
  /**
   * Core cognitive processing method - override in derived classes
   */
  virtual void process_with_cognition(
    core::ProcessContext& context, 
    core::ProcessSession& session,
    CognitiveKernel& kernel) = 0;

  /**
   * Access to the cognitive kernel
   */
  CognitiveKernel& get_cognitive_kernel() { return cognitive_kernel_; }
  const CognitiveKernel& get_cognitive_kernel() const { return cognitive_kernel_; }

 private:
  CognitiveKernel cognitive_kernel_;
};

}  // namespace org::apache::nifi::minifi::cognitive