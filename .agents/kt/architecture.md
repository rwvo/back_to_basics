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

- **Lexer**: Tokenizes BASIC source into token stream
- **Parser**: Builds AST from tokens (classic line-numbered BASIC grammar)
- **AST**: Node types for all statements, expressions, and GPU extensions
- **Interpreter**: Tree-walk executor with environment/variable storage
- **GPU Runtime**: Device memory management, kernel codegen (BASIC→HIP C++), hiprtc compilation, kernel launch
- **MPI Runtime** (future): Inter-process communication for multi-node execution

## Key Invariants

- Line numbers are required and determine execution order
- GPU kernels are defined inline but compiled separately via hiprtc
- Host and device memory are explicitly managed (no unified memory magic)
- The interpreter is single-threaded on the CPU side; parallelism is GPU-only

## Phases

1. **v1**: CPU-only classic BASIC interpreter
2. **v2**: GPU extensions (GPU DIM, GPU COPY, GPU KERNEL, GPU LAUNCH)
3. **v3**: MPI extensions (multi-node, boundary exchanges)

## Dossiers

- [Implementation Dossier](implementations/impl_basic_gpu_interpreter.md) — master plan

## Last Verified

Commit: N/A
Date: 2026-03-09
