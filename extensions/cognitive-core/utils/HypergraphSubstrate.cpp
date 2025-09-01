/**
 * @file HypergraphSubstrate.cpp
 * @brief Core Layer: Hypergraph Substrate Materialization Implementation
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

#include "utils/HypergraphSubstrate.h"
#include "core/ProcessContext.h"
#include "core/ProcessSession.h"
#include <algorithm>
#include <cmath>
#include <queue>
#include <set>

namespace org::apache::nifi::minifi::cognitive {

// MemorySubstratePersistence Implementation
bool MemorySubstratePersistence::save_node(const HyperNode& node) {
  std::lock_guard<std::mutex> lock(persistence_mutex_);
  nodes_[node.id] = node;
  return true;
}

bool MemorySubstratePersistence::save_edge(const HyperEdge& edge) {
  std::lock_guard<std::mutex> lock(persistence_mutex_);
  edges_[edge.id] = edge;
  return true;
}

bool MemorySubstratePersistence::load_node(const NodeId& id, HyperNode& node) {
  std::lock_guard<std::mutex> lock(persistence_mutex_);
  auto it = nodes_.find(id);
  if (it != nodes_.end()) {
    node = it->second;
    return true;
  }
  return false;
}

bool MemorySubstratePersistence::load_edge(const EdgeId& id, HyperEdge& edge) {
  std::lock_guard<std::mutex> lock(persistence_mutex_);
  auto it = edges_.find(id);
  if (it != edges_.end()) {
    edge = it->second;
    return true;
  }
  return false;
}

bool MemorySubstratePersistence::remove_node(const NodeId& id) {
  std::lock_guard<std::mutex> lock(persistence_mutex_);
  return nodes_.erase(id) > 0;
}

bool MemorySubstratePersistence::remove_edge(const EdgeId& id) {
  std::lock_guard<std::mutex> lock(persistence_mutex_);
  return edges_.erase(id) > 0;
}

std::vector<NodeId> MemorySubstratePersistence::get_all_node_ids() {
  std::lock_guard<std::mutex> lock(persistence_mutex_);
  std::vector<NodeId> ids;
  ids.reserve(nodes_.size());
  for (const auto& pair : nodes_) {
    ids.push_back(pair.first);
  }
  return ids;
}

std::vector<EdgeId> MemorySubstratePersistence::get_all_edge_ids() {
  std::lock_guard<std::mutex> lock(persistence_mutex_);
  std::vector<EdgeId> ids;
  ids.reserve(edges_.size());
  for (const auto& pair : edges_) {
    ids.push_back(pair.first);
  }
  return ids;
}

void MemorySubstratePersistence::clear() {
  std::lock_guard<std::mutex> lock(persistence_mutex_);
  nodes_.clear();
  edges_.clear();
}

size_t MemorySubstratePersistence::node_count() const {
  std::lock_guard<std::mutex> lock(persistence_mutex_);
  return nodes_.size();
}

size_t MemorySubstratePersistence::edge_count() const {
  std::lock_guard<std::mutex> lock(persistence_mutex_);
  return edges_.size();
}

// HypergraphSubstrate Implementation
HypergraphSubstrate::HypergraphSubstrate(std::unique_ptr<SubstratePersistence> persistence)
  : persistence_(std::move(persistence)) {
  if (!persistence_) {
    persistence_ = std::make_unique<MemorySubstratePersistence>();
  }
}

bool HypergraphSubstrate::add_node(const HyperNode& node) {
  std::lock_guard<std::mutex> lock(substrate_mutex_);
  
  if (nodes_.find(node.id) != nodes_.end()) {
    return false; // Node already exists
  }
  
  nodes_[node.id] = node;
  persistence_->save_node(node);
  return true;
}

bool HypergraphSubstrate::remove_node(const NodeId& id) {
  std::lock_guard<std::mutex> lock(substrate_mutex_);
  
  // Remove all edges connected to this node
  std::vector<EdgeId> edges_to_remove;
  for (const auto& [edge_id, edge] : edges_) {
    if (edge.connected_nodes.find(id) != edge.connected_nodes.end()) {
      edges_to_remove.push_back(edge_id);
    }
  }
  
  for (const auto& edge_id : edges_to_remove) {
    edges_.erase(edge_id);
    persistence_->remove_edge(edge_id);
  }
  
  bool removed = nodes_.erase(id) > 0;
  if (removed) {
    persistence_->remove_node(id);
  }
  return removed;
}

bool HypergraphSubstrate::update_node(const HyperNode& node) {
  std::lock_guard<std::mutex> lock(substrate_mutex_);
  
  auto it = nodes_.find(node.id);
  if (it == nodes_.end()) {
    return false;
  }
  
  it->second = node;
  persistence_->save_node(node);
  return true;
}

bool HypergraphSubstrate::get_node(const NodeId& id, HyperNode& node) const {
  std::lock_guard<std::mutex> lock(substrate_mutex_);
  
  auto it = nodes_.find(id);
  if (it != nodes_.end()) {
    node = it->second;
    return true;
  }
  return false;
}

bool HypergraphSubstrate::node_exists(const NodeId& id) const {
  std::lock_guard<std::mutex> lock(substrate_mutex_);
  return nodes_.find(id) != nodes_.end();
}

bool HypergraphSubstrate::add_edge(const HyperEdge& edge) {
  std::lock_guard<std::mutex> lock(substrate_mutex_);
  
  if (edges_.find(edge.id) != edges_.end()) {
    return false; // Edge already exists
  }
  
  // Validate that all connected nodes exist
  validate_edge_consistency(edge);
  
  edges_[edge.id] = edge;
  persistence_->save_edge(edge);
  return true;
}

bool HypergraphSubstrate::remove_edge(const EdgeId& id) {
  std::lock_guard<std::mutex> lock(substrate_mutex_);
  
  bool removed = edges_.erase(id) > 0;
  if (removed) {
    persistence_->remove_edge(id);
  }
  return removed;
}

bool HypergraphSubstrate::update_edge(const HyperEdge& edge) {
  std::lock_guard<std::mutex> lock(substrate_mutex_);
  
  auto it = edges_.find(edge.id);
  if (it == edges_.end()) {
    return false;
  }
  
  validate_edge_consistency(edge);
  it->second = edge;
  persistence_->save_edge(edge);
  return true;
}

bool HypergraphSubstrate::get_edge(const EdgeId& id, HyperEdge& edge) const {
  std::lock_guard<std::mutex> lock(substrate_mutex_);
  
  auto it = edges_.find(id);
  if (it != edges_.end()) {
    edge = it->second;
    return true;
  }
  return false;
}

bool HypergraphSubstrate::edge_exists(const EdgeId& id) const {
  std::lock_guard<std::mutex> lock(substrate_mutex_);
  return edges_.find(id) != edges_.end();
}

std::vector<NodeId> HypergraphSubstrate::get_connected_nodes(const NodeId& id) const {
  std::lock_guard<std::mutex> lock(substrate_mutex_);
  
  std::set<NodeId> connected;
  for (const auto& [edge_id, edge] : edges_) {
    if (edge.connected_nodes.find(id) != edge.connected_nodes.end()) {
      for (const auto& node_id : edge.connected_nodes) {
        if (node_id != id) {
          connected.insert(node_id);
        }
      }
    }
  }
  
  return std::vector<NodeId>(connected.begin(), connected.end());
}

std::vector<EdgeId> HypergraphSubstrate::get_incident_edges(const NodeId& id) const {
  std::lock_guard<std::mutex> lock(substrate_mutex_);
  
  std::vector<EdgeId> incident;
  for (const auto& [edge_id, edge] : edges_) {
    if (edge.connected_nodes.find(id) != edge.connected_nodes.end()) {
      incident.push_back(edge_id);
    }
  }
  return incident;
}

std::vector<NodeId> HypergraphSubstrate::get_neighbors_via_edge(const EdgeId& edge_id) const {
  std::lock_guard<std::mutex> lock(substrate_mutex_);
  
  auto it = edges_.find(edge_id);
  if (it != edges_.end()) {
    return std::vector<NodeId>(it->second.connected_nodes.begin(), it->second.connected_nodes.end());
  }
  return {};
}

std::vector<NodeId> HypergraphSubstrate::find_nodes_by_attribute(const std::string& key, const std::string& value) const {
  std::lock_guard<std::mutex> lock(substrate_mutex_);
  
  std::vector<NodeId> matches;
  for (const auto& [node_id, node] : nodes_) {
    auto attr_it = node.attributes.find(key);
    if (attr_it != node.attributes.end() && attr_it->second == value) {
      matches.push_back(node_id);
    }
  }
  return matches;
}

std::vector<EdgeId> HypergraphSubstrate::find_edges_by_attribute(const std::string& key, const std::string& value) const {
  std::lock_guard<std::mutex> lock(substrate_mutex_);
  
  std::vector<EdgeId> matches;
  for (const auto& [edge_id, edge] : edges_) {
    auto attr_it = edge.attributes.find(key);
    if (attr_it != edge.attributes.end() && attr_it->second == value) {
      matches.push_back(edge_id);
    }
  }
  return matches;
}

std::vector<NodeId> HypergraphSubstrate::find_nodes_by_echo_pattern(const std::string& pattern) const {
  std::lock_guard<std::mutex> lock(substrate_mutex_);
  
  std::vector<NodeId> matches;
  for (const auto& [node_id, node] : nodes_) {
    for (const auto& echo_pattern : node.echo_signature.echo_patterns) {
      if (echo_pattern == pattern) {
        matches.push_back(node_id);
        break;
      }
    }
  }
  return matches;
}

void HypergraphSubstrate::propagate_activation(const NodeId& source_id, Weight initial_activation) {
  std::lock_guard<std::mutex> lock(substrate_mutex_);
  
  // Set initial activation for source node
  auto source_it = nodes_.find(source_id);
  if (source_it == nodes_.end()) {
    return;
  }
  
  source_it->second.activation_level = initial_activation;
  
  // Get reachable nodes and propagate activation
  auto reachable = get_reachable_nodes(source_id, 2); // Propagate up to 2 hops
  
  for (const auto& target_id : reachable) {
    auto target_it = nodes_.find(target_id);
    if (target_it != nodes_.end()) {
      // Simple decay model - activation decreases with distance
      Weight propagated_activation = initial_activation * 0.7; // 70% retention
      target_it->second.activation_level += propagated_activation;
    }
  }
  
  // Notify subclasses
  on_activation_propagation(source_id, reachable, initial_activation);
}

void HypergraphSubstrate::update_node_activation(const NodeId& id, Weight activation) {
  std::lock_guard<std::mutex> lock(substrate_mutex_);
  
  auto it = nodes_.find(id);
  if (it != nodes_.end()) {
    it->second.activation_level = activation;
  }
}

Weight HypergraphSubstrate::get_node_activation(const NodeId& id) const {
  std::lock_guard<std::mutex> lock(substrate_mutex_);
  
  auto it = nodes_.find(id);
  return (it != nodes_.end()) ? it->second.activation_level : 0.0;
}

size_t HypergraphSubstrate::node_count() const {
  std::lock_guard<std::mutex> lock(substrate_mutex_);
  return nodes_.size();
}

size_t HypergraphSubstrate::edge_count() const {
  std::lock_guard<std::mutex> lock(substrate_mutex_);
  return edges_.size();
}

double HypergraphSubstrate::calculate_clustering_coefficient(const NodeId& id) const {
  std::lock_guard<std::mutex> lock(substrate_mutex_);
  
  auto neighbors = get_connected_nodes(id);
  if (neighbors.size() < 2) {
    return 0.0;
  }
  
  size_t possible_edges = neighbors.size() * (neighbors.size() - 1) / 2;
  size_t actual_edges = 0;
  
  // Count edges between neighbors
  for (size_t i = 0; i < neighbors.size(); ++i) {
    for (size_t j = i + 1; j < neighbors.size(); ++j) {
      auto neighbor_connections = get_connected_nodes(neighbors[i]);
      if (std::find(neighbor_connections.begin(), neighbor_connections.end(), neighbors[j]) != neighbor_connections.end()) {
        actual_edges++;
      }
    }
  }
  
  return static_cast<double>(actual_edges) / possible_edges;
}

double HypergraphSubstrate::calculate_substrate_density() const {
  std::lock_guard<std::mutex> lock(substrate_mutex_);
  
  if (nodes_.size() < 2) {
    return 0.0;
  }
  
  size_t possible_connections = nodes_.size() * (nodes_.size() - 1) / 2;
  return static_cast<double>(edges_.size()) / possible_connections;
}

bool HypergraphSubstrate::save_substrate() {
  std::lock_guard<std::mutex> lock(substrate_mutex_);
  
  // Save all nodes and edges through persistence interface
  for (const auto& [node_id, node] : nodes_) {
    if (!persistence_->save_node(node)) {
      return false;
    }
  }
  
  for (const auto& [edge_id, edge] : edges_) {
    if (!persistence_->save_edge(edge)) {
      return false;
    }
  }
  
  return true;
}

bool HypergraphSubstrate::load_substrate() {
  std::lock_guard<std::mutex> lock(substrate_mutex_);
  
  // Clear current state
  nodes_.clear();
  edges_.clear();
  
  // Load all nodes
  auto node_ids = persistence_->get_all_node_ids();
  for (const auto& node_id : node_ids) {
    HyperNode node;
    if (persistence_->load_node(node_id, node)) {
      nodes_[node_id] = node;
    }
  }
  
  // Load all edges
  auto edge_ids = persistence_->get_all_edge_ids();
  for (const auto& edge_id : edge_ids) {
    HyperEdge edge;
    if (persistence_->load_edge(edge_id, edge)) {
      edges_[edge_id] = edge;
    }
  }
  
  return true;
}

void HypergraphSubstrate::clear_substrate() {
  std::lock_guard<std::mutex> lock(substrate_mutex_);
  nodes_.clear();
  edges_.clear();
  if (auto* memory_persistence = dynamic_cast<MemorySubstratePersistence*>(persistence_.get())) {
    memory_persistence->clear();
  }
}

std::vector<NodeId> HypergraphSubstrate::get_reachable_nodes(const NodeId& source, int max_hops) const {
  std::vector<NodeId> reachable;
  std::set<NodeId> visited;
  std::queue<std::pair<NodeId, int>> queue;
  
  queue.push({source, 0});
  visited.insert(source);
  
  while (!queue.empty()) {
    auto [current_id, hops] = queue.front();
    queue.pop();
    
    if (hops >= max_hops) {
      continue;
    }
    
    // Find all directly connected nodes
    for (const auto& [edge_id, edge] : edges_) {
      if (edge.connected_nodes.find(current_id) != edge.connected_nodes.end()) {
        for (const auto& neighbor_id : edge.connected_nodes) {
          if (neighbor_id != current_id && visited.find(neighbor_id) == visited.end()) {
            visited.insert(neighbor_id);
            queue.push({neighbor_id, hops + 1});
            reachable.push_back(neighbor_id);
          }
        }
      }
    }
  }
  
  return reachable;
}

void HypergraphSubstrate::validate_edge_consistency(const HyperEdge& edge) const {
  for (const auto& node_id : edge.connected_nodes) {
    if (nodes_.find(node_id) == nodes_.end()) {
      throw std::invalid_argument("Edge references non-existent node: " + node_id);
    }
  }
}

// HypergraphProcessor Implementation
HypergraphProcessor::HypergraphProcessor(core::ProcessorMetadata metadata)
  : CognitiveProcessor(std::move(metadata)), substrate_() {
}

void HypergraphProcessor::initialize() {
  CognitiveProcessor::initialize();
  
  // Initialize substrate with cognitive kernel identity
  const auto& echo_identity = get_cognitive_kernel().get_echo_identity();
  
  // Create a root node for this processor in the substrate
  HyperNode processor_node(getName() + "_root", "Processor Root Node");
  processor_node.echo_signature = echo_identity;
  processor_node.attributes["processor_type"] = "HypergraphProcessor";
  processor_node.attributes["processor_uuid"] = getUUIDStr().c_str();
  
  substrate_.add_node(processor_node);
}

void HypergraphProcessor::process_with_cognition(
    core::ProcessContext& context,
    core::ProcessSession& session, 
    CognitiveKernel& kernel) {
  process_with_hypergraph(context, session, kernel, substrate_);
}

}  // namespace org::apache::nifi::minifi::cognitive