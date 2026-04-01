# GPU Runtime

## Responsibility
HIP device management layer. Handles device initialization, GPU memory allocation/free,
host-device memory transfers, hiprtc kernel compilation, and kernel launch.

## Core Concepts
- **DeviceArray**: Tracks a `void*` device pointer and element count per named GPU array.
- **CompiledKernel**: Holds `hipModule_t` and `hipFunction_t` for a compiled kernel.
- **Lazy availability**: `available_` is set at construction based on whether a HIP device is found.
- **Conditional compilation**: Entire implementation is `#ifdef ROCBAS_HAS_HIP`; without it, a stub runtime reports "GPU not available".

## Key Invariants
- All operations check `available_` via `check_available()` and throw if no GPU.
- `gpu_dim()` allocates via `hipMalloc`, stores in `device_arrays_`.
- `compile_kernel()` uses `hiprtcCreateProgram` / `hiprtcCompileProgram` / `hipModuleLoadData`.
- `launch_kernel()` uses `hipModuleLaunchKernel` with `hipDeviceSynchronize()` after every launch (synchronous).
- Destructor calls `cleanup()` to free all device arrays and unload all modules.

## Data Flow
```
Interpreter → gpu_dim("GA", 1024)           → hipMalloc
Interpreter → gpu_copy_host_to_device(...)  → hipMemcpy H2D
Interpreter → compile_kernel(name, src)     → hiprtc compile → hipModule
Interpreter → launch_kernel(name, args, grid, block) → hipModuleLaunchKernel + sync
Interpreter → gpu_copy_device_to_host(...)  → hipMemcpy D2H
Interpreter → gpu_free("GA")               → hipFree
```

## Interfaces
- `GpuRuntime()` — constructor (device init) — `src/gpu_runtime.h:16`
- `void gpu_dim(name, num_elements)` — allocate — `src/gpu_runtime.h:27`
- `void gpu_copy_host_to_device(...)` / `gpu_copy_device_to_host(...)` — transfers — `src/gpu_runtime.h:29-32`
- `void compile_kernel(name, hip_source)` — hiprtc compile — `src/gpu_runtime.h:35`
- `void launch_kernel(name, arg_names, scalar_args, grid_dims, block_dims)` — launch — `src/gpu_runtime.h:36`
- `bool is_available()` — GPU presence check — `src/gpu_runtime.h:47`

## Dependencies
- **HIP/ROCm** (optional, via `ROCBAS_HAS_HIP` define).

## Also Load
- [GPU Codegen](gpu_codegen.md) — generates the HIP C++ source that this module compiles.
- [Interpreter](interpreter.md) — orchestrates all GPU operations.

## Performance Constraints
- Synchronous kernel launches (hipDeviceSynchronize after every launch).
- No streams — single default stream.
- No LDS/shared memory.

## Known Limitations
- 1D and 3D grid/block dims supported; no 2D shorthand.
- No kernel caching across runs (recompiled each time interpreter is created).
- No GPU-aware MPI (data must transit through host).

## Last Verified
Commit: a315030
Date: 2026-04-01
