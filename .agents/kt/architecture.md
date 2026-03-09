# Back to Basics — Architecture

## Overview

A BASIC language interpreter with AMD GPU compute extensions. Classic BASIC syntax
(line numbers, GOTO, GOSUB) with added GPU statements for kernel definition, device
memory management, and kernel launch via ROCm/HIP.

## System Map

```
 .bas source file
       │
       ▼
 ┌───────────┐     ┌────────────┐
 │   Lexer   │────▶│   Parser   │
 └───────────┘     └─────┬──────┘
                         │ AST
                         ▼
              ┌────────────────────┐
              │    Interpreter     │
              │  ┌──────────────┐  │
              │  │  CPU Engine  │  │
              │  └──────────────┘  │
              │  ┌──────────────┐  │
              │  │  GPU Runtime │  │──▶ hiprtc compile ──▶ GPU kernel launch
              │  └──────────────┘  │
              │  ┌──────────────┐  │
              │  │ MPI Runtime  │  │   (future: v3)
              │  │   (future)   │  │
              │  └──────────────┘  │
              └────────────────────┘
```

## Implementation Language

C++17, built with CMake, linking against ROCm/HIP for GPU support.

## Subsystems

- **Lexer** (`src/lexer.h/.cpp`): Tokenizes BASIC source into token stream
- **Parser** (`src/parser.h/.cpp`): Builds AST from tokens (recursive descent)
- **AST** (`src/ast.h`): `std::variant`-based node types for all statements/expressions
- **Interpreter** (`src/interpreter.h/.cpp`): Tree-walk executor dispatching CPU and GPU statements
- **Environment** (`src/environment.h/.cpp`): Variable/array storage, bulk data access for GPU transfers
- **GPU Codegen** (`src/gpu_codegen.h/.cpp`): Translates BASIC kernel AST → HIP C++ source
- **GPU Runtime** (`src/gpu_runtime.h/.cpp`): HIP device management, hipMalloc/hipMemcpy, hiprtc compile, kernel launch
- **MPI Runtime** (future): Inter-process communication for multi-node execution

## Key Invariants

- Line numbers are required and determine execution order
- GPU kernels are defined inline but compiled separately via hiprtc
- Host and device memory are explicitly managed (no unified memory magic)
- The interpreter is single-threaded on the CPU side; parallelism is GPU-only
- CPU-side BASIC arrays default to 1-indexed; GPU kernel arrays default to 0-indexed
- Both are configurable via `OPTION BASE N` (host) and `OPTION GPU BASE N` (GPU), where N is 0 or 1
- GPU kernel launches are synchronous (implicit hipDeviceSynchronize)
- GPU runtime is lazily initialized on first GPU statement
- Build works without HIP (CPU-only stub runtime via `#ifdef ROCBAS_HAS_HIP`)

## Phases

1. **v1**: CPU-only classic BASIC interpreter
2. **v2**: GPU extensions (GPU DIM, GPU COPY, GPU KERNEL, GPU GOSUB) — **complete**
3. **v3**: MPI extensions (multi-node, boundary exchanges)

## Dossiers

- [Implementation Dossier](implementations/impl_basic_gpu_interpreter.md) — master plan

## Test Summary

175 tests: 24 lexer, 10 AST, 32 parser, 66 interpreter, 9 GPU parser,
14 GPU runtime, 10 GPU codegen, 3 GPU integration, 7 other GPU.

## Last Verified

Commit: 9e67c63
Date: 2026-03-09
