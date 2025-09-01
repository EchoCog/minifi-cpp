/**
 * @file HypergraphSubstrate.h
 * @brief Core Layer: Hypergraph Substrate Materialization
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
#include <unordered_set>
#include <vector>
#include <atomic>
#include <mutex>
#include <functional>

#include "utils/CognitiveKernel.h"

namespace org::apache::nifi::minifi::cognitive {

/**
 * @brief Unique identifier for hypergraph nodes
 */
using NodeId = std::string;

/**
 * @brief Unique identifier for hypergraph edges
 */
using EdgeId = std::string;

/**
 * @brief Weight type for hypergraph relationships
 */
using Weight = double;

/**
 * @brief Node in the hypergraph substrate
 */
struct HyperNode {
  NodeId id;
  std::string label;
  std::unordered_map<std::string, std::string> attributes;
  DeepTreeEcho echo_signature;
  Weight activation_level = 0.0;
  
  HyperNode() = default;
  explicit HyperNode(const NodeId& node_id) : id(node_id) {}
  HyperNode(const NodeId& node_id, const std::string& node_label) 
    : id(node_id), label(node_label) {}
};

/**
 * @brief Hyperedge connecting multiple nodes in the substrate
 */
struct HyperEdge {
  EdgeId id;
  std::string label;
  std::unordered_set<NodeId> connected_nodes;
  std::unordered_map<std::string, std::string> attributes;
  Weight strength = 1.0;
  
  HyperEdge() = default;
  explicit HyperEdge(const EdgeId& edge_id) : id(edge_id) {}
  HyperEdge(const EdgeId& edge_id, const std::unordered_set<NodeId>& nodes)
    : id(edge_id), connected_nodes(nodes) {}
};

/**
 * @brief Substrate persistence interface for saving/loading hypergraph state
 */
class SubstratePersistence {
 public:
  virtual ~SubstratePersistence() = default;
  
  virtual bool save_node(const HyperNode& node) = 0;
  virtual bool save_edge(const HyperEdge& edge) = 0;
  virtual bool load_node(const NodeId& id, HyperNode& node) = 0;
  virtual bool load_edge(const EdgeId& id, HyperEdge& edge) = 0;
  virtual bool remove_node(const NodeId& id) = 0;
  virtual bool remove_edge(const EdgeId& id) = 0;
  
  virtual std::vector<NodeId> get_all_node_ids() = 0;
  virtual std::vector<EdgeId> get_all_edge_ids() = 0;
};

/**
 * @brief In-memory substrate persistence implementation
 */
class MemorySubstratePersistence : public SubstratePersistence {
 public:
  bool save_node(const HyperNode& node) override;
  bool save_edge(const HyperEdge& edge) override;
  bool load_node(const NodeId& id, HyperNode& node) override;
  bool load_edge(const EdgeId& id, HyperEdge& edge) override;
  bool remove_node(const NodeId& id) override;
  bool remove_edge(const EdgeId& id) override;
  
  std::vector<NodeId> get_all_node_ids() override;
  std::vector<EdgeId> get_all_edge_ids() override;
  
  void clear();
  size_t node_count() const;
  size_t edge_count() const;

 private:
  mutable std::mutex persistence_mutex_;
  std::unordered_map<NodeId, HyperNode> nodes_;
  std::unordered_map<EdgeId, HyperEdge> edges_;
};

/**
 * @brief Core hypergraph substrate for cognitive relationship mapping
 * 
 * This class implements a hypergraph data structure that can represent
 * complex many-to-many relationships in the cognitive architecture.
 */
class HypergraphSubstrate {
 public:
  explicit HypergraphSubstrate(std::unique_ptr<SubstratePersistence> persistence = nullptr);
  virtual ~HypergraphSubstrate() = default;

  // Node management
  bool add_node(const HyperNode& node);
  bool remove_node(const NodeId& id);
  bool update_node(const HyperNode& node);
  bool get_node(const NodeId& id, HyperNode& node) const;
  bool node_exists(const NodeId& id) const;
  
  // Edge management  
  bool add_edge(const HyperEdge& edge);
  bool remove_edge(const EdgeId& id);
  bool update_edge(const HyperEdge& edge);
  bool get_edge(const EdgeId& id, HyperEdge& edge) const;
  bool edge_exists(const EdgeId& id) const;
  
  // Relationship queries
  std::vector<NodeId> get_connected_nodes(const NodeId& id) const;
  std::vector<EdgeId> get_incident_edges(const NodeId& id) const;
  std::vector<NodeId> get_neighbors_via_edge(const EdgeId& edge_id) const;
  
  // Substrate analysis
  std::vector<NodeId> find_nodes_by_attribute(const std::string& key, const std::string& value) const;
  std::vector<EdgeId> find_edges_by_attribute(const std::string& key, const std::string& value) const;
  std::vector<NodeId> find_nodes_by_echo_pattern(const std::string& pattern) const;
  
  // Activation propagation
  void propagate_activation(const NodeId& source_id, Weight initial_activation);
  void update_node_activation(const NodeId& id, Weight activation);
  Weight get_node_activation(const NodeId& id) const;
  
  // Substrate metrics
  size_t node_count() const;
  size_t edge_count() const;
  double calculate_clustering_coefficient(const NodeId& id) const;
  double calculate_substrate_density() const;
  
  // Persistence operations
  bool save_substrate();
  bool load_substrate();
  void clear_substrate();

 protected:
  /**
   * Activation propagation algorithm - can be overridden
   */
  virtual void on_activation_propagation(const NodeId& source, const std::vector<NodeId>& targets, Weight activation) {}

 private:
  mutable std::mutex substrate_mutex_;
  std::unordered_map<NodeId, HyperNode> nodes_;
  std::unordered_map<EdgeId, HyperEdge> edges_;
  std::unique_ptr<SubstratePersistence> persistence_;
  
  // Internal helper methods
  std::vector<NodeId> get_reachable_nodes(const NodeId& source, int max_hops = 1) const;
  void validate_edge_consistency(const HyperEdge& edge) const;
};

/**
 * @brief Hypergraph-aware cognitive processor
 * 
 * Extends CognitiveProcessor with hypergraph substrate capabilities
 */
class HypergraphProcessor : public CognitiveProcessor {
 public:
  explicit HypergraphProcessor(core::ProcessorMetadata metadata);
  ~HypergraphProcessor() override = default;

  void initialize() override;

 protected:
  /**
   * Access to the hypergraph substrate
   */
  HypergraphSubstrate& get_substrate() { return substrate_; }
  const HypergraphSubstrate& get_substrate() const { return substrate_; }
  
  /**
   * Hypergraph-aware cognitive processing
   */
  virtual void process_with_hypergraph(
    core::ProcessContext& context,
    core::ProcessSession& session, 
    CognitiveKernel& kernel,
    HypergraphSubstrate& substrate) = 0;

  void process_with_cognition(
    core::ProcessContext& context,
    core::ProcessSession& session,
    CognitiveKernel& kernel) override final;

 private:
  HypergraphSubstrate substrate_;
};

}  // namespace org::apache::nifi::minifi::cognitive