/**
 * @file CognitiveEchoProcessor.cpp
 * @brief Foundation Layer: Basic Cognitive Echo Processing Implementation
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

#include "processors/CognitiveEchoProcessor.h"
#include "core/ProcessContext.h"
#include "core/ProcessSession.h"
#include "core/FlowFile.h"
#include "core/Resource.h"
#include "utils/Id.h"
#include "io/InputStream.h"
#include <string>
#include <chrono>

namespace org::apache::nifi::minifi::cognitive::processors {

CognitiveEchoProcessor::CognitiveEchoProcessor(core::ProcessorMetadata metadata)
    : CognitiveProcessor(std::move(metadata)) {
  setup_properties();
}

void CognitiveEchoProcessor::setup_properties() {
  // Configure processor properties for cognitive processing
  echo_pattern_ = "cognitive_echo";
  resonance_threshold_ = 0.5;
  memory_store_enabled_ = true;
}

void CognitiveEchoProcessor::process_with_cognition(
    core::ProcessContext& context,
    core::ProcessSession& session,
    CognitiveKernel& kernel) {
  
  auto flow_file = session.get();
  if (!flow_file) {
    return;
  }

  try {
    // Read FlowFile content for cognitive processing
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

    // Process content through cognitive kernel
    kernel.process_cognitive_signal(content);
    
    // Calculate cognitive resonance
    double resonance = kernel.calculate_resonance(content);
    
    // Get current cognitive state as string
    std::string cognitive_state_str;
    switch (kernel.get_cognitive_state()) {
      case CognitiveState::DORMANT: cognitive_state_str = "dormant"; break;
      case CognitiveState::AWAKENING: cognitive_state_str = "awakening"; break;
      case CognitiveState::PROCESSING: cognitive_state_str = "processing"; break;
      case CognitiveState::LEARNING: cognitive_state_str = "learning"; break;
      case CognitiveState::REASONING: cognitive_state_str = "reasoning"; break;
      case CognitiveState::CREATING: cognitive_state_str = "creating"; break;
      case CognitiveState::REFLECTING: cognitive_state_str = "reflecting"; break;
    }

    // Add cognitive attributes to FlowFile
    add_cognitive_attributes(*flow_file, resonance, cognitive_state_str);
    
    // Store in cognitive memory if enabled
    if (memory_store_enabled_) {
      kernel.get_memory().store("last_processed_uuid", flow_file->getUUIDStr().c_str());
      kernel.get_memory().store("last_content_hash", std::to_string(std::hash<std::string>{}(content)));
    }

    // Route based on resonance level
    if (resonance >= resonance_threshold_) {
      logger_->log_debug("FlowFile {} has high cognitive resonance: {:.3f}", 
                        flow_file->getUUIDStr(), resonance);
      session.transfer(flow_file, HighResonance);
    } else {
      logger_->log_debug("FlowFile {} has low cognitive resonance: {:.3f}", 
                        flow_file->getUUIDStr(), resonance);
      session.transfer(flow_file, LowResonance);
    }

    // Also send to success relationship
    auto success_flow = session.clone(*flow_file);
    if (success_flow) {
      session.transfer(success_flow, Success);
    }

  } catch (const std::exception& ex) {
    logger_->log_error("Failed to process FlowFile {} through cognitive kernel: {}", 
                      flow_file->getUUIDStr(), ex.what());
    session.transfer(flow_file, Success); // Transfer to success to avoid infinite loop
  }
}

void CognitiveEchoProcessor::add_cognitive_attributes(
    core::FlowFile& flow_file,
    double resonance,
    const std::string& cognitive_state) const {
  
  const auto& echo_identity = get_cognitive_kernel().get_echo_identity();
  
  // Add Deep Tree Echo identity attributes
  flow_file.addAttribute("cognitive.echo.identity", echo_identity.identity_signature);
  flow_file.addAttribute("cognitive.echo.frequency", std::to_string(echo_identity.resonance_frequency));
  flow_file.addAttribute("cognitive.echo.weight", std::to_string(echo_identity.cognitive_weight));
  
  // Add processing attributes
  flow_file.addAttribute("cognitive.resonance", std::to_string(resonance));
  flow_file.addAttribute("cognitive.state", cognitive_state);
  flow_file.addAttribute("cognitive.processor", getProcessorType());
  flow_file.addAttribute("cognitive.timestamp", std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()).count()));
  
  // Add echo patterns
  for (size_t i = 0; i < echo_identity.echo_patterns.size(); ++i) {
    flow_file.addAttribute("cognitive.echo.pattern." + std::to_string(i), 
                          echo_identity.echo_patterns[i]);
  }
}

REGISTER_RESOURCE(CognitiveEchoProcessor, Processor);

}  // namespace org::apache::nifi::minifi::cognitive::processors