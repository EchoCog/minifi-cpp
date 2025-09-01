/**
 * @file HypergraphMapperProcessor.h
 * @brief Core Layer: Hypergraph Relationship Mapping Processor
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

#include "utils/HypergraphSubstrate.h"
#include "core/PropertyDefinition.h"
#include "core/PropertyDefinitionBuilder.h"
#include "core/RelationshipDefinition.h"
#include "core/Resource.h"
#include "utils/Id.h"

namespace org::apache::nifi::minifi::cognitive::processors {

/**
 * @brief Hypergraph Mapper Processor - Advanced relationship mapping and analysis
 * 
 * This processor demonstrates hypergraph substrate capabilities by:
 * - Creating nodes from FlowFile content and attributes
 * - Establishing hypergraph relationships between data elements
 * - Performing substrate analysis and pattern detection
 * - Propagating cognitive activation through the substrate
 * - Generating relationship metadata and insights
 */
class HypergraphMapperProcessor : public HypergraphProcessor {
 public:
  explicit HypergraphMapperProcessor(core::ProcessorMetadata metadata);
  ~HypergraphMapperProcessor() override = default;

  EXTENSIONAPI static constexpr const char* Description = "Maps data relationships using hypergraph substrate, "
      "creating complex many-to-many associations and enabling advanced cognitive pattern analysis.";

  EXTENSIONAPI static constexpr auto Properties = std::array<core::PropertyReference, 0>{{}};

  EXTENSIONAPI static constexpr auto Relationships = std::array<core::RelationshipDefinition, 4>{{
      {"mapped", "FlowFiles with successfully mapped relationships"},
      {"enhanced", "FlowFiles with enhanced relationship metadata"},
      {"clustered", "FlowFiles identified as part of significant clusters"},
      {"isolated", "FlowFiles with minimal or no relationships"}
  }};

  EXTENSIONAPI static constexpr bool SupportsDynamicProperties = true;
  EXTENSIONAPI static constexpr bool SupportsDynamicRelationships = false;
  EXTENSIONAPI static constexpr core::annotation::Input InputRequirement = core::annotation::Input::INPUT_REQUIRED;
  EXTENSIONAPI static constexpr bool IsSingleThreaded = false;

  ADD_COMMON_VIRTUAL_FUNCTIONS_FOR_PROCESSORS

  // Define relationship constants
  static constexpr auto Mapped = core::RelationshipDefinition{"mapped", "FlowFiles with successfully mapped relationships"};
  static constexpr auto Enhanced = core::RelationshipDefinition{"enhanced", "FlowFiles with enhanced relationship metadata"};
  static constexpr auto Clustered = core::RelationshipDefinition{"clustered", "FlowFiles identified as part of significant clusters"};
  static constexpr auto Isolated = core::RelationshipDefinition{"isolated", "FlowFiles with minimal or no relationships"};

 protected:
  void process_with_hypergraph(
      core::ProcessContext& context,
      core::ProcessSession& session,
      CognitiveKernel& kernel,
      HypergraphSubstrate& substrate) override;

 private:
  void create_content_node(const std::string& content, const std::string& flow_id, HypergraphSubstrate& substrate);
  void create_attribute_nodes(const core::FlowFile& flow_file, const std::string& flow_id, HypergraphSubstrate& substrate);
  void analyze_relationships(const std::string& flow_id, HypergraphSubstrate& substrate, core::FlowFile& flow_file);
  void add_hypergraph_attributes(core::FlowFile& flow_file, const HypergraphSubstrate& substrate, 
                                const std::string& node_id) const;
  
  std::atomic<uint64_t> node_counter_{0};
  std::atomic<uint64_t> edge_counter_{0};
};

}  // namespace org::apache::nifi::minifi::cognitive::processors