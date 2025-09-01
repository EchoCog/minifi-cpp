/**
 * @file HypergraphMapperProcessor.cpp
 * @brief Core Layer: Hypergraph Relationship Mapping Processor Implementation
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

#include "processors/HypergraphMapperProcessor.h"
#include "core/ProcessContext.h"
#include "core/ProcessSession.h"
#include "core/FlowFile.h"
#include "core/Resource.h"
#include "utils/Id.h"
#include "io/InputStream.h"
#include <string>
#include <chrono>
#include <sstream>
#include <algorithm>

namespace org::apache::nifi::minifi::cognitive::processors {

HypergraphMapperProcessor::HypergraphMapperProcessor(core::ProcessorMetadata metadata)
    : HypergraphProcessor(std::move(metadata)) {
}

void HypergraphMapperProcessor::process_with_hypergraph(
    core::ProcessContext& context,
    core::ProcessSession& session,
    CognitiveKernel& kernel,
    HypergraphSubstrate& substrate) {
  
  auto flow_file = session.get();
  if (!flow_file) {
    return;
  }

  try {
    // Read FlowFile content
    std::string content;
    session.read(flow_file, [&content](const std::shared_ptr<io::InputStream>& stream) -> int64_t {
      std::array<std::byte, 1024> buffer{};
      int64_t total_read = 0;
      size_t bytes_read;
      
      while ((bytes_read = stream->read(buffer)) > 0) {
        content.append(reinterpret_cast<const char*>(buffer.data()), bytes_read);
        total_read += bytes_read;
      }
      return total_read;
    });

    // Process through cognitive kernel
    kernel.process_cognitive_signal(content);
    
    // Generate unique identifier for this FlowFile in the substrate
    std::string flow_node_id = flow_file->getUUIDStr().c_str();
    
    // Create nodes in the hypergraph substrate
    create_content_node(content, flow_node_id, substrate);
    create_attribute_nodes(*flow_file, flow_node_id, substrate);
    
    // Analyze relationships and clustering
    analyze_relationships(flow_node_id, substrate, *flow_file);
    
    // Add comprehensive hypergraph metadata
    add_hypergraph_attributes(*flow_file, substrate, flow_node_id);
    
    // Determine routing based on hypergraph analysis
    double clustering_coeff = substrate.calculate_clustering_coefficient(flow_node_id);
    size_t connection_count = substrate.get_connected_nodes(flow_node_id).size();
    
    if (clustering_coeff > 0.5 && connection_count >= 3) {
      logger_->log_debug("FlowFile {} identified as highly clustered (coeff: {:.3f}, connections: {})", 
                        flow_file->getUUIDStr(), clustering_coeff, connection_count);
      session.transfer(flow_file, Clustered);
    } else if (connection_count >= 2) {
      logger_->log_debug("FlowFile {} has moderate connectivity (connections: {})", 
                        flow_file->getUUIDStr(), connection_count);
      session.transfer(flow_file, Enhanced);
    } else if (connection_count >= 1) {
      logger_->log_debug("FlowFile {} successfully mapped with {} connections", 
                        flow_file->getUUIDStr(), connection_count);
      session.transfer(flow_file, Mapped);
    } else {
      logger_->log_debug("FlowFile {} appears isolated with no significant connections", 
                        flow_file->getUUIDStr());
      session.transfer(flow_file, Isolated);
    }

  } catch (const std::exception& ex) {
    logger_->log_error("Failed to process FlowFile {} through hypergraph mapper: {}", 
                      flow_file->getUUIDStr(), ex.what());
    session.transfer(flow_file, Mapped); // Transfer to mapped to avoid infinite loop
  }
}

void HypergraphMapperProcessor::create_content_node(const std::string& content, const std::string& flow_id, HypergraphSubstrate& substrate) {
  // Create primary content node
  HyperNode content_node(flow_id + "_content", "FlowFile Content");
  content_node.echo_signature = get_cognitive_kernel().get_echo_identity();
  content_node.attributes["type"] = "content";
  content_node.attributes["flow_id"] = flow_id;
  content_node.attributes["content_length"] = std::to_string(content.length());
  content_node.attributes["content_hash"] = std::to_string(std::hash<std::string>{}(content));
  
  substrate.add_node(content_node);
  
  // Create nodes for significant words (simple tokenization)
  std::istringstream iss(content);
  std::string word;
  std::unordered_set<std::string> unique_words;
  
  while (iss >> word) {
    // Simple preprocessing - remove punctuation and convert to lowercase
    word.erase(std::remove_if(word.begin(), word.end(), ::ispunct), word.end());
    std::transform(word.begin(), word.end(), word.begin(), ::tolower);
    
    if (word.length() > 3) { // Only consider significant words
      unique_words.insert(word);
    }
  }
  
  // Create word nodes and edges
  std::unordered_set<NodeId> content_related_nodes;
  content_related_nodes.insert(flow_id + "_content");
  
  for (const auto& unique_word : unique_words) {
    NodeId word_node_id = "word_" + unique_word;
    
    HyperNode word_node(word_node_id, "Word: " + unique_word);
    word_node.attributes["type"] = "word";
    word_node.attributes["word"] = unique_word;
    word_node.activation_level = 0.1; // Base activation for words
    
    if (substrate.add_node(word_node)) {
      content_related_nodes.insert(word_node_id);
    }
  }
  
  // Create hyperedge connecting content to its words
  if (content_related_nodes.size() > 1) {
    EdgeId content_edge_id = flow_id + "_content_edge_" + std::to_string(edge_counter_.fetch_add(1));
    HyperEdge content_edge(content_edge_id, content_related_nodes);
    content_edge.label = "Content-Word Relationship";
    content_edge.attributes["type"] = "contains_words";
    content_edge.strength = 1.0;
    
    substrate.add_edge(content_edge);
  }
}

void HypergraphMapperProcessor::create_attribute_nodes(const core::FlowFile& flow_file, const std::string& flow_id, HypergraphSubstrate& substrate) {
  // Create nodes for FlowFile attributes
  const auto& attributes = flow_file.getAttributes();
  std::unordered_set<NodeId> attribute_nodes;
  
  for (const auto& [attr_name, attr_value] : attributes) {
    if (attr_name.find("cognitive.") != 0) { // Skip cognitive attributes to avoid cycles
      NodeId attr_node_id = flow_id + "_attr_" + attr_name;
      
      HyperNode attr_node(attr_node_id, "Attribute: " + attr_name);
      attr_node.attributes["type"] = "attribute";
      attr_node.attributes["name"] = attr_name;
      attr_node.attributes["value"] = attr_value;
      attr_node.activation_level = 0.2; // Moderate activation for attributes
      
      if (substrate.add_node(attr_node)) {
        attribute_nodes.insert(attr_node_id);
        
        // Create edge to main flow node
        std::unordered_set<NodeId> attr_connection{flow_id + "_content", attr_node_id};
        EdgeId attr_edge_id = flow_id + "_attr_edge_" + attr_name;
        HyperEdge attr_edge(attr_edge_id, attr_connection);
        attr_edge.label = "FlowFile-Attribute Relationship";
        attr_edge.attributes["type"] = "has_attribute";
        attr_edge.attributes["attribute_name"] = attr_name;
        attr_edge.strength = 0.8;
        
        substrate.add_edge(attr_edge);
      }
    }
  }
  
  // Create hyperedge connecting all attributes if there are multiple
  if (attribute_nodes.size() > 1) {
    attribute_nodes.insert(flow_id + "_content"); // Include main content node
    EdgeId attr_cluster_id = flow_id + "_attribute_cluster_" + std::to_string(edge_counter_.fetch_add(1));
    HyperEdge attr_cluster_edge(attr_cluster_id, attribute_nodes);
    attr_cluster_edge.label = "Attribute Cluster";
    attr_cluster_edge.attributes["type"] = "attribute_cluster";
    attr_cluster_edge.strength = 0.6;
    
    substrate.add_edge(attr_cluster_edge);
  }
}

void HypergraphMapperProcessor::analyze_relationships(const std::string& flow_id, HypergraphSubstrate& substrate, core::FlowFile& flow_file) {
  NodeId main_node_id = flow_id + "_content";
  
  // Propagate activation from this node
  substrate.propagate_activation(main_node_id, 1.0);
  
  // Look for similar nodes based on attributes
  auto similar_content_nodes = substrate.find_nodes_by_attribute("type", "content");
  auto similar_words = substrate.find_nodes_by_attribute("type", "word");
  
  // Create relationships with similar content
  std::unordered_set<NodeId> similar_cluster;
  similar_cluster.insert(main_node_id);
  
  for (const auto& similar_node : similar_content_nodes) {
    if (similar_node != main_node_id) {
      // Check if there are common words
      auto this_neighbors = substrate.get_connected_nodes(main_node_id);
      auto other_neighbors = substrate.get_connected_nodes(similar_node);
      
      std::set<NodeId> common_neighbors;
      std::set_intersection(this_neighbors.begin(), this_neighbors.end(),
                           other_neighbors.begin(), other_neighbors.end(),
                           std::inserter(common_neighbors, common_neighbors.begin()));
      
      if (common_neighbors.size() >= 2) { // At least 2 common words
        similar_cluster.insert(similar_node);
      }
    }
  }
  
  // Create similarity hyperedge if we found similar content
  if (similar_cluster.size() > 1) {
    EdgeId similarity_edge_id = flow_id + "_similarity_" + std::to_string(edge_counter_.fetch_add(1));
    HyperEdge similarity_edge(similarity_edge_id, similar_cluster);
    similarity_edge.label = "Content Similarity";
    similarity_edge.attributes["type"] = "similarity";
    similarity_edge.attributes["similarity_type"] = "content_overlap";
    similarity_edge.strength = 0.9;
    
    substrate.add_edge(similarity_edge);
    
    // Update flow file with similarity information
    flow_file.addAttribute("hypergraph.similar_content_count", std::to_string(similar_cluster.size() - 1));
  }
}

void HypergraphMapperProcessor::add_hypergraph_attributes(core::FlowFile& flow_file, const HypergraphSubstrate& substrate, 
                                                         const std::string& node_id) const {
  // Basic substrate metrics
  flow_file.addAttribute("hypergraph.total_nodes", std::to_string(substrate.node_count()));
  flow_file.addAttribute("hypergraph.total_edges", std::to_string(substrate.edge_count()));
  flow_file.addAttribute("hypergraph.substrate_density", std::to_string(substrate.calculate_substrate_density()));
  
  // Node-specific metrics
  NodeId main_node_id = node_id + "_content";
  auto connected_nodes = substrate.get_connected_nodes(main_node_id);
  auto incident_edges = substrate.get_incident_edges(main_node_id);
  
  flow_file.addAttribute("hypergraph.node_connections", std::to_string(connected_nodes.size()));
  flow_file.addAttribute("hypergraph.incident_edges", std::to_string(incident_edges.size()));
  flow_file.addAttribute("hypergraph.clustering_coefficient", 
                        std::to_string(substrate.calculate_clustering_coefficient(main_node_id)));
  flow_file.addAttribute("hypergraph.node_activation", 
                        std::to_string(substrate.get_node_activation(main_node_id)));
  
  // Relationship details
  if (!connected_nodes.empty()) {
    std::ostringstream connected_stream;
    for (size_t i = 0; i < connected_nodes.size() && i < 10; ++i) { // Limit to first 10
      if (i > 0) connected_stream << ",";
      connected_stream << connected_nodes[i];
    }
    flow_file.addAttribute("hypergraph.connected_nodes", connected_stream.str());
  }
  
  // Timestamp for analysis
  flow_file.addAttribute("hypergraph.analysis_timestamp", 
                        std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now().time_since_epoch()).count()));
  
  // Processor identification
  flow_file.addAttribute("hypergraph.processor", "HypergraphMapperProcessor");
  flow_file.addAttribute("hypergraph.processor_uuid", getUUIDStr().c_str());
}

REGISTER_RESOURCE(HypergraphMapperProcessor, Processor);

}  // namespace org::apache::nifi::minifi::cognitive::processors