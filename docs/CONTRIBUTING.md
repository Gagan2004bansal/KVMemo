# Contributing to KVMemo

Thank you for your interest in contributing to KVMemo. This guide covers everything you need to get the project building, understand the codebase, follow the coding conventions, and submit a clean pull request.

---

## Table of Contents

1. [Prerequisites](#1-prerequisites)
2. [Building the Project](#2-building-the-project)
3. [Project Structure](#3-project-structure)
4. [Code Style](#4-code-style)
5. [Testing](#5-testing)
6. [Running the Server and CLI](#6-running-the-server-and-cli)
7. [Pull Request Process](#7-pull-request-process)
8. [Commit Message Guidelines](#8-commit-message-guidelines)

---

## 1. Prerequisites

| Tool | Minimum Version |
|---|---|
| C++ compiler (GCC / Clang / MSVC) | C++17 support required |
| CMake | 3.16+ |
| Make / Ninja | Any modern version |
| Python 3 | For load test scripts (optional) |

---

## 2. Building the Project

```bash
# Clone the repository
git clone https://github.com/GaganBansalCSE/KVMemo.git
cd KVMemo

# Create and enter a build directory
mkdir build && cd build

# Configure and build
cmake ..
make
```

This produces two executables inside `build/`:

| Executable | Purpose |
|---|---|
| `kvmemo` | The KVMemo server |
| `kv_cli` | The interactive command-line client |

### Build options

```bash
# Release build (optimised)
cmake -DCMAKE_BUILD_TYPE=Release ..
make

# Debug build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```

---

## 3. Project Structure

```
KVMemo/
├── src/
│   ├── client/       # kv_cli — TCP command-line client
│   ├── common/       # Shared types, constants, utilities
│   ├── core/         # KVEngine, ShardManager, Shard
│   ├── eviction/     # EvictionPolicy interface, LRU implementation, EvictionManager
│   ├── metrics/      # MetricsRegistry, latency tracking, counters
│   ├── net/          # TCP server, connection handling (select-based)
│   ├── protocol/     # Request parsing, response serialisation
│   ├── server/       # Server bootstrap, request dispatch
│   └── main.cpp      # Entry point
├── tests/
│   ├── test_kv.cpp          # Core KV operation tests
│   ├── test_concurrency.cpp # Concurrent access tests
│   └── test_protocol.cpp    # Protocol parsing tests
├── scripts/
│   ├── run_server.sh    # Helper to start the server
│   ├── benchmark.sh     # Throughput benchmark
│   └── load_test.py     # Python load generator
├── docs/                # Design documents (HLD, LLD, Protocol, etc.)
├── CMakeLists.txt
└── Command.md           # Full command reference
```

Key design boundaries to be aware of:

- **`net/` and `server/`** own all networking concerns — they have no knowledge of storage internals.
- **`core/`** owns all data and orchestration — it has no knowledge of the wire protocol.
- **`eviction/`** selects victim keys but never deletes them — deletion is always executed by `KVEngine`.
- **`protocol/`** is a pure parsing/serialisation layer with no side effects.

Refer to [`docs/HLD.md`](docs/HLD.md) and [`docs/LLD.md`](docs/LLD.md) for the full architecture.

---

## 4. Code Style

KVMemo follows modern C++17 conventions. Please keep all contributions consistent with the existing style.

### General

- Standard: **C++17**. Do not use C++20 features.
- Headers use `#pragma once`.
- No raw owning pointers — prefer `std::unique_ptr` / `std::shared_ptr`.
- No `using namespace std;` in header files.
- Keep files focused: one class or one logical concern per file.

### Naming

| Entity | Convention | Example |
|---|---|---|
| Classes / structs | `PascalCase` | `ShardManager`, `EvictionPolicy` |
| Methods / functions | `PascalCase` | `Set()`, `ProcessExpired()` |
| Member variables | `snake_case_` (trailing underscore) | `shard_count_`, `ttl_index_` |
| Local variables | `snake_case` | `expire_time`, `victim_keys` |
| Constants / enums | `UPPER_SNAKE_CASE` | `DEFAULT_PORT`, `MAX_SHARDS` |
| Files | `snake_case` | `kv_engine.cpp`, `shard_manager.h` |

### Formatting

- Indent with **4 spaces** — no tabs.
- Opening braces on the same line as the statement.
- One blank line between method definitions.
- Keep lines under **120 characters** where practical.

### SOLID principles

The codebase is structured around SOLID. When adding new behaviour:

- **SRP** — each class has exactly one reason to change.
- **OCP** — extend via new implementations (e.g. a new `EvictionPolicy`), not by modifying existing classes.
- **DIP** — depend on abstractions; inject concrete types through constructors.

---

## 5. Testing

Tests live in `tests/`. They are standalone `.cpp` files compiled separately.

```bash
# From the build directory — compile a test manually
g++ -std=c++17 -I../src ../tests/test_kv.cpp -o test_kv && ./test_kv
g++ -std=c++17 -I../src ../tests/test_concurrency.cpp -o test_concurrency -lpthread && ./test_concurrency
g++ -std=c++17 -I../src ../tests/test_protocol.cpp -o test_protocol && ./test_protocol
```

**Guidelines:**

- Every new feature or bug fix must be accompanied by a test.
- Tests for concurrent code must be deterministic — avoid sleep-based synchronisation where possible.
- Do not modify or delete existing tests. Add new cases or new test files.

### Load and benchmark testing

```bash
# Quick benchmark (requires a running server)
cd scripts && ./benchmark.sh

# Python load generator
python3 scripts/load_test.py
```

---

## 6. Running the Server and CLI

```bash
# Start the server on the default port (6379)
./build/kvmemo

# Start on a custom port
./build/kvmemo 8082

# Connect with the CLI (default: 127.0.0.1:8082)
./build/kv_cli

# Connect to a specific host and port
./build/kv_cli 127.0.0.1 6379
```

See [`Command.md`](Command.md) for the full command reference (SET, GET, DEL, SETEX, KEYS, PING).

---

## 7. Pull Request Process

1. **Branch** — create a branch from `main` with a descriptive name:
   ```
   feature/lfu-eviction-policy
   fix/ttl-overflow-edge-case
   docs/update-hld-sharding
   ```

2. **Keep changes focused** — one logical change per PR. Avoid mixing refactors with feature additions.

3. **Self-review** — before opening the PR, review your own diff and check:
   - No debug output or temporary code left in.
   - New code follows the style guide above.
   - Tests pass.
   - Architecture boundaries have not been crossed (e.g. storage logic leaking into `net/`).

4. **PR description** — include:
   - What the change does and why.
   - Which module(s) are affected.
   - How to test or reproduce the scenario.

5. **Review** — address all review comments before merging.

---

## 8. Commit Message Guidelines

Use short, imperative-mood subject lines under 72 characters:

```
Add LFU eviction policy implementation
Fix TTL overflow for keys with large expiry values
Refactor ShardManager to accept custom hash function
docs: update HLD concurrency model section
```

Prefix with a scope when useful: `fix:`, `feat:`, `refactor:`, `docs:`, `test:`, `chore:`.

---

*KVMemo is proprietary software. All contributions remain the property of the project. By submitting a pull request you confirm that your contribution is your own original work.*