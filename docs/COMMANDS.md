# KVMemo Command Reference

> A complete reference for all commands supported by the KVMemo in-memory key-value store.

---

## Table of Contents

1. [Getting Started](#getting-started)
2. [Commands](#commands)
   - [SET](#set)
   - [GET](#get)
   - [DEL](#del)
   - [SETEX](#setex)
   - [KEYS](#keys)
   - [PING](#ping)
   - [FLUSH](#flush)
   - [EXISTS](#exists)
3. [Using the CLI](#using-the-cli)
4. [Error Responses](#error-responses)

---

## Getting Started

### Build the Project

```bash
mkdir build && cd build
cmake ..
make
```

This produces two executables:
- `kvmemo` — the server
- `kv_cli` — the interactive command-line client

### Start the Server

```bash
./kvmemo [port]
```

Default port is `6379` if not specified.

```bash
# Start on default port 6379
./kvmemo

# Start on a custom port
./kvmemo 8082
```

### Connect with the CLI

```bash
./kv_cli [host] [port]
```

Default host is `127.0.0.1` and default port is `8082`.

```bash
# Connect to default host:port
./kv_cli

# Connect to a specific host and port
./kv_cli 127.0.0.1 6379
```

---

## Commands

### SET

Store a key-value pair in the store.

**Syntax:**
```
SET <key> <value>
```

**Arguments:**
| Argument | Required | Description |
|---|---|---|
| `key` | Yes | The key to store |
| `value` | Yes | The value to associate with the key |

**Response:** `OK` on success, error message on failure.

**Examples:**
```
kvmemo> SET name Alice
OK

kvmemo> SET age 30
OK
```

---

### GET

Retrieve the value associated with a key.

**Syntax:**
```
GET <key>
```

**Arguments:**
| Argument | Required | Description |
|---|---|---|
| `key` | Yes | The key to look up |

**Response:** The stored value on success, or an error if the key does not exist.

**Examples:**
```
kvmemo> GET name
Alice

kvmemo> GET missing_key
ERR Key not found
```

---

### DEL

Delete a key-value pair from the store.

**Syntax:**
```
DEL <key>
```

**Arguments:**
| Argument | Required | Description |
|---|---|---|
| `key` | Yes | The key to delete |

**Response:** `OK` on success.

**Examples:**
```
kvmemo> DEL name
OK

kvmemo> GET name
ERR Key not found
```

---

### SETEX

Store a key-value pair with a TTL (time-to-live). The key is automatically deleted after the TTL expires.

**Syntax:**
```
SETEX <key> <ttl_ms> <value>
```

**Arguments:**
| Argument | Required | Description |
|---|---|---|
| `key` | Yes | The key to store |
| `ttl_ms` | Yes | Time-to-live in **milliseconds** (must be a positive integer) |
| `value` | Yes | The value to associate with the key |

**Response:** `OK` on success, error message on failure.

**Examples:**
```
# Store "session_token" with a 5-second TTL (5000 ms)
kvmemo> SETEX session_token 5000 abc123
OK

# The key is accessible before expiry
kvmemo> GET session_token
abc123

# After 5000 ms the key is automatically removed
kvmemo> GET session_token
ERR Key not found
```

**Error cases:**
```
kvmemo> SETEX mykey -100 value
ERR SETEX ttl_ms must be a positive integer

kvmemo> SETEX mykey 0 value
ERR SETEX ttl_ms must be a positive integer
```

---

### KEYS

List all keys and their current values stored in the server.

**Syntax:**
```
KEYS
```

**Arguments:** None.

**Response:** A newline-separated list of `key:value` pairs, or an empty response if the store is empty.

**Examples:**
```
kvmemo> SET foo bar
OK

kvmemo> SET hello world
OK

kvmemo> KEYS
foo:bar
hello:world
```

---

### PING

Health check command. Verifies the server is running and responsive.

**Syntax:**
```
PING
```

**Arguments:** None.

**Response:** `PONG`

**Example:**
```
kvmemo> PING
PONG
```

---

### FLUSH
Delete all keys from the store. Resets the TTL index and memory tracker.
**Syntax:**
```
FLUSH
```
**Arguments:** None.
**Response:** `OK`
**Example:**
```
kvmemo> SET foo bar
OK
kvmemo> SET hello world
OK
kvmemo> FLUSH
OK
kvmemo> KEYS
```
---
### EXISTS
Check if a key exists in the store. Expired keys return `0`.
**Syntax:**
```
EXISTS <key>
```
**Arguments:**
| Argument | Required | Description |
|---|---|---|
| `key` | Yes | The key to check |
**Response:** `1` if the key exists, `0` if it does not exist or has expired.
**Examples:**
```
kvmemo> SET name Alice
OK
kvmemo> EXISTS name
1
kvmemo> EXISTS missing_key
0
kvmemo> SETEX temp_key 100 value
OK
# After TTL expires:
kvmemo> EXISTS temp_key
0
```
---

## Using the CLI

After connecting with `kv_cli`, type commands at the `kvmemo>` prompt.

```
Connected to KVMemo server at 127.0.0.1:8082
Type 'exit' to quit.
kvmemo> SET greeting "Hello, World!"
OK
kvmemo> GET greeting
Hello, World!
kvmemo> PING
PONG
kvmemo> KEYS
greeting:Hello, World!
kvmemo> DEL greeting
OK
kvmemo> exit
```

To quit the CLI, type `exit` or `quit`, or press `Ctrl+D`.

---

## Error Responses

All errors are returned with an `ERR` prefix:

| Error | Cause |
|---|---|
| `ERR Empty Command` | Sent an empty input |
| `ERR Unknown command` | Command name not recognized |
| `ERR SET requires key and value` | `SET` called with fewer than 2 arguments |
| `ERR Get requires key` | `GET` called without a key |
| `ERR DEL requires key` | `DEL` called without a key |
| `ERR SETEX requires key, ttl_ms and value` | `SETEX` called with fewer than 3 arguments |
| `ERR SETEX ttl_ms must be a positive integer` | TTL is zero or negative |
| `ERR SETEX ttl_ms must be a valid integer` | TTL is not a valid number |
| `ERR KEYS takes no arguments` | `KEYS` called with extra arguments |
| `ERR PING takes no arguments` | `PING` called with extra arguments |
| `ERR Key not found` | `GET` on a key that does not exist or has expired |
| `ERR FLUSH takes no arguments` | `FLUSH` called with extra arguments |
| `ERR EXISTS requires key` | `EXISTS` called without a key |
