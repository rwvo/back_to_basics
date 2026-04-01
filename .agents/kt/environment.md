# Environment

## Responsibility
Runtime storage for BASIC variables and arrays. Manages scalar get/set, multi-dimensional
array allocation and access, configurable array base index, and bulk data access for
GPU memory transfers.

## Core Concepts
- **Value**: `std::variant<double, std::string>` — the runtime type.
- **Array**: Flat storage (`std::vector<Value>`) with dimension metadata. Multi-dimensional indexing via `flat_index()`.
- **Array base**: Configurable via `OPTION BASE` (0, 0.5, or 1; default 1 for classic BASIC).
- **Bulk access**: `get_array_data()` / `set_array_data()` extract/inject `std::vector<double>` for GPU transfers.

## Key Invariants
- Undefined variable access throws a runtime error.
- Array indices are `double` (not truncated before reaching Environment) to support `OPTION BASE 0.5`.
- `flat_index()` computes `int adj = static_cast<int>(indices[i] - base)` — exact for 0.5 in IEEE 754.
- Arrays are zero-filled on allocation.

## Interfaces
- `void set(name, value)` / `Value get(name)` — scalar access — `src/environment.h:18-19`
- `void dim_array(name, dimensions)` — allocate array — `src/environment.h:23`
- `void set_array(name, indices, value)` / `Value get_array(name, indices)` — element access — `src/environment.h:24-25`
- `get_array_data()` / `set_array_data()` — bulk GPU transfer — `src/environment.h:30-31`
- `set_array_base()` / `array_base()` — OPTION BASE — `src/environment.h:34-35`

## Dependencies
- None (self-contained).

## Also Load
- [Interpreter](interpreter.md) — primary consumer.

## Last Verified
Commit: a315030
Date: 2026-04-01
