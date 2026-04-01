# Glossary

- **BASIC line number**: Integer prefix on every source line (e.g., `10`, `20`, `30`) that determines execution order and serves as GOTO/GOSUB targets.
- **OPTION BASE**: Statement to set array indexing base. Host default is 1 (classic BASIC), GPU default is 0 (matching HIP/C). Supports 0, 0.5, and 1.
- **hiprtc**: HIP Runtime Compilation — AMD's API for compiling HIP C++ kernels at runtime from source strings.
- **GPU GOSUB**: Launches a GPU kernel, analogous to CPU GOSUB but dispatches to the GPU. Synchronous (blocks until kernel completes).
- **GPU DIM**: Allocates device memory (hipMalloc) for a named GPU array.
- **GPU COPY**: Transfers data between host and device memory (hipMemcpy).
- **GPU KERNEL ... END KERNEL**: Multi-line block defining a GPU kernel in BASIC syntax. Compiled to HIP C++ via codegen.
- **Kernel intrinsics**: Built-in GPU functions (`THREAD_IDX`, `BLOCK_IDX`, `BLOCK_DIM`, `GRID_DIM`) that map to HIP equivalents. Parameterized by dimension (1=x, 2=y, 3=z).
- **Value**: Runtime type `std::variant<double, std::string>` — all BASIC values are either numbers or strings.
- **ExprPtr / StmtPtr**: `std::unique_ptr` aliases for AST node ownership.
- **SPMD**: Single Program, Multiple Data — MPI execution model where all ranks run the same program.
- **THRU**: Keyword for MPI range syntax (`A(1) THRU A(N)`), chosen over `TO` to avoid ambiguity with destination.
- **rocBAS**: The executable name for the interpreter.
