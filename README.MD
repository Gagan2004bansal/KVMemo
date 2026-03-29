# KVMemo

A high-performance, cross-platform in-memory key-value store written in C++17.

<img src="./docs/assets/kvmemo-logo.png" alt="KVMemo">

**Live:** [kvmemo.dev](https://www.kvmemo.dev)

---

## Overview

KVMemo is a lightweight, TCP-based key-value store designed for low-latency data access. It features a multi-threaded server, a sharded storage engine with LRU eviction, TTL-based key expiration, and an interactive command-line client.

The project is under active development and built incrementally with daily commits.

---

## Features

- Multi-threaded TCP server with a configurable thread pool
- Sharded storage engine for reduced lock contention
- LRU eviction policy with configurable memory limits
- TTL-based key expiration with millisecond precision
- Built-in metrics collection (latency, memory usage)
- Lightweight binary protocol with request framing
- Interactive CLI client (`kv_cli`)
- Cross-platform build via CMake (C++17)

---

## License

This project is proprietary and not licensed for reuse.

