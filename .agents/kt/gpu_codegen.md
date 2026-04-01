# GPU Codegen

## Responsibility
Translates a `GpuKernelStmt` AST into HIP C++ source code suitable for hiprtc runtime
compilation. Produces `extern "C" __global__` kernel functions with all parameters as
`double*` (arrays) or `double` (scalars).

## Core Concepts
- **String building**: Direct AST-to-string translation, no intermediate IR.
- **gpu_base offset**: When `OPTION GPU BASE` is non-zero, array accesses are offset (`(int)(expr - base)`) and index intrinsics (`THREAD_IDX`, `BLOCK_IDX`) are offset by `+gpu_base`.
- **printf casting**: HIP intrinsics return `unsigned int`; PRINT generates `printf()` calls that cast args to `(double)` for `%g` format specifier.
- **format_base() helper**: Formats the base value for codegen (e.g., `0.5` stays as `0.5`, integers omit decimal).

## Key Invariants
- Kernel parameters: arrays → `double*`, scalars → `double`. Determined by whether the param is used as an array in the kernel body.
- GPU intrinsic mapping: `THREAD_IDX(d)` → `threadIdx.{x,y,z}`, `BLOCK_IDX(d)` → `blockIdx.{x,y,z}`, etc. Dimension d is 1-indexed (1=x, 2=y, 3=z).
- `BLOCK_DIM` and `GRID_DIM` are NOT offset by gpu_base (they are sizes, not indices).
- Kernel body restrictions enforced at parse time, not codegen time.

## Data Flow
```
GpuKernelStmt + gpu_base → generate_kernel_source() → std::string (HIP C++ source)
                                                           ↓
                                                     hiprtc compile (via GpuRuntime)
```

## Interfaces
- `std::string generate_kernel_source(const GpuKernelStmt& kernel, double gpu_base)` — `src/gpu_codegen.h:11`

## Dependencies
- **AST** — reads `GpuKernelStmt` and expression nodes.

## Also Load
- [GPU Runtime](gpu_runtime.md) — consumes generated source.
- [Interpreter](interpreter.md) — orchestrates codegen + launch.

## Known Limitations
- No shared memory (`__shared__`) support.
- No support for local variables within kernels (all data via parameters).
- String operations in kernels limited to PRINT with string literals.

## Last Verified
Commit: a315030
Date: 2026-04-01
