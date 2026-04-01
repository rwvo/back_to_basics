# Interpreter

## Responsibility
Tree-walk interpreter that executes a parsed BASIC `Program`. Dispatches CPU statements
directly, GPU statements to `GpuRuntime` + `gpu_codegen`, and manages the execution
environment (variables, arrays, control flow stacks).

## Core Concepts
- **Tree-walk execution**: Iterates over `Program::lines` by index (`pc`), dispatching each statement via `std::visit`.
- **Value**: `std::variant<double, std::string>` — the runtime representation of all BASIC values.
- **ForContext**: Tracks FOR loop state (var name, end value, step, body PC) on `for_stack_`.
- **GOSUB stack**: `gosub_stack_` stores return PCs for subroutine calls.
- **Lazy GPU init**: `GpuRuntime` is created on first GPU statement via `gpu()` accessor.

## Key Invariants
- Execution follows line-number order; GOTO/GOSUB jump by searching `Program::lines`.
- `gpu_base_` (double) tracks the current GPU array base index (set by `OPTION GPU BASE`).
- Host array base is managed by `Environment::array_base_`.
- GPU kernels are registered in `kernel_defs_` on `GPU KERNEL` encounter, compiled on first `GPU GOSUB`.
- I/O is injected via `out_` / `in_` streams (enables test capture).

## Data Flow
```
Program (AST) → Interpreter::run() → side effects (output, GPU launches)
                    ↓
              Environment (variables/arrays)
                    ↓
              GpuRuntime (device memory, kernel launch)
```

## Interfaces
- `Interpreter(std::ostream& out, std::istream& in)` — constructor — `src/interpreter.h:17`
- `void run(const std::string& source)` — parse + execute — `src/interpreter.h:20`
- `void run(const Program& program)` — execute pre-parsed AST — `src/interpreter.h:21`

## Dependencies
- **Environment** — variable/array storage.
- **GpuRuntime** — GPU device operations.
- **GPU Codegen** — kernel AST → HIP C++ source.
- **MpiRuntime** — MPI send/recv, barrier, rank/size.
- **Lexer + Parser** — used internally by `run(string)`.

## Also Load
- [Environment](environment.md) — runtime state.
- [GPU Codegen](gpu_codegen.md) — kernel translation.
- [GPU Runtime](gpu_runtime.md) — HIP device operations.

## Known Limitations
- Single-threaded CPU execution.
- No `DATA`/`READ`/`RESTORE` execution.

## Last Verified
Commit: 2fb3c7e
Date: 2026-04-01
