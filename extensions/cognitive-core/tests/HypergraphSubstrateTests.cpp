/**
 * @file HypergraphSubstrateTests.cpp
 * @brief Test suite for Hypergraph Substrate Core Layer
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
#include "processors/HypergraphMapperProcessor.h"
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <memory>
#include <unordered_set>

using namespace org::apache::nifi::minifi::cognitive;
using namespace org::apache::nifi::minifi::cognitive::processors;

TEST_CASE("HyperNode creation and manipulation", "[hypergraph][node]") {
  HyperNode node;
  
  REQUIRE(node.id.empty());
  REQUIRE(node.label.empty());
  REQUIRE(node.attributes.empty());
  REQUIRE(node.activation_level == 0.0);
  
  HyperNode node_with_id("test_node");
  REQUIRE(node_with_id.id == "test_node");
  REQUIRE(node_with_id.label.empty());
  
  HyperNode node_with_label("test_node", "Test Node Label");
  REQUIRE(node_with_label.id == "test_node");
  REQUIRE(node_with_label.label == "Test Node Label");
}

TEST_CASE("HyperEdge creation and manipulation", "[hypergraph][edge]") {
  HyperEdge edge;
  
  REQUIRE(edge.id.empty());
  REQUIRE(edge.label.empty());
  REQUIRE(edge.connected_nodes.empty());
  REQUIRE(edge.strength == 1.0);
  
  std::unordered_set<NodeId> nodes = {"node1", "node2", "node3"};
  HyperEdge edge_with_nodes("edge1", nodes);
  
  REQUIRE(edge_with_nodes.id == "edge1");
  REQUIRE(edge_with_nodes.connected_nodes == nodes);
  REQUIRE(edge_with_nodes.strength == 1.0);
}

TEST_CASE("MemorySubstratePersistence functionality", "[hypergraph][persistence]") {
  MemorySubstratePersistence persistence;
  
  REQUIRE(persistence.node_count() == 0);
  REQUIRE(persistence.edge_count() == 0);
  
  HyperNode node("test_node", "Test Node");
  node.attributes["key1"] = "value1";
  
  REQUIRE(persistence.save_node(node));
  REQUIRE(persistence.node_count() == 1);
  
  HyperNode loaded_node;
  REQUIRE(persistence.load_node("test_node", loaded_node));
  REQUIRE(loaded_node.id == "test_node");
  REQUIRE(loaded_node.label == "Test Node");
  REQUIRE(loaded_node.attributes["key1"] == "value1");
  
  HyperNode missing_node;
  REQUIRE_FALSE(persistence.load_node("missing", missing_node));
  
  std::unordered_set<NodeId> edge_nodes = {"test_node"};
  HyperEdge edge("test_edge", edge_nodes);
  
  REQUIRE(persistence.save_edge(edge));
  REQUIRE(persistence.edge_count() == 1);
  
  HyperEdge loaded_edge;
  REQUIRE(persistence.load_edge("test_edge", loaded_edge));
  REQUIRE(loaded_edge.id == "test_edge");
  REQUIRE(loaded_edge.connected_nodes == edge_nodes);
  
  auto node_ids = persistence.get_all_node_ids();
  REQUIRE(node_ids.size() == 1);
  REQUIRE(node_ids[0] == "test_node");
  
  auto edge_ids = persistence.get_all_edge_ids();
  REQUIRE(edge_ids.size() == 1);
  REQUIRE(edge_ids[0] == "test_edge");
  
  REQUIRE(persistence.remove_node("test_node"));
  REQUIRE(persistence.node_count() == 0);
  
  REQUIRE(persistence.remove_edge("test_edge"));
  REQUIRE(persistence.edge_count() == 0);
}

TEST_CASE("HypergraphSubstrate basic operations", "[hypergraph][substrate]") {
  HypergraphSubstrate substrate;
  
  REQUIRE(substrate.node_count() == 0);
  REQUIRE(substrate.edge_count() == 0);
  
  // Add nodes
  HyperNode node1("node1", "First Node");
  HyperNode node2("node2", "Second Node");
  HyperNode node3("node3", "Third Node");
  
  REQUIRE(substrate.add_node(node1));
  REQUIRE(substrate.add_node(node2));
  REQUIRE(substrate.add_node(node3));
  REQUIRE(substrate.node_count() == 3);
  
  // Cannot add duplicate nodes
  REQUIRE_FALSE(substrate.add_node(node1));
  
  // Check node existence
  REQUIRE(substrate.node_exists("node1"));
  REQUIRE_FALSE(substrate.node_exists("nonexistent"));
  
  // Retrieve nodes
  HyperNode retrieved_node;
  REQUIRE(substrate.get_node("node1", retrieved_node));
  REQUIRE(retrieved_node.id == "node1");
  REQUIRE(retrieved_node.label == "First Node");
  
  REQUIRE_FALSE(substrate.get_node("missing", retrieved_node));
}

TEST_CASE("HypergraphSubstrate edge operations", "[hypergraph][substrate][edges]") {
  HypergraphSubstrate substrate;
  
  // First add nodes
  HyperNode node1("node1", "First Node");
  HyperNode node2("node2", "Second Node");
  HyperNode node3("node3", "Third Node");
  
  substrate.add_node(node1);
  substrate.add_node(node2);
  substrate.add_node(node3);
  
  // Add edges
  std::unordered_set<NodeId> edge1_nodes = {"node1", "node2"};
  std::unordered_set<NodeId> edge2_nodes = {"node1", "node2", "node3"};
  
  HyperEdge edge1("edge1", edge1_nodes);
  HyperEdge edge2("edge2", edge2_nodes);
  
  REQUIRE(substrate.add_edge(edge1));
  REQUIRE(substrate.add_edge(edge2));
  REQUIRE(substrate.edge_count() == 2);
  
  // Cannot add duplicate edges
  REQUIRE_FALSE(substrate.add_edge(edge1));
  
  // Test edge retrieval
  HyperEdge retrieved_edge;
  REQUIRE(substrate.get_edge("edge1", retrieved_edge));
  REQUIRE(retrieved_edge.id == "edge1");
  REQUIRE(retrieved_edge.connected_nodes == edge1_nodes);
  
  // Test connectivity queries
  auto connected_to_node1 = substrate.get_connected_nodes("node1");
  REQUIRE(connected_to_node1.size() == 2); // node2 and node3
  REQUIRE(std::find(connected_to_node1.begin(), connected_to_node1.end(), "node2") != connected_to_node1.end());
  REQUIRE(std::find(connected_to_node1.begin(), connected_to_node1.end(), "node3") != connected_to_node1.end());
  
  auto incident_edges = substrate.get_incident_edges("node1");
  REQUIRE(incident_edges.size() == 2); // edge1 and edge2
  
  // Test edge validation - should fail for non-existent nodes
  std::unordered_set<NodeId> invalid_edge_nodes = {"node1", "nonexistent"};
  HyperEdge invalid_edge("invalid", invalid_edge_nodes);
  REQUIRE_THROWS(substrate.add_edge(invalid_edge));
}

TEST_CASE("HypergraphSubstrate node removal and consistency", "[hypergraph][substrate][removal]") {
  HypergraphSubstrate substrate;
  
  // Create nodes and edges
  HyperNode node1("node1", "First Node");
  HyperNode node2("node2", "Second Node");
  HyperNode node3("node3", "Third Node");
  
  substrate.add_node(node1);
  substrate.add_node(node2);
  substrate.add_node(node3);
  
  std::unordered_set<NodeId> edge1_nodes = {"node1", "node2"};
  std::unordered_set<NodeId> edge2_nodes = {"node1", "node3"};
  std::unordered_set<NodeId> edge3_nodes = {"node2", "node3"};
  
  HyperEdge edge1("edge1", edge1_nodes);
  HyperEdge edge2("edge2", edge2_nodes);
  HyperEdge edge3("edge3", edge3_nodes);
  
  substrate.add_edge(edge1);
  substrate.add_edge(edge2);
  substrate.add_edge(edge3);
  
  REQUIRE(substrate.node_count() == 3);
  REQUIRE(substrate.edge_count() == 3);
  
  // Remove node1 - should also remove edges that connect to it
  REQUIRE(substrate.remove_node("node1"));
  REQUIRE(substrate.node_count() == 2);
  REQUIRE(substrate.edge_count() == 1); // Only edge3 should remain
  
  REQUIRE_FALSE(substrate.node_exists("node1"));
  REQUIRE_FALSE(substrate.edge_exists("edge1"));
  REQUIRE_FALSE(substrate.edge_exists("edge2"));
  REQUIRE(substrate.edge_exists("edge3"));
}

TEST_CASE("HypergraphSubstrate attribute-based queries", "[hypergraph][substrate][queries]") {
  HypergraphSubstrate substrate;
  
  HyperNode node1("node1", "Type A Node");
  node1.attributes["type"] = "content";
  node1.attributes["category"] = "text";
  
  HyperNode node2("node2", "Type B Node");
  node2.attributes["type"] = "attribute";
  node2.attributes["category"] = "metadata";
  
  HyperNode node3("node3", "Another Type A");
  node3.attributes["type"] = "content";
  node3.attributes["category"] = "image";
  
  substrate.add_node(node1);
  substrate.add_node(node2);
  substrate.add_node(node3);
  
  // Find nodes by attribute
  auto content_nodes = substrate.find_nodes_by_attribute("type", "content");
  REQUIRE(content_nodes.size() == 2);
  REQUIRE(std::find(content_nodes.begin(), content_nodes.end(), "node1") != content_nodes.end());
  REQUIRE(std::find(content_nodes.begin(), content_nodes.end(), "node3") != content_nodes.end());
  
  auto text_nodes = substrate.find_nodes_by_attribute("category", "text");
  REQUIRE(text_nodes.size() == 1);
  REQUIRE(text_nodes[0] == "node1");
  
  auto missing_nodes = substrate.find_nodes_by_attribute("nonexistent", "value");
  REQUIRE(missing_nodes.empty());
}

TEST_CASE("HypergraphSubstrate activation propagation", "[hypergraph][substrate][activation]") {
  HypergraphSubstrate substrate;
  
  // Create a small network
  HyperNode node1("node1", "Source Node");
  HyperNode node2("node2", "Target Node 1");  
  HyperNode node3("node3", "Target Node 2");
  HyperNode node4("node4", "Distant Node");
  
  substrate.add_node(node1);
  substrate.add_node(node2);
  substrate.add_node(node3);
  substrate.add_node(node4);
  
  // Create connections: node1 -> node2, node2 -> node3, node3 -> node4
  std::unordered_set<NodeId> edge1_nodes = {"node1", "node2"};
  std::unordered_set<NodeId> edge2_nodes = {"node2", "node3"};
  std::unordered_set<NodeId> edge3_nodes = {"node3", "node4"};
  
  substrate.add_edge(HyperEdge("edge1", edge1_nodes));
  substrate.add_edge(HyperEdge("edge2", edge2_nodes));
  substrate.add_edge(HyperEdge("edge3", edge3_nodes));
  
  // Test activation propagation
  substrate.propagate_activation("node1", 1.0);
  
  REQUIRE(substrate.get_node_activation("node1") == 1.0);
  REQUIRE(substrate.get_node_activation("node2") > 0.0); // Should receive some activation
  REQUIRE(substrate.get_node_activation("node3") > 0.0); // Should receive some activation (2 hops)
  
  // Test manual activation update
  substrate.update_node_activation("node4", 0.5);
  REQUIRE(substrate.get_node_activation("node4") == 0.5);
}

TEST_CASE("HypergraphSubstrate clustering analysis", "[hypergraph][substrate][clustering]") {
  HypergraphSubstrate substrate;
  
  // Create a triangular cluster (high clustering coefficient)
  HyperNode nodeA("nodeA", "Node A");
  HyperNode nodeB("nodeB", "Node B");
  HyperNode nodeC("nodeC", "Node C");
  
  substrate.add_node(nodeA);
  substrate.add_node(nodeB);
  substrate.add_node(nodeC);
  
  // Create edges to form a triangle: A-B, B-C, A-C
  substrate.add_edge(HyperEdge("edgeAB", {"nodeA", "nodeB"}));
  substrate.add_edge(HyperEdge("edgeBC", {"nodeB", "nodeC"}));
  substrate.add_edge(HyperEdge("edgeAC", {"nodeA", "nodeC"}));
  
  // All nodes should have clustering coefficient of 1.0 (perfect triangle)
  REQUIRE(substrate.calculate_clustering_coefficient("nodeA") == 1.0);
  REQUIRE(substrate.calculate_clustering_coefficient("nodeB") == 1.0);
  REQUIRE(substrate.calculate_clustering_coefficient("nodeC") == 1.0);
  
  // Test substrate density
  double density = substrate.calculate_substrate_density();
  REQUIRE(density == 1.0); // All possible edges exist in a 3-node graph
}

TEST_CASE("HypergraphSubstrate echo pattern queries", "[hypergraph][substrate][echo]") {
  HypergraphSubstrate substrate;
  
  HyperNode node1("node1", "Echo Node 1");
  node1.echo_signature.echo_patterns = {"cognitive", "pattern", "test"};
  
  HyperNode node2("node2", "Echo Node 2");
  node2.echo_signature.echo_patterns = {"cognitive", "analysis"};
  
  HyperNode node3("node3", "Different Node");
  node3.echo_signature.echo_patterns = {"unrelated", "pattern"};
  
  substrate.add_node(node1);
  substrate.add_node(node2);
  substrate.add_node(node3);
  
  // Find nodes with "cognitive" pattern
  auto cognitive_nodes = substrate.find_nodes_by_echo_pattern("cognitive");
  REQUIRE(cognitive_nodes.size() == 2);
  REQUIRE(std::find(cognitive_nodes.begin(), cognitive_nodes.end(), "node1") != cognitive_nodes.end());
  REQUIRE(std::find(cognitive_nodes.begin(), cognitive_nodes.end(), "node2") != cognitive_nodes.end());
  
  // Find nodes with "pattern" pattern
  auto pattern_nodes = substrate.find_nodes_by_echo_pattern("pattern");
  REQUIRE(pattern_nodes.size() == 2);
  REQUIRE(std::find(pattern_nodes.begin(), pattern_nodes.end(), "node1") != pattern_nodes.end());
  REQUIRE(std::find(pattern_nodes.begin(), pattern_nodes.end(), "node3") != pattern_nodes.end());
  
  // Find nodes with non-existent pattern
  auto missing_nodes = substrate.find_nodes_by_echo_pattern("nonexistent");
  REQUIRE(missing_nodes.empty());
}

TEST_CASE("HypergraphSubstrate persistence operations", "[hypergraph][substrate][persistence]") {
  HypergraphSubstrate substrate;
  
  // Add some data
  HyperNode node1("node1", "Persistent Node");
  node1.attributes["key"] = "value";
  
  substrate.add_node(node1);
  substrate.add_edge(HyperEdge("edge1", {"node1"}));
  
  REQUIRE(substrate.node_count() == 1);
  REQUIRE(substrate.edge_count() == 1);
  
  // Save substrate
  REQUIRE(substrate.save_substrate());
  
  // Clear and reload
  substrate.clear_substrate();
  REQUIRE(substrate.node_count() == 0);
  REQUIRE(substrate.edge_count() == 0);
  
  REQUIRE(substrate.load_substrate());
  REQUIRE(substrate.node_count() == 1);
  REQUIRE(substrate.edge_count() == 1);
  
  HyperNode reloaded_node;
  REQUIRE(substrate.get_node("node1", reloaded_node));
  REQUIRE(reloaded_node.label == "Persistent Node");
  REQUIRE(reloaded_node.attributes["key"] == "value");
}