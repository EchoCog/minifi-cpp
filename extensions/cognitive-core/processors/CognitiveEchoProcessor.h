/**
 * @file CognitiveEchoProcessor.h
 * @brief Foundation Layer: Basic Cognitive Echo Processing
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

#include "utils/CognitiveKernel.h"
#include "core/PropertyDefinition.h"
#include "core/PropertyDefinitionBuilder.h"
#include "core/RelationshipDefinition.h"
#include "core/Resource.h"
#include "utils/Id.h"

namespace org::apache::nifi::minifi::cognitive::processors {

/**
 * @brief Cognitive Echo Processor - Demonstrates basic cognitive processing
 * 
 * This processor implements the foundational cognitive capabilities by:
 * - Processing FlowFile content through the cognitive kernel
 * - Computing resonance with Deep Tree Echo identity
 * - Generating cognitive metadata
 * - Routing based on cognitive resonance levels
 */
class CognitiveEchoProcessor : public CognitiveProcessor {
 public:
  explicit CognitiveEchoProcessor(core::ProcessorMetadata metadata);
  ~CognitiveEchoProcessor() override = default;

  EXTENSIONAPI static constexpr const char* Description = "Processes data through cognitive echo patterns, "
      "integrating Deep Tree Echo identity for cognitive-aware data flow routing.";

  EXTENSIONAPI static constexpr auto Properties = std::array<core::PropertyReference, 0>{{}};

  EXTENSIONAPI static constexpr auto Relationships = std::array<core::RelationshipDefinition, 3>{{
      {"success", "FlowFiles that are successfully processed through cognitive echo"},
      {"high-resonance", "FlowFiles with high cognitive resonance"},
      {"low-resonance", "FlowFiles with low cognitive resonance"}
  }};

  EXTENSIONAPI static constexpr bool SupportsDynamicProperties = true;
  EXTENSIONAPI static constexpr bool SupportsDynamicRelationships = false;
  EXTENSIONAPI static constexpr core::annotation::Input InputRequirement = core::annotation::Input::INPUT_REQUIRED;
  EXTENSIONAPI static constexpr bool IsSingleThreaded = false;

  ADD_COMMON_VIRTUAL_FUNCTIONS_FOR_PROCESSORS

  // Define relationship constants
  static constexpr auto Success = core::RelationshipDefinition{"success", "FlowFiles that are successfully processed through cognitive echo"};
  static constexpr auto HighResonance = core::RelationshipDefinition{"high-resonance", "FlowFiles with high cognitive resonance"};
  static constexpr auto LowResonance = core::RelationshipDefinition{"low-resonance", "FlowFiles with low cognitive resonance"};

 protected:
  void process_with_cognition(
      core::ProcessContext& context,
      core::ProcessSession& session,
      CognitiveKernel& kernel) override;

 private:
  void setup_properties();
  void add_cognitive_attributes(core::FlowFile& flow_file, double resonance, 
                               const std::string& cognitive_state) const;

  // Property values
  std::string echo_pattern_;
  double resonance_threshold_{0.5};
  bool memory_store_enabled_{true};
};

}  // namespace org::apache::nifi::minifi::cognitive::processors