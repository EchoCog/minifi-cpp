<!--
  Licensed to the Apache Software Foundation (ASF) under one or more
  contributor license agreements.  See the NOTICE file distributed with
  this work for additional information regarding copyright ownership.
  The ASF licenses this file to You under the Apache License, Version 2.0
  (the "License"); you may not use this file except in compliance with
  the License.  You may obtain a copy of the License at
      http://www.apache.org/licenses/LICENSE-2.0
  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
-->

# Apache NiFi MiNiFi C++ - Technical Architecture Documentation

## Table of Contents

- [Overview](#overview)
- [System Architecture](#system-architecture)
  - [High-Level Architecture](#high-level-architecture)
  - [Core Components](#core-components)
  - [Component Relationships](#component-relationships)
- [Data Flow Architecture](#data-flow-architecture)
  - [FlowFile Lifecycle](#flowfile-lifecycle)
  - [Processing Pipeline](#processing-pipeline)
  - [Data Routing](#data-routing)
- [Extension System](#extension-system)
  - [Extension Framework](#extension-framework)
  - [Available Extensions](#available-extensions)
  - [Extension Loading](#extension-loading)
- [Configuration Architecture](#configuration-architecture)
  - [Configuration Flow](#configuration-flow)
  - [Parameter Management](#parameter-management)
  - [Runtime Configuration](#runtime-configuration)
- [Repository Architecture](#repository-architecture)
  - [Repository Types](#repository-types)
  - [Data Persistence](#data-persistence)
  - [Repository Interactions](#repository-interactions)
- [Scheduling and Execution](#scheduling-and-execution)
  - [Scheduling Agents](#scheduling-agents)
  - [Thread Management](#thread-management)
  - [Execution Flow](#execution-flow)
- [State Management](#state-management)
  - [Component States](#component-states)
  - [State Transitions](#state-transitions)
  - [Monitoring and Metrics](#monitoring-and-metrics)
- [Communication Architecture](#communication-architecture)
  - [Site-to-Site Protocol](#site-to-site-protocol)
  - [C2 Protocol](#c2-protocol)
  - [HTTP Communication](#http-communication)

## Overview

Apache NiFi MiNiFi C++ is a lightweight, edge-focused data collection and processing framework designed for resource-constrained environments. It provides a subset of Apache NiFi's capabilities with a smaller footprint and enhanced performance characteristics.

### Key Design Principles

- **Lightweight**: Minimal resource consumption for edge deployment
- **Modular**: Plugin-based extension system for customization
- **Configurable**: YAML-based configuration with parameter providers
- **Reliable**: Built-in fault tolerance and data persistence
- **Secure**: Comprehensive security features and protocols

## System Architecture

### High-Level Architecture

```mermaid
graph TB
    subgraph "MiNiFi C++ Agent"
        subgraph "Application Layer"
            ML[MiNiFi Main]
            FC[FlowController]
            PG[ProcessGroup]
        end
        
        subgraph "Core Framework"
            CF[Core Framework]
            EF[Extension Framework]
            RA[Repository Abstraction]
        end
        
        subgraph "Processing Engine"
            SA[Scheduling Agents]
            PE[Processing Engine]
            SM[State Management]
        end
        
        subgraph "Extensions"
            SP[Standard Processors]
            EP[Extension Processors]
            CS[Controller Services]
        end
        
        subgraph "Repositories"
            FFR[FlowFile Repository]
            CR[Content Repository]
            PR[Provenance Repository]
        end
        
        subgraph "Communication"
            S2S[Site-to-Site]
            C2[C2 Protocol]
            HTTP[HTTP Interface]
        end
    end
    
    subgraph "External Systems"
        NiFi[Apache NiFi]
        ES[External Systems]
        FS[File Systems]
    end
    
    ML --> FC
    FC --> PG
    FC --> SA
    FC --> SM
    PG --> PE
    PE --> SP
    PE --> EP
    PE --> CS
    SA --> PE
    CF --> EF
    CF --> RA
    RA --> FFR
    RA --> CR
    RA --> PR
    S2S --> NiFi
    C2 --> NiFi
    HTTP --> ES
    SP --> FS
    EP --> ES
```

### Core Components

The MiNiFi C++ architecture is built around several core components that work together to provide data processing capabilities:

#### FlowController
The central orchestrator that manages the entire data flow execution environment.

```mermaid
classDiagram
    class FlowController {
        -running_: bool
        -root_wrapper_: RootProcessGroupWrapper
        -flow_configuration_: FlowConfiguration
        -scheduling_agents_: vector~SchedulingAgent~
        -repositories_: map~Repository~
        -c2_agent_: C2Agent
        +load(reload: bool)
        +start(): int16_t
        +stop(): int16_t
        +isRunning(): bool
        +getRoot(): ProcessGroup
    }
    
    class ProcessGroup {
        -type_: ProcessGroupType
        -processors_: vector~Processor~
        -connections_: vector~Connection~
        -child_process_groups_: vector~ProcessGroup~
        -controller_services_: map~ControllerService~
        +addProcessor(processor: Processor)
        +addConnection(connection: Connection)
        +startProcessing()
        +stopProcessing()
    }
    
    class Processor {
        -scheduled_state_: ScheduledState
        -scheduling_strategy_: SchedulingStrategy
        -properties_: map~Property~
        -relationships_: vector~Relationship~
        +onSchedule(context: ProcessContext)
        +onTrigger(context: ProcessContext, session: ProcessSession)
        +onUnSchedule()
    }
    
    class Connection {
        -source_: Connectable
        -destination_: Connectable
        -relationships_: set~Relationship~
        -flow_file_queue_: FlowFileQueue
        +poll(): FlowFile
        +offer(flowfile: FlowFile)
    }
    
    FlowController ||--|| ProcessGroup : "manages root"
    ProcessGroup ||--o{ Processor : "contains"
    ProcessGroup ||--o{ Connection : "contains"
    ProcessGroup ||--o{ ProcessGroup : "nested"
    Processor ||--o{ Connection : "connected by"
```

#### Repository System
Manages data persistence across different types of storage requirements.

```mermaid
classDiagram
    class Repository {
        <<interface>>
        +initialize(): bool
        +start(): bool
        +stop(): bool
        +Put(key: string, value: string): bool
        +Get(key: string, value: string): bool
        +Delete(key: string): bool
    }
    
    class FlowFileRepository {
        +storeElement(element: SerializableComponent): bool
        +getElements(store: vector~SerializableComponent~): bool
        +loadComponent(content_repo: ContentRepository)
    }
    
    class ContentRepository {
        +create(): ResourceClaim
        +read(claim: ResourceClaim): InputStream
        +write(claim: ResourceClaim): OutputStream
        +exists(claim: ResourceClaim): bool
        +remove(claim: ResourceClaim): bool
    }
    
    class ProvenanceRepository {
        +registerEvent(event: ProvenanceEvent)
        +getEvents(events: vector~ProvenanceEvent~): size_t
    }
    
    Repository <|-- FlowFileRepository
    Repository <|-- ContentRepository
    Repository <|-- ProvenanceRepository
```

### Component Relationships

```mermaid
graph LR
    subgraph "Core Components"
        FC[FlowController]
        PG[ProcessGroup]
        P[Processor]
        C[Connection]
    end
    
    subgraph "Scheduling"
        TDS[Timer Driven Agent]
        EDS[Event Driven Agent]
        CDS[Cron Driven Agent]
    end
    
    subgraph "Configuration"
        YAML[YAML Config]
        PP[Parameter Providers]
        PC[Parameter Context]
    end
    
    subgraph "State & Metrics"
        SM[State Manager]
        MS[Metrics Store]
        BL[Bulletin Store]
    end
    
    FC --> PG
    PG --> P
    P --> C
    FC --> TDS
    FC --> EDS
    FC --> CDS
    YAML --> FC
    PP --> PC
    PC --> PG
    FC --> SM
    FC --> MS
    P --> BL
```

## Data Flow Architecture

### FlowFile Lifecycle

FlowFiles are the fundamental data units that flow through the MiNiFi processing pipeline.

```mermaid
stateDiagram-v2
    [*] --> Created : Processor creates FlowFile
    Created --> Queued : Added to Connection queue
    Queued --> Processing : Processor polls FlowFile
    Processing --> Modified : Processor modifies content/attributes
    Processing --> Routed : Processor routes to relationship
    Modified --> Routed : Processing complete
    Routed --> Queued : Sent to next Connection
    Routed --> Repository : Persisted for recovery
    Routed --> [*] : Processor removes/completes
    
    Processing --> Failed : Processing error
    Failed --> Penalty : Apply penalty period
    Penalty --> Queued : Return to queue after penalty
    
    Repository --> Restored : System restart recovery
    Restored --> Queued : Resume processing
```

### Processing Pipeline

```mermaid
sequenceDiagram
    participant SA as Scheduling Agent
    participant P as Processor
    participant C as Connection
    participant R as Repository
    participant CS as Content Store
    
    SA->>P: Schedule execution
    P->>C: Poll for FlowFiles
    C->>P: Return FlowFile(s)
    
    alt FlowFile available
        P->>P: onTrigger()
        P->>CS: Read content (if needed)
        P->>P: Process data
        P->>CS: Write content (if modified)
        P->>R: Update FlowFile record
        P->>C: Transfer to relationship
        C->>C: Queue for next processor
    else No FlowFile available
        P->>P: yield()
        P->>SA: Signal completion
    end
    
    SA->>SA: Schedule next execution
```

### Data Routing

```mermaid
flowchart TD
    subgraph "Input Sources"
        FS[File System]
        DB[Database]
        API[REST API]
        MQTT[MQTT Broker]
    end
    
    subgraph "Processing Pipeline"
        GP[GetFile/GetData Processor]
        TP[Transform Processor]
        RP[Route Processor]
        EP[Export Processor]
    end
    
    subgraph "Output Destinations"
        S3[AWS S3]
        HDFS[HDFS]
        HTTP[HTTP Endpoint]
        LOG[Log File]
    end
    
    FS --> GP
    DB --> GP
    API --> GP
    MQTT --> GP
    
    GP --> TP
    TP --> RP
    
    RP -->|success| EP
    RP -->|failure| LOG
    RP -->|retry| TP
    
    EP --> S3
    EP --> HDFS
    EP --> HTTP
    EP --> LOG
```

## Extension System

### Extension Framework

MiNiFi C++ uses a modular extension system that allows for dynamic loading of processors and services.

```mermaid
graph TB
    subgraph "Extension Manager"
        EM[Extension Manager]
        EL[Extension Loader]
        ER[Extension Registry]
    end
    
    subgraph "Core Extensions"
        SP[Standard Processors]
        EL_EXT[Expression Language]
        ARCH[Archive Processors]
    end
    
    subgraph "External Extensions"
        AWS[AWS Extension]
        AZURE[Azure Extension]
        KAFKA[Kafka Extension]
        PYTHON[Python Extension]
        LUA[Lua Extension]
    end
    
    subgraph "Extension Interface"
        EI[Extension Interface]
        PA[Processor API]
        CSA[Controller Service API]
    end
    
    EM --> EL
    EM --> ER
    EL --> SP
    EL --> EL_EXT
    EL --> ARCH
    EL --> AWS
    EL --> AZURE
    EL --> KAFKA
    EL --> PYTHON
    EL --> LUA
    
    SP --> EI
    AWS --> EI
    AZURE --> EI
    KAFKA --> EI
    PYTHON --> EI
    LUA --> EI
    
    EI --> PA
    EI --> CSA
```

### Available Extensions

```mermaid
mindmap
  root((MiNiFi Extensions))
    Core
      Standard Processors
      Expression Language
      Archive Support
    Cloud
      AWS
        S3
        SQS
        SNS
      Azure
        Blob Storage
        Event Hubs
      Google Cloud
        Cloud Storage
        Pub/Sub
    Messaging
      MQTT
      Kafka
      AMQP
    Scripting
      Python
      Lua
      Bustache
    Storage
      SQL
      Elasticsearch
      Couchbase
    Monitoring
      Prometheus
      Grafana Loki
      Splunk
    System
      Kubernetes
      Systemd
      ProcFS
    Security
      SFTP
      OPC-UA
```

### Extension Loading

```mermaid
sequenceDiagram
    participant FC as FlowController
    participant EM as Extension Manager
    participant EL as Extension Loader
    participant SO as Shared Object
    participant EF as Extension Factory
    
    FC->>EM: Initialize extensions
    EM->>EL: Load extension modules
    
    loop For each extension
        EL->>SO: dlopen() shared library
        SO->>EF: Register factory functions
        EF->>EM: Register processors/services
        EM->>EM: Add to registry
    end
    
    EM->>FC: Extensions loaded
    
    Note over FC, EF: Runtime processor creation
    FC->>EM: Create processor by name
    EM->>EF: Call factory function
    EF->>FC: Return processor instance
```

## Configuration Architecture

### Configuration Flow

```mermaid
flowchart TD
    subgraph "Configuration Sources"
        YAML[config.yml]
        ENV[Environment Variables]
        FILE[File Parameters]
        CMD[Command Line]
    end
    
    subgraph "Parameter System"
        PP[Parameter Providers]
        PC[Parameter Contexts]
        PV[Parameter Values]
    end
    
    subgraph "Configuration Processing"
        CF[Configuration Factory]
        FC_CONFIG[Flow Configuration]
        VALIDATE[Validation]
    end
    
    subgraph "Runtime Components"
        FC[FlowController]
        PG[Process Groups]
        PROC[Processors]
        CONN[Connections]
    end
    
    YAML --> CF
    ENV --> PP
    FILE --> PP
    CMD --> CF
    
    PP --> PC
    PC --> PV
    PV --> FC_CONFIG
    
    CF --> FC_CONFIG
    FC_CONFIG --> VALIDATE
    VALIDATE --> FC
    
    FC --> PG
    PG --> PROC
    PG --> CONN
```

### Parameter Management

```mermaid
classDiagram
    class ParameterProvider {
        <<interface>>
        +getParameters(): map~string, string~
        +supportsRefresh(): bool
        +refresh()
    }
    
    class EnvironmentVariableParameterProvider {
        -inclusion_strategy_: string
        -include_pattern_: regex
        +getParameters(): map~string, string~
    }
    
    class FileParameterProvider {
        -file_path_: string
        -parameter_group_name_: string
        +getParameters(): map~string, string~
    }
    
    class ParameterContext {
        -name_: string
        -parameters_: map~string, Parameter~
        +getParameter(name: string): Parameter
        +setParameter(name: string, value: string)
    }
    
    class Parameter {
        -name_: string
        -value_: string
        -sensitive_: bool
        -description_: string
    }
    
    ParameterProvider <|-- EnvironmentVariableParameterProvider
    ParameterProvider <|-- FileParameterProvider
    ParameterContext ||--o{ Parameter : "contains"
    ParameterProvider ..> ParameterContext : "populates"
```

### Runtime Configuration

```mermaid
sequenceDiagram
    participant YAML as YAML Config
    participant CF as Configuration Factory
    participant PP as Parameter Provider
    participant PC as Parameter Context
    participant FC as FlowController
    participant PG as ProcessGroup
    
    YAML->>CF: Load configuration
    CF->>PP: Initialize parameter providers
    PP->>PC: Populate parameter contexts
    CF->>FC: Create flow configuration
    FC->>PG: Build process group hierarchy
    
    loop Runtime parameter updates
        PP->>PC: Refresh parameters
        PC->>PG: Update processor properties
        PG->>PG: Reconfigure processors
    end
```

## Repository Architecture

### Repository Types

```mermaid
graph TB
    subgraph "Repository Layer"
        RA[Repository Abstraction]
        
        subgraph "FlowFile Repository"
            FFR[FlowFile Repository]
            FFR_IMPL[Repository Implementation]
            FFR_ROCKS[RocksDB Repository]
            FFR_VOL[Volatile Repository]
        end
        
        subgraph "Content Repository"
            CR[Content Repository]
            CR_FS[FileSystem Repository]
            CR_VOL[Volatile Repository]
            CR_NULL[Null Repository]
        end
        
        subgraph "Provenance Repository"
            PR[Provenance Repository]
            PR_IMPL[Repository Implementation]
            PR_ROCKS[RocksDB Repository]
            PR_VOL[Volatile Repository]
        end
    end
    
    subgraph "Storage Backends"
        FS[File System]
        MEM[Memory]
        ROCKS[RocksDB]
    end
    
    RA --> FFR
    RA --> CR
    RA --> PR
    
    FFR --> FFR_IMPL
    FFR_IMPL --> FFR_ROCKS
    FFR_IMPL --> FFR_VOL
    
    CR --> CR_FS
    CR --> CR_VOL
    CR --> CR_NULL
    
    PR --> PR_IMPL
    PR_IMPL --> PR_ROCKS
    PR_IMPL --> PR_VOL
    
    FFR_ROCKS --> ROCKS
    FFR_VOL --> MEM
    CR_FS --> FS
    CR_VOL --> MEM
    PR_ROCKS --> ROCKS
    PR_VOL --> MEM
```

### Data Persistence

```mermaid
sequenceDiagram
    participant P as Processor
    participant FFR as FlowFile Repository
    participant CR as Content Repository
    participant PR as Provenance Repository
    participant STORAGE as Storage Backend
    
    P->>FFR: Store FlowFile metadata
    FFR->>STORAGE: Serialize FlowFile record
    
    P->>CR: Store content data
    CR->>STORAGE: Write content to claim
    
    P->>PR: Record provenance event
    PR->>STORAGE: Store event data
    
    Note over P, STORAGE: Recovery scenario
    STORAGE->>FFR: Load FlowFile records
    FFR->>P: Restore FlowFiles
    STORAGE->>CR: Verify content claims
    CR->>P: Provide content access
```

### Repository Interactions

```mermaid
flowchart LR
    subgraph "Processor Execution"
        P[Processor]
        PS[Process Session]
    end
    
    subgraph "Repository Operations"
        FFR[FlowFile Repo]
        CR[Content Repo]
        PR[Provenance Repo]
    end
    
    subgraph "Transaction Management"
        TM[Transaction Manager]
        COMMIT[Commit]
        ROLLBACK[Rollback]
    end
    
    P --> PS
    PS --> FFR
    PS --> CR
    PS --> PR
    
    PS --> TM
    TM --> COMMIT
    TM --> ROLLBACK
    
    COMMIT --> FFR
    COMMIT --> CR
    COMMIT --> PR
    
    ROLLBACK --> FFR
    ROLLBACK --> CR
    ROLLBACK --> PR
```

## Scheduling and Execution

### Scheduling Agents

```mermaid
classDiagram
    class SchedulingAgent {
        <<abstract>>
        -running_: bool
        -scheduled_processors_: set~Processor~
        +schedule(processor: Processor)
        +unschedule(processor: Processor)
        +run(): void
    }
    
    class TimerDrivenSchedulingAgent {
        -thread_pool_: ThreadPool
        -scheduling_period_: chrono::duration
        +run(): void
        -executeProcessor(processor: Processor): void
    }
    
    class EventDrivenSchedulingAgent {
        -event_queue_: Queue~ProcessorEvent~
        -worker_threads_: vector~thread~
        +run(): void
        +notifyEvent(processor: Processor): void
    }
    
    class CronDrivenSchedulingAgent {
        -cron_scheduler_: CronScheduler
        -scheduled_tasks_: map~Processor, CronTask~
        +run(): void
        -scheduleCronTask(processor: Processor): void
    }
    
    SchedulingAgent <|-- TimerDrivenSchedulingAgent
    SchedulingAgent <|-- EventDrivenSchedulingAgent
    SchedulingAgent <|-- CronDrivenSchedulingAgent
```

### Thread Management

```mermaid
graph TB
    subgraph "Thread Pool Management"
        TP[Thread Pool]
        TM[Thread Manager]
        WT[Worker Threads]
    end
    
    subgraph "Scheduling Agents"
        TDS[Timer Driven Agent]
        EDS[Event Driven Agent]
        CDS[Cron Driven Agent]
    end
    
    subgraph "Processor Execution"
        PE[Processor Execution]
        PS[Process Session]
        PC[Process Context]
    end
    
    subgraph "Concurrency Control"
        MC[Max Concurrent Tasks]
        SC[Single Threading]
        YP[Yield Period]
    end
    
    TM --> TP
    TP --> WT
    
    TDS --> TP
    EDS --> TP
    CDS --> TP
    
    WT --> PE
    PE --> PS
    PE --> PC
    
    PE --> MC
    PE --> SC
    PE --> YP
```

### Execution Flow

```mermaid
stateDiagram-v2
    [*] --> Idle : Processor created
    Idle --> Scheduled : FlowController schedules
    Scheduled --> Running : Scheduling agent selects
    Running --> Processing : onTrigger() called
    Processing --> Yielding : yield() called
    Processing --> Completed : Processing finished
    Yielding --> Scheduled : Yield period expires
    Completed --> Scheduled : More work available
    Completed --> Idle : No work available
    Scheduled --> Stopped : FlowController stops
    Running --> Stopped : FlowController stops
    Processing --> Stopped : FlowController stops
    Yielding --> Stopped : FlowController stops
    Stopped --> [*] : Processor shutdown
    
    state Processing {
        [*] --> onSchedule : First time
        onSchedule --> onTrigger
        onTrigger --> onTrigger : Multiple executions
        onTrigger --> onUnSchedule : Last time
        onUnSchedule --> [*]
    }
```

## State Management

### Component States

```mermaid
stateDiagram-v2
    [*] --> Stopped : Initial state
    Stopped --> Starting : start() called
    Starting --> Running : Startup complete
    Running --> Stopping : stop() called
    Stopping --> Stopped : Shutdown complete
    
    state Running {
        [*] --> Enabled
        Enabled --> Disabled : Processor disabled
        Disabled --> Enabled : Processor enabled
        
        state Enabled {
            [*] --> Scheduled
            Scheduled --> Unscheduled : Processor unscheduled
            Unscheduled --> Scheduled : Processor scheduled
        }
    }
    
    Stopped --> [*] : Component destroyed
```

### State Transitions

```mermaid
sequenceDiagram
    participant FC as FlowController
    participant SA as Scheduling Agent
    participant P as Processor
    participant PS as Process Session
    participant R as Repository
    
    FC->>SA: start()
    SA->>P: schedule()
    P->>P: onSchedule()
    
    loop Processing Loop
        SA->>P: trigger execution
        P->>PS: createSession()
        P->>P: onTrigger(context, session)
        P->>PS: commit() or rollback()
        PS->>R: persist changes
    end
    
    FC->>SA: stop()
    SA->>P: unschedule()
    P->>P: onUnSchedule()
    P->>P: shutdown()
```

### Monitoring and Metrics

```mermaid
graph TB
    subgraph "Metrics Collection"
        MP[Metrics Publisher]
        MS[Metrics Store]
        MC[Metrics Collector]
    end
    
    subgraph "Component Metrics"
        PM[Processor Metrics]
        CM[Connection Metrics]
        RM[Repository Metrics]
        SM[System Metrics]
    end
    
    subgraph "State Information"
        CS[Component State]
        BS[Bulletin Store]
        SS[State Store]
    end
    
    subgraph "External Reporting"
        PROM[Prometheus]
        HTTP[HTTP Endpoint]
        LOG[Log Files]
    end
    
    MC --> PM
    MC --> CM
    MC --> RM
    MC --> SM
    
    MC --> MS
    MS --> MP
    
    CS --> SS
    BS --> SS
    SS --> MP
    
    MP --> PROM
    MP --> HTTP
    MP --> LOG
```

## Communication Architecture

### Site-to-Site Protocol

```mermaid
sequenceDiagram
    participant MC as MiNiFi Client
    participant MS as MiNiFi Server/NiFi
    participant HC as HTTP Client
    participant HS as HTTP Server
    
    Note over MC, HS: HTTP Site-to-Site
    MC->>HS: GET /nifi-api/site-to-site
    HS->>MC: Site-to-Site info
    MC->>HS: POST /nifi-api/data-transfer/input-ports/{port-id}/transactions
    HS->>MC: Transaction created
    MC->>HS: PUT /nifi-api/data-transfer/input-ports/{port-id}/transactions/{transaction-id}/flow-files
    HS->>MC: Flow files received
    MC->>HS: DELETE /nifi-api/data-transfer/input-ports/{port-id}/transactions/{transaction-id}
    HS->>MC: Transaction confirmed
    
    Note over MC, MS: RAW Site-to-Site
    MC->>MS: Handshake
    MS->>MC: Handshake response
    MC->>MS: Request transaction
    MS->>MC: Transaction ID
    MC->>MS: Send flow files
    MS->>MC: Acknowledgment
    MC->>MS: Confirm transaction
    MS->>MC: Transaction complete
```

### C2 Protocol

```mermaid
graph TB
    subgraph "MiNiFi Agent"
        C2A[C2 Agent]
        FC[FlowController]
        CM[Configuration Manager]
    end
    
    subgraph "C2 Server/NiFi"
        C2S[C2 Server]
        C2H[C2 Handler]
        FR[Flow Registry]
    end
    
    subgraph "C2 Operations"
        HB[Heartbeat]
        CONFIG[Configuration Update]
        MANIFEST[Manifest Request]
        METRICS[Metrics Report]
    end
    
    C2A --> HB
    C2A --> CONFIG
    C2A --> MANIFEST
    C2A --> METRICS
    
    HB --> C2S
    CONFIG --> C2S
    MANIFEST --> C2S
    METRICS --> C2S
    
    C2S --> C2H
    C2H --> FR
    
    C2A --> FC
    C2A --> CM
```

### HTTP Communication

```mermaid
flowchart TD
    subgraph "HTTP Interface"
        HS[HTTP Server]
        HR[HTTP Router]
        HH[HTTP Handlers]
    end
    
    subgraph "REST Endpoints"
        SS[System Status]
        PROC[Processor Status]
        CONN[Connection Status]
        CONFIG[Configuration]
        METRICS[Metrics]
    end
    
    subgraph "Security"
        AUTH[Authentication]
        TLS[TLS/SSL]
        CERT[Certificate Management]
    end
    
    subgraph "Client Communication"
        WEB[Web Interface]
        API[REST Client]
        CURL[Command Line]
    end
    
    WEB --> HS
    API --> HS
    CURL --> HS
    
    HS --> TLS
    HS --> AUTH
    HS --> HR
    
    HR --> HH
    HH --> SS
    HH --> PROC
    HH --> CONN
    HH --> CONFIG
    HH --> METRICS
    
    TLS --> CERT
```

---

This comprehensive architecture documentation provides a detailed technical overview of Apache NiFi MiNiFi C++, including system components, data flow patterns, extension mechanisms, and communication protocols. The Mermaid diagrams illustrate the relationships and interactions between different components, making it easier to understand the overall system design and implementation.