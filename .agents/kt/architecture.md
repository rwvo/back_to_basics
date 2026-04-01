# Back to Basics — Architecture

## Overview

A BASIC language interpreter with AMD GPU compute extensions and MPI support.
Classic BASIC syntax (line numbers, GOTO, GOSUB) with added GPU statements for
kernel definition, device memory management, and kernel launch via ROCm/HIP.
MPI extensions (v3, in progress) enable multi-rank execution via `mpirun`.

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
              │  │ MPI Runtime  │  │──▶ MPI_Send/Recv ──▶ inter-rank comm (v3)
              │  └──────────────┘  │
              └────────────────────┘
```

## Implementation Language

C++17, built with CMake. Optional dependencies: ROCm/HIP (GPU), MPI (multi-rank).

## Subsystems

- **Lexer** (`src/lexer.h/.cpp`): Tokenizes BASIC source into token stream
- **Parser** (`src/parser.h/.cpp`): Builds AST from tokens (recursive descent)
- **AST** (`src/ast.h`): `std::variant`-based node types for all statements/expressions
- **Interpreter** (`src/interpreter.h/.cpp`): Tree-walk executor dispatching CPU and GPU statements
- **Environment** (`src/environment.h/.cpp`): Variable/array storage, bulk data access for GPU transfers
- **GPU Codegen** (`src/gpu_codegen.h/.cpp`): Translates BASIC kernel AST → HIP C++ source
- **GPU Runtime** (`src/gpu_runtime.h/.cpp`): HIP device management, hipMalloc/hipMemcpy, hiprtc compile, kernel launch
- **MPI Runtime** (`src/mpi_runtime.h/.cpp`, v3): MPI init/finalize, blocking send/recv, barrier

## Key Invariants

- Line numbers are required and determine execution order
- GPU kernels are defined inline but compiled separately via hiprtc
- Host and device memory are explicitly managed (no unified memory magic)
- The interpreter is single-threaded on the CPU side; parallelism is GPU-only
- CPU-side BASIC arrays default to 1-indexed; GPU kernel arrays default to 0-indexed
- Both are configurable via `OPTION BASE N` (host) and `OPTION GPU BASE N` (GPU), where N is 0, 0.5, or 1
- GPU arrays are 1D only; use stride arguments for 2D indexing (matches HIP practice)
- GPU kernel launches are synchronous (implicit hipDeviceSynchronize)
- GPU runtime is lazily initialized on first GPU statement
- Build works without HIP (CPU-only stub runtime via `#ifdef ROCBAS_HAS_HIP`)
- Build works without MPI (stub runtime via `#ifdef ROCBAS_HAS_MPI`)
- MPI programs work without `mpirun` (single-process: rank 0, size 1)
- REPL supports LOAD/SAVE for file I/O (C64-style, `src/main.cpp`)

## Phases

1. **v1**: CPU-only classic BASIC interpreter — **complete**
2. **v2**: GPU extensions (GPU DIM, GPU COPY, GPU KERNEL, GPU GOSUB) — **complete**
3. **v3**: MPI extensions (MPI SEND/RECV, BARRIER, multi-rank) — **complete** (steps 21-25 done, including heat diffusion demo)

## Dossiers

- [Implementation Dossier](implementations/impl_basic_gpu_interpreter.md) — master plan and progress log
- [Lexer](lexer.md) — tokenizer (source → token stream)
- [Parser](parser.md) — recursive descent parser (tokens → AST)
- [AST](ast.md) — std::variant-based node type definitions
- [Interpreter](interpreter.md) — tree-walk executor, CPU + GPU dispatch
- [Environment](environment.md) — variable/array storage with configurable base index
- [GPU Codegen](gpu_codegen.md) — BASIC kernel AST → HIP C++ source
- [GPU Runtime](gpu_runtime.md) — HIP device management, memory, kernel launch
- [Glossary](glossary.md) — domain terminology

## Test Summary

220+ tests: 26 lexer, 18 AST, 48 parser (incl. 16 MPI), 82 interpreter (incl. 12 MPI),
9 GPU parser, 14 GPU runtime, 12 GPU codegen, 3 GPU integration, 9 MPI runtime,
6 multi-rank MPI integration (.bas files via mpirun).

## Last Verified

Commit: 2fb3c7e
Date: 2026-04-01
