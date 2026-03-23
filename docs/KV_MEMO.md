# KVMemo

**Version:** 1.0  
**Author:** Gagan Bansal  
**Date:** 2026-03-22  
**Copyright © 2026 KVMemo — All Rights Reserved**

---

## Table of Contents

1. [Project Overview](#1-project-overview)
2. [Design Principles](#2-design-principles)
3. [Architecture](#3-architecture)
4. [Component Reference](#4-component-reference)
5. [Data Structures](#5-data-structures)
6. [Concurrency Model](#6-concurrency-model)
7. [Wire Protocol](#7-wire-protocol)
8. [API Reference](#8-api-reference)
9. [Interaction Flows](#9-interaction-flows)
10. [Configuration](#10-configuration)
11. [Building & Running](#11-building--running)
12. [Related Documents](#12-related-documents)

---

## 1. Project Overview

KVMemo is a production-grade, in-memory key-value storage engine written in modern C++17. It is inspired by Redis and purpose-built for high concurrency, clean architectural boundaries, and long-term extensibility.

### Core Capabilities

| Feature | Description |
|---|---|
| **KV Operations** | `SET`, `GET`, `DEL` with optional per-key TTL |
| **TTL Support** | Proactive background expiration + lazy expiration on read |
| **Eviction** | Pluggable eviction policy interface; LRU is the default |
| **Shard Parallelism** | Keys distributed across N independently-locked shards |
| **Network Layer** | `select()`-based single-threaded TCP server |
| **Protocol** | Lightweight, length-prefixed text protocol |
| **Observability** | Latency tracking, operation counters, memory metrics |

### What KVMemo Is Not

KVMemo is **not** a general-purpose database. It is a purpose-built storage primitive with a well-defined boundary between infrastructure concerns and core storage semantics. It does not provide persistence, transactions, or query capabilities in its current version.

---

## 2. Design Principles

KVMemo enforces **SOLID** principles at every layer. These are not aspirational — they are architectural constraints.

### 2.1 Single Responsibility Principle (SRP)

Every component has exactly one reason to change:

| Component | Single Responsibility |
|---|---|
| `TcpServer` | Manage the listening socket and accept connections |
| `Connection` | Own a single client socket and its I/O buffers |
| `Framing` | Extract length-prefixed frames from the byte stream |
| `Parser` | Tokenize a frame string into a `Request` |
| `Serializer` | Convert a `Response` into wire bytes |
| `Dispatcher` | Route a parsed `Request` to the correct `KVEngine` call |
| `KVEngine` | Orchestrate Set/Get/Delete across storage, TTL, and eviction |
| `ShardManager` | Distribute key routing across N shards |
| `Shard` | Store one mutex-protected partition of the key space |
| `LRUCache` | Track key recency order in O(1) |
| `TTLIndex` | Maintain an ordered expiry map for keys with TTL |
| `MemoryTracker` | Account for current memory usage atomically |
| `EvictionManager` | Coordinate memory pressure response |
| `TTLManager` | Run periodic expiration sweep cycles |

### 2.2 Open/Closed Principle (OCP)

KVMemo is open for extension but closed for modification:

- **Eviction policies** are extensible without modifying `EvictionManager`. A new policy only requires implementing the `EvictionPolicy` abstract interface.
- **Commands** are extensible without modifying `Dispatcher`. A new command only requires implementing `CommandHandler` and registering it in `CommandRegistry`.
- **Transport** is replaceable without changing protocol parsing. Any class exposing `ListenFD()`, `Accept()`, and `Connection()` can replace `TcpServer`.

### 2.3 Liskov Substitution Principle (LSP)

Any implementation of `EvictionPolicy` (LRU, LFU, FIFO, Random) is a valid substitution. The `EvictionManager` makes no assumptions about the internal mechanics of the policy — it only calls `OnRead`, `OnWrite`, `OnDelete`, and `SelectVictim`.

### 2.4 Interface Segregation Principle (ISP)

Interfaces are narrow. Callers depend only on what they use:

- `EvictionPolicy` exposes only 4 methods — no storage internals.
- `CommandHandler` exposes only `Execute` — no registry management.
- `KVEngine` exposes only `Set`, `Get`, `Delete`, `ProcessExpired`, `ProcessEvictions` to callers.

### 2.5 Dependency Inversion Principle (DIP)

High-level modules depend on abstractions, not concrete types:

- `KVEngine` depends on `ShardManager`, `TTLIndex`, and `EvictionManager` interfaces, all injected at construction.
- `EvictionManager` depends on `EvictionPolicy` and `MemoryTracker`, both injected.
- No component constructs its own dependencies. All injection is via constructor.

> **Constructor injection is enforced throughout. No singletons, no service locators, no global state.**

---

## 3. Architecture

### 3.1 Layered Component Hierarchy

```
main.cpp
  └── ServerApp                        (server layer orchestrator)
        ├── TcpServer                  (accept TCP connections)
        │    └── ConnectionManager     (map fd → Connection)
        │         └── Connection       (per-client socket + buffers)
        ├── KVEngine                   (core KV API surface)
        │    ├── ShardManager          (hash-partition keys across shards)
        │    │    └── Shard[N]         (mutex-protected KV map)
        │    │         ├── Entry       (value + TTL metadata)
        │    │         ├── LRUCache    (per-shard recency tracking)
        │    │         └── TTLIndex    (per-shard expiry map)
        │    ├── TTLIndex              (engine-level global expiry index)
        │    └── EvictionManager       (memory limit + LRU victim selection)
        │         ├── MemoryTracker    (atomic memory counter)
        │         └── LRUPolicy        (wraps LRUCache for policy interface)
        └── Dispatcher                 (route Request → KVEngine method)
              └── Protocol layer
                    ├── Framing        (extract \r\n-delimited frames)
                    ├── Parser         (tokenize frame → Request)
                    └── Serializer     (Response → RESP wire bytes)
```

### 3.2 Layer Ownership Rules

Each layer owns a clearly bounded concern. No layer mutates data it does not own.

| Layer | Owns | Does Not Own |
|---|---|---|
| **Server** | Networking, I/O, request dispatch | Storage, TTL, eviction |
| **Engine** | Orchestration of KV operations | Data storage, expiration scheduling |
| **Storage** | Key-value data, per-shard mutex | Network, protocol, eviction logic |
| **Expiration** | TTL index, sweep scheduling | Storage mutation (delegates to engine) |
| **Eviction** | Memory tracking, victim selection | Storage mutation (delegates to engine) |
| **Protocol** | Frame parsing, serialization | Business logic, storage |

### 3.3 Dependency Injection Graph

```
main()
  └── ServerApp(port)
        ├── TcpServer(port)                          ← value
        ├── KVEngine(
        │     ShardManager(16, 10000),               ← unique_ptr
        │     TTLIndex(),                            ← unique_ptr
        │     EvictionManager(
        │       MemoryTracker(256 MB),               ← unique_ptr
        │       LRUPolicy(
        │         LRUCache(10000)                    ← unique_ptr
        │       )                                    ← unique_ptr
        │     )                                      ← unique_ptr
        │   )                                        ← by value (owns ptrs)
        └── Dispatcher(engine_)                      ← reference
```

### 3.4 Ownership Rules

| Owner | Owned Type | Mechanism |
|---|---|---|
| `KVEngine` | `ShardManager` | `std::unique_ptr` |
| `KVEngine` | `TTLIndex` | `std::unique_ptr` |
| `KVEngine` | `EvictionManager` | `std::unique_ptr` |
| `EvictionManager` | `MemoryTracker` | `std::unique_ptr` |
| `EvictionManager` | `EvictionPolicy` | `std::unique_ptr` |
| `LRUPolicy` | `LRUCache` | `std::unique_ptr` |
| `ShardManager` | `Shard[]` | `vector<unique_ptr<Shard>>` |
| `ConnectionManager` | `Connection[]` | `unordered_map<fd, unique_ptr<Connection>>` |
| `Dispatcher` | `KVEngine` | non-owning reference |

---

## 4. Component Reference

### 4.1 Server Module (`src/server/`)

#### ServerApp — `server_app.h`

Top-level application orchestrator. Initializes and owns all subsystems. Runs the main `select()` event loop.

```cpp
void Run()                                          // Starts the server loop (blocking)
void ProcessConnections()                           // select() loop body
```

- `select()` timeout: `50 ms`
- Single-threaded event loop; no lock required at the network layer
- Exceptions during request handling silently close the offending connection

#### Dispatcher — `dispatcher.h`

Stateless command dispatcher. Validates request structure, maps commands to `KVEngine` calls, and constructs `Response` objects.

```cpp
protocol::Response Dispatch(const protocol::Request& request)
```

| Command | Min Args | Engine Call |
|---|---|---|
| `SET` | 2 | `engine_.Set(key, value)` |
| `GET` | 1 | `engine_.Get(key)` |
| `DEL` | 1 | `engine_.Delete(key)` |

#### CommandRegistry — `command_registry.h`

Registry/Strategy pattern for extensible command handlers. Read-only after startup registration.

```cpp
void Register(const std::string& command, std::unique_ptr<CommandHandler> handler)
CommandHandler* Get(const std::string& command)
bool Exists(const std::string& command) const
```

#### ThreadPool — `thread_pool.h`

Fixed-size worker thread pool for future async command handling.

```cpp
explicit ThreadPool(std::size_t thread_count)
void Submit(std::function<void()> task)
~ThreadPool()   // graceful shutdown: drains queue, joins threads
```

---

### 4.2 Core Module (`src/core/`)

#### KVEngine — `kv_engine.h`

The central public API boundary. All Set/Get/Delete operations pass through here. Stateless orchestration — holds no data itself.

```cpp
void Set(const std::string& key, const std::string& value,
         std::optional<uint64_t> ttl_ms = std::nullopt)

std::optional<std::string> Get(const std::string& key)

void Delete(const std::string& key)

void ProcessExpired()    // collect expired keys from TTLIndex → delete from shards
void ProcessEvictions()  // collect victims from EvictionManager → delete from shards
```

**Set logic:**
1. With TTL → `SetWithTTL` on shard + `ttl_index_->Upsert(key, expire_at)`
2. Without TTL → `Set` on shard + `ttl_index_->Remove(key)`
3. Always → `eviction_manager_->OnWrite(key)`

#### ShardManager — `shard_manager.h`

Distributes keys across N independently-locked shards using `std::hash`.

```cpp
// Shard routing
std::size_t index = std::hash<std::string>{}(key) % shard_count_;
```

**Construction:**
```cpp
ShardManager(std::size_t shard_count, std::size_t shard_capacity)
// Example: ShardManager(16, 10000) → 16 shards, 10,000 keys each
```

#### Shard — `shard.h`

Single mutex-protected KV storage partition. Contains its own `LRUCache` and `TTLIndex`.

```cpp
void Set(const Key& key, std::string value)
void SetWithTTL(const Key& key, std::string value, uint64_t ttl_ms)
std::optional<std::string> Get(const Key& key)   // lazy expiry on read
void Delete(const Key& key)
void CleanupExpired(uint64_t now)
```

When `lru_.Touch(key)` signals capacity overflow, `EvictOne()` is called immediately, removing the LRU key from both `store_` and `ttl_index_`.

#### Entry — `entry.h`

Value record with TTL metadata. The key is stored as the `unordered_map` key (not inside `Entry`), keeping `Entry` minimal.

```cpp
struct Entry {
    std::string value_        // stored payload
    uint64_t    created_at_   // epoch ms at construction
    uint64_t    expire_at_    // epoch ms at expiry; 0 = no TTL
}

bool HasTTL() const noexcept               // expire_at_ != 0
bool IsExpired() const noexcept            // Clock::NowEpochMillis() >= expire_at_
uint64_t RemainingTTL() const noexcept     // ms remaining; 0 if no TTL or expired
```

#### LRUCache — `lru_cache.h`

O(1) LRU recency tracking (keys only, no values).

**Data structure:** doubly-linked list (`std::list<Key>`) + hash map (`std::unordered_map<Key, list::iterator>`)

```cpp
bool Touch(const Key& key)           // insert or move-to-front; returns true on overflow
void Remove(const Key& key)          // O(1)
Key PopEvictionCandidate()           // remove and return LRU key (back of list)
```

| Operation | Complexity |
|---|---|
| `Touch` | O(1) |
| `Remove` | O(1) |
| `PopEvictionCandidate` | O(1) |

#### TTLIndex — `ttl_index.h`

Time-ordered expiration tracking. Two complementary structures for efficient range scans and reverse lookups.

```cpp
// Internal state
std::map<Timestamp, std::vector<Key>> expiry_map_  // expire_at → [keys]
std::unordered_map<Key, Timestamp>    key_index_   // key → expire_at

void Upsert(const Key& key, Timestamp expire_at)      // O(log N)
void Remove(const Key& key)                           // O(log N)
std::vector<Key> CollectExpired(Timestamp now)        // O(K log N), K = expired keys
```

---

### 4.3 Eviction Module (`src/eviction/`)

#### EvictionPolicy — abstract interface

```cpp
class EvictionPolicy {
public:
    virtual void OnRead(const std::string& key) = 0;
    virtual void OnWrite(const std::string& key) = 0;
    virtual void OnDelete(const std::string& key) = 0;
    virtual std::optional<std::string> SelectVictim() = 0;
};
```

New eviction policies implement this interface. `EvictionManager` is never modified.

#### LRUPolicy — `eviction_manager.h`

Default eviction policy. Wraps `LRUCache` to satisfy the `EvictionPolicy` interface.

#### EvictionManager — `eviction_manager.h`

Coordinates memory tracking and victim selection. Fully self-synchronized.

```cpp
void OnWrite(const std::string& key)   // reserves 100 bytes; updates LRU position
void OnDelete(const std::string& key)  // releases 100 bytes
std::vector<std::string> CollectEvictionCandidates()
```

**Eviction does not delete keys.** It returns a victim list. `KVEngine::ProcessEvictions()` performs the actual deletion. This preserves the ownership boundary.

#### MemoryTracker — `memory_tracker.h`

Lock-free memory usage accounting using `std::atomic<std::size_t>`.

```cpp
bool Reserve(std::size_t bytes) noexcept     // fetch_add; returns !IsOverLimit()
void Release(std::size_t bytes) noexcept     // fetch_sub
bool IsOverLimit() const noexcept
```

#### TTLManager — `ttl_manager.h`

Background thread that drives proactive expiration. Calls `run_expiration_cycle()` on a configurable tick interval.

```cpp
size_t run_expiration_cycle()   // fetch expired keys → delete via ShardManager
void set_enabled(bool enabled) noexcept
```

**Ownership rule:** `TTLManager` schedules. `KVEngine` executes. No layer crosses these boundaries.

---

### 4.4 Protocol Module (`src/protocol/`)

#### Request — `request.h`

Immutable parsed client command.

```cpp
Request(std::string command, std::vector<std::string> args)
const std::string& Command() const noexcept
const std::string& Arg(std::size_t index) const   // throws std::out_of_range
```

#### Response — `response.h`

Server response value object.

```cpp
static Response Ok(std::string message = {})
static Response Error(std::string message)
bool IsOk() const noexcept
bool IsError() const noexcept
```

#### Parser — `parser.h`

Stateless tokenizer. Splits on whitespace; first token is the command, remainder are arguments. Throws `std::invalid_argument` on empty input.

#### Serializer — `serializer.h`

Converts `Response` to RESP-like wire bytes.

| Response Type | Wire Format | Example |
|---|---|---|
| OK (no data) | `+OK\r\n` | `+OK\r\n` |
| Bulk string | `$<len>\r\n<data>\r\n` | `$5\r\nAlice\r\n` |
| Error | `-ERR<message>\r\n` | `-ERRKey not found\r\n` |

#### Framing — `framing.h`

Extracts complete length-prefixed frames from the TCP byte stream. Returns `false` if more data is needed (partial read).

#### Buffer — `buffer.h`

Dynamic byte buffer for connection I/O. Uses a contiguous `vector<char>` with a read cursor. `Consume()` resets storage when all bytes are consumed, reclaiming memory without reallocation.

---

### 4.5 Network Module (`src/net/`)

#### TcpServer — `tcp_server.h`

Creates and manages the listening TCP socket. Options: `INADDR_ANY`, backlog = 128.

#### Connection — `connection.h`

Owns a single client socket and its I/O buffers (input + output `Buffer`). Read buffer size: 4096 bytes per call.

#### ConnectionManager — `connection_manager.h`

Stores the active connection set keyed by file descriptor.

```cpp
void Add(std::unique_ptr<Connection> connection)
void Remove(int fd)
Connection* Get(int fd)
```

---

### 4.6 Metrics Module (`src/metrics/`)

#### MetricsRegistry — `metrics_registry.h`

Thread-safe named counter registry backed by `std::mutex`.

```cpp
void increment(const std::string& name, uint64_t value = 1)  // auto-registers if absent
MetricSnapshot snapshot() const
```

#### LatencyTracker — `latency_tracker.h`

High-resolution latency measurement. Hot path is lock-free (`std::atomic`); min/max are mutex-protected.

```cpp
TimePoint start() const noexcept
void stop(TimePoint start_time) noexcept   // records ns duration
LatencyStats snapshot() const
```

#### MetricsSnapshot — `metrics_snapshot.h`

Immutable point-in-time view of all subsystem metrics. Thread-safe for concurrent reads after construction.

```cpp
struct EngineMetricsSnapshot {
    uint64_t total_keys;
    uint64_t total_requests;
    uint64_t total_evictions;
    uint64_t total_expirations;
};
```

---

### 4.7 Common Utilities (`src/common/`)

#### Logger — `logger.h`

Thread-safe structured logging. Static-only class; `std::mutex` protects `std::cerr`; log level is `std::atomic`.

```
[EPOCH_MS] [LEVEL] [tid=THREAD_ID] file:line | message
```

**Macros:** `KV_LOG_TRACE`, `KV_LOG_DEBUG`, `KV_LOG_INFO`, `KV_LOG_WARN`, `KV_LOG_ERROR`, `KV_LOG_FATAL`

> `KV_LOG_FATAL` calls `std::abort()` after logging.

#### Status — `status.h`

Lightweight error-carrying return type.

| Code | Use Case |
|---|---|
| `kOk` | Success |
| `kInvalidArgument` | Bad parameters |
| `kNotFound` | Missing key |
| `kProtocolError` | Malformed protocol |
| `kResourceExhausted` | Memory/connection limit |
| `kInternalError` | Unexpected failures |

#### Clock — `time.h`

Centralized time provider. Static-only, non-instantiable.

```cpp
static EpochMillis NowEpochMillis() noexcept     // wall-clock for TTL expiry
static SteadyTimePoints NowSteady() noexcept     // monotonic for latency measurement
```

> **Rationale:** Wall-clock (`system_clock`) for TTL timestamps; monotonic (`steady_clock`) for latency to guard against NTP adjustments.

---

## 5. Data Structures

### 5.1 KV Storage Hierarchy

```
KVEngine
  └── ShardManager (N shards)
        └── Shard[i]
              ├── std::unordered_map<Key, Entry>   ← primary store
              ├── LRUCache                          ← recency order
              │     ├── std::list<Key>              ← MRU front, LRU back
              │     └── std::unordered_map<Key, list::iterator>
              └── TTLIndex                          ← per-shard expiry
                    ├── std::map<Timestamp, vector<Key>>  ← sorted by expire time
                    └── std::unordered_map<Key, Timestamp>
```

### 5.2 Time Complexity Summary

| Operation | Complexity | Notes |
|---|---|---|
| `Set(key, value)` | O(1) avg | Hash map insert + LRU O(1) touch |
| `Get(key)` | O(1) avg | Hash map lookup + LRU O(1) touch |
| `Delete(key)` | O(1) avg | Hash map erase + LRU O(1) remove |
| `LRU Touch` | O(1) | Doubly-linked list splice + map update |
| `LRU Evict` | O(1) | Pop back of list |
| `TTL Upsert` | O(log N) | `std::map` insertion |
| `TTL CollectExpired` | O(K log N) | K = expired keys |
| `Shard Lookup` | O(1) | `std::hash` + modulo |
| `MemoryTracker Reserve` | O(1) | Atomic `fetch_add` |

### 5.3 Entry Memory Layout

```
Entry {
    std::string value_       // heap-allocated payload
    uint64_t    created_at_  // 8 bytes
    uint64_t    expire_at_   // 8 bytes (0 = no TTL)
}
```

The key is stored as the `unordered_map` key, not inside `Entry`, keeping the record minimal.

---

## 6. Concurrency Model

### 6.1 Thread Model

| Thread | Responsibility | Count |
|---|---|---|
| **Main / Event Loop** | `select()`, accept, read, dispatch, write | 1 |
| **TTLManager** | Periodic expiration sweep | 1 (optional) |
| **ThreadPool Workers** | Future async command handling | Configurable |

### 6.2 Synchronization Strategy

KVMemo avoids a global storage lock entirely. Parallelism is proportional to shard count.

```
Main Event Loop (single thread)
  │ calls KVEngine (thread-safe by delegation)
  ▼
KVEngine (stateless orchestration)
  │ hash → Shard[i]
  ▼
Shard[i]
  std::mutex mutex_  (exclusive per shard)
  Protects: store_, lru_, ttl_index_
  ▲
  TTLManager thread also acquires the same per-shard mutex via ShardManager
```

Two operations on keys that hash to different shards run fully in parallel.

### 6.3 Mutex Inventory

| Class | Mutex Type | Guards | Scope |
|---|---|---|---|
| `Shard` | `std::mutex` | `store_`, `lru_`, `ttl_index_` | Per-shard |
| `EvictionManager` | `std::mutex` | `policy_`, `memory_tracker_` | Single instance |
| `MetricsRegistry` | `std::mutex` | `counters_` map | Single instance |
| `LatencyTracker` | `std::mutex` | `min_latency_ns_`, `max_latency_ns_` | Per-tracker |
| `ThreadPool` | `std::mutex` | `tasks_` queue | Single instance |
| `Logger` | `std::mutex` | `std::cerr` output | Static |

### 6.4 Lock-Free Components

| Class | Mechanism |
|---|---|
| `MemoryTracker` | `std::atomic<size_t>` with `memory_order_relaxed` |
| `LatencyTracker` (hot path) | `std::atomic<uint64_t>` for `total_ops`, `total_ns` |
| `Logger::level_` | `std::atomic<LogLevel>` |
| `TTLManager::enabled_` | `std::atomic<bool>` |

### 6.5 Deadlock Prevention

- No two mutexes are ever held simultaneously — no lock ordering issues.
- Each `Shard` acquires only its own mutex per operation.
- `EvictionManager` acquires its own mutex and never calls back into shards while locked.

---

## 7. Wire Protocol

KVMemo uses a lightweight, length-prefixed text protocol over TCP.

### 7.1 Message Framing

Each message is prefixed by its payload length:

```
{length}\r\n{payload}
```

Example:
```
21\r\nSET user:1 Alice
```

### 7.2 Supported Commands

| Command | Syntax | Response on Success | Response on Error |
|---|---|---|---|
| `SET` | `SET <key> <value>` | `+OK\r\n` | `-ERR SET requires key and value\r\n` |
| `SET with TTL` | `SET <key> <value> PX <ttl_ms>` | `+OK\r\n` | `-ERRInvalid TTL\r\n` |
| `GET` | `GET <key>` | `$<len>\r\n<value>\r\n` | `-ERRKey not found\r\n` |
| `DEL` | `DEL <key>` | `+OK\r\n` | `-ERR DEL requires key\r\n` |
| *(unknown)* | — | — | `-ERRUnknown command\r\n` |

### 7.3 Example Session

```
Client: 21\r\nSET user:1 Alice
Server: 3\r\n+OK

Client: 13\r\nGET user:1
Server: 8\r\n$5\r\nAlice

Client: 13\r\nDEL user:1
Server: 3\r\n+OK

Client: 13\r\nGET user:1
Server: 15\r\n-ERRKey not found
```

### 7.4 Expiration Semantics

If a key has expired, it behaves as non-existent:
- `GET` returns `-ERRKey not found`
- Protocol does not expose expiration timestamps

---

## 8. API Reference

### 8.1 KVEngine C++ API

```cpp
// Store key with optional TTL (milliseconds)
engine.Set("key", "value");
engine.Set("session:123", "data", 30000);  // expires in 30 seconds

// Retrieve value; nullopt if missing or expired
std::optional<std::string> v = engine.Get("key");
if (v.has_value()) { /* use *v */ }

// Delete key (no error if missing)
engine.Delete("key");

// Called by TTLManager background thread
engine.ProcessExpired();

// Called when memory limit is exceeded
engine.ProcessEvictions();
```

### 8.2 Extending Eviction Policy

```cpp
class MyCustomPolicy : public EvictionPolicy {
public:
    void OnRead(const std::string& key) override { /* update policy state */ }
    void OnWrite(const std::string& key) override { /* update policy state */ }
    void OnDelete(const std::string& key) override { /* remove from policy state */ }
    std::optional<std::string> SelectVictim() override { /* return next eviction candidate */ }
};

// Inject at construction — no EvictionManager changes required
auto policy = std::make_unique<MyCustomPolicy>();
auto tracker = std::make_unique<MemoryTracker>(256 * 1024 * 1024);
auto eviction = std::make_unique<EvictionManager>(std::move(tracker), std::move(policy));
```

### 8.3 Registering a New Command

```cpp
class PingHandler : public CommandHandler {
public:
    protocol::Response Execute(
        const protocol::Request& request,
        core::KVEngine& engine) override
    {
        return protocol::Response::Ok("PONG");
    }
};

registry.Register("PING", std::make_unique<PingHandler>());
```

---

## 9. Interaction Flows

### 9.1 SET Request

```
Client: "SET key value\r\n"
  │
  ▼ Connection::ReadFromSocket()    → raw bytes into input_buffer_
  ▼ Framing::NextFrame()            → frame = "SET key value"
  ▼ Parser::Parse(frame)            → Request{command="SET", args=["key","value"]}
  ▼ Dispatcher::Dispatch(request)   → HandleSet()
  ▼ KVEngine::Set("key", "value")
       ├── ShardManager::Set(key, value)
       │     └── Shard[hash(key) % N]::Set(key, value)
       │           ├── store_["key"] = Entry("value")
       │           ├── lru_.Touch("key")  → overflow? → EvictOne()
       │           └── ttl_index_.Remove("key")
       ├── ttl_index_.Remove("key")   (engine-level)
       └── eviction_manager_.OnWrite("key")
             ├── memory_tracker_.Reserve(100)
             └── policy_.OnWrite("key") → lru_.Touch("key")
  ▼ Response::Ok() → "+OK\r\n"
  ▼ Connection::WriteToSocket()
```

### 9.2 GET Request

```
Client: "GET key\r\n"
  │
  ▼ Framing::NextFrame()   → "GET key"
  ▼ Parser::Parse()         → Request{command="GET", args=["key"]}
  ▼ KVEngine::Get("key")
       └── ShardManager::Get(key)
             └── Shard::Get(key)
                   ├── store_.find("key")  → not found? → nullopt
                   ├── entry.IsExpired()?  → yes → RemoveInternal() → nullopt
                   └── lru_.Touch("key")   → return entry.Value()
  ├── value found  → eviction_manager_.OnRead() → Response::Ok(value)
  └── not found    → Response::Error("Key not found")
```

### 9.3 TTL Expiration Flow

```
TTLManager thread (periodic)
  │
  ▼ TTLIndex::CollectExpired(Clock::NowEpochMillis())
       → scan expiry_map_ prefix where expire_at <= now
       → return expired_keys[]
  ▼ for each expired_key:
       ShardManager::Delete(key)
         └── Shard::Delete(key)
               → store_.erase(key)
               → lru_.Remove(key)
               → ttl_index_.Remove(key)

[Parallel path — lazy expiry on read]
Shard::Get(key)
  → entry.IsExpired()?
      yes → RemoveInternal(key) → return nullopt
```

### 9.4 Memory Eviction Flow

```
KVEngine::Set(key, value)
  │
  ▼ EvictionManager::OnWrite(key)
       ├── memory_tracker_.Reserve(100)
       ├── policy_.OnWrite(key)  → lru_.Touch(key)
       └── EnforceMemoryLimit()  → no-op if within limit

[Explicit eviction — KVEngine::ProcessEvictions()]
EvictionManager::CollectEvictionCandidates()
  loop while IsOverLimit():
    victim = policy_.SelectVictim() → lru_.PopEvictionCandidate()
    victims.push_back(victim)
    memory_tracker_.Release(100)

for each victim:
  ShardManager::Delete(victim)
  TTLIndex::Remove(victim)
```

---

## 10. Configuration

Configuration is defined in `src/common/config.h` via the `Config` struct. The table below lists the `Config` struct defaults. `main.cpp` currently bypasses `Config` and constructs `ServerApp` with hardcoded values (port `6379`, 16 shards, 10,000 capacity per shard, 256 MB memory limit); the `Config` struct is available for future integration.

| Field | Type | `Config` Default | Active hardcoded value (main.cpp) | Description |
|---|---|---|---|---|
| `shard_count` | `size_t` | `64` | `16` | Must be a power of two |
| `max_memory_bytes` | `uint64_t` | `256 MB` | `256 MB` | Global memory limit |
| `max_value_bytes` | `uint64_t` | `8 MB` | — | Max single value size |
| `listen_port` | `uint16_t` | `8080` | `6379` | TCP listen port |
| `max_connections` | `size_t` | `4096` | — | Soft connection limit |
| `worker_threads` | `size_t` | `0` (auto) | — | 0 = auto-detect |
| `enable_ttl` | `bool` | `true` | — | Enables TTL expiration |
| `ttl_sweep_interval_ms` | `uint32_t` | `250` | — | Background sweep period |
| `enable_metrics` | `bool` | `true` | — | Enables metrics collection |
| `eviction_policy` | `EvictionPolicy` | `kLRU` | — | `kNone` or `kLRU` |

**Validation rules:**
- `shard_count` > 0 and must be a power of two
- `max_memory_bytes` > 0
- `max_value_bytes` <= `max_memory_bytes`
- Valid `listen_port`
- `worker_threads` <= 1024
- `ttl_sweep_interval_ms` > 0 if TTL enabled

---

## 11. Building & Running

### 11.1 Prerequisites

- C++17-compatible compiler (GCC 8+, Clang 7+, MSVC 2017+)
- CMake 3.16+

### 11.2 Build

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

This produces two executables:
- `kvmemo` — the server
- `kv_cli` — the command-line client

### 11.3 Run the Server

```bash
./kvmemo            # default port 6379
./kvmemo 7379       # custom port
```

### 11.4 Connect with the CLI Client

```bash
./kv_cli
> SET user:1 Alice
+OK
> GET user:1
Alice
> DEL user:1
+OK
```

---

## 12. Related Documents

| Document | Description |
|---|---|
| [HLD.md](./HLD.md) | High-level system design: architecture diagrams, scalability strategy, roadmap |
| [LLD.md](./LLD.md) | Low-level design: full module breakdown, data structures, threading, API contracts |
| [PROTOCOL.md](./PROTOCOL.md) | Wire protocol specification: framing, commands, error handling |
| [BENCHMARKS.md](./BENCHMARKS.md) | Performance benchmarks and throughput characteristics |
| [DEPLOYMENT.md](./DEPLOYMENT.md) | Deployment guide and operational runbook |

---

*Copyright © 2026 KVMemo — All Rights Reserved*  
*This document may not be copied, modified, or distributed without explicit permission from the author.*