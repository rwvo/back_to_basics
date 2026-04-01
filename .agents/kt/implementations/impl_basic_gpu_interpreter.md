# Implementation: BASIC Interpreter with GPU Extensions

## Status
- [x] In Progress (Phase 2 complete, Phase 3 design complete)
- [ ] Blocked
- [ ] Done

## Objective

Build a classic BASIC language interpreter in C++17 with AMD GPU compute extensions.
The interpreter supports line-numbered BASIC programs with GPU kernel definition and
launch via ROCm/HIP and hiprtc runtime compilation.

## Implementation Contract

### Goal
A working interpreter that can:
1. Parse and execute classic BASIC programs (line numbers, GOTO, GOSUB, FOR/NEXT, etc.)
2. Define GPU kernels inline in BASIC syntax
3. Allocate/copy device memory and launch GPU kernels with configurable grid/block dims
4. (Future v3) Communicate between MPI ranks for multi-node execution

### Non-Goals / Invariants
- Full compatibility with any specific BASIC dialect (we define our own)
- Optimizing CPU-side interpretation performance
- Unified memory or automatic host/device management
- MPI in v1 or v2 (architecture should accommodate it, but no implementation yet)

### Verification Gates
- Build: `cmake --build build/` compiles cleanly
- Test: `ctest --test-dir build/` — all GTest unit tests pass
- Integration: Example .bas programs run and produce expected output
- GPU (v2): Vector addition kernel runs on AMD GPU and produces correct results
- MPI (v3): Heat diffusion demo runs across 4 ranks with correct results

### Methodology: Test-Driven Development (TDD)
- **Red-green-refactor**: Write failing tests first, then implement to make them pass
- **Test framework**: Google Test (GTest), fetched via CMake FetchContent
- **Each micro-step**: Write tests → verify they fail → implement → verify they pass
- **Test granularity**: Unit tests per component (lexer, parser, interpreter); integration
  tests for end-to-end .bas program execution

## Design Decisions

### BASIC Dialect — Classic

Line-numbered, maximally old-fashioned:
```basic
10 REM Hello World
20 PRINT "Hello, World!"
30 END
```

Supported statements (v1):
- `REM` — comments
- `LET var = expr` — assignment (LET optional per tradition)
- `PRINT expr [; expr ...]` — output (`;` suppresses newline between items)
- `INPUT [prompt;] var` — read from stdin
- `GOTO line` — unconditional jump
- `GOSUB line` / `RETURN` — subroutine call/return
- `IF expr THEN statement` — conditional (single-line)
- `FOR var = expr TO expr [STEP expr]` / `NEXT var` — loop
- `DIM var(size [, size ...])` — array declaration
- `END` — halt execution
- `DATA` / `READ` / `RESTORE` — inline data (stretch goal for v1)

Supported expressions:
- Arithmetic: `+`, `-`, `*`, `/`, `^` (exponentiation)
- Comparison: `=`, `<>`, `<`, `>`, `<=`, `>=`
- Logical: `AND`, `OR`, `NOT`
- String concatenation: `+`
- Built-in functions: `ABS`, `INT`, `SQR`, `RND`, `SIN`, `COS`, `TAN`, `LEN`,
  `LEFT$`, `RIGHT$`, `MID$`, `CHR$`, `ASC`, `VAL`, `STR$`, `TAB`

Variable conventions:
- Undecorated names are numeric (float64): `X`, `COUNT`
- `$`-suffixed names are strings: `N$`, `LINE$`
- Arrays declared with DIM, 1-indexed by default (configurable via OPTION BASE)

### GPU Extensions (v2)

```basic
80 GPU DIM GA(1024), GB(1024), GC(1024)
90 GPU COPY A TO GA
100 GPU COPY B TO GB
110 GPU KERNEL VECADD(X, Y, Z, N)
120   LET I = BLOCK_IDX(1) * BLOCK_DIM(1) + THREAD_IDX(1)
130   IF I < N THEN LET Z(I) = X(I) + Y(I)
140 END KERNEL
150 GPU GOSUB VECADD(GA, GB, GC, 1024) WITH 4 BLOCKS OF 256
160 GPU COPY GC TO C
```

New statements:
- `GPU DIM var(size)` — allocate device memory
- `GPU COPY hostvar TO devvar` / `GPU COPY devvar TO hostvar` — transfer data
- `GPU KERNEL name(params) ... END KERNEL` — define kernel (multi-line)
- `GPU GOSUB name(args) WITH n BLOCKS OF m` — call GPU kernel (1D grid)
- `GPU GOSUB name(args) WITH (nx,ny,nz) BLOCKS OF (bx,by,bz)` — call GPU kernel (3D grid)
- `GPU FREE var` — deallocate device memory (optional, auto-free at END)
- `OPTION BASE N` — set host array base index (0, 0.5, or 1; default 1)
- `OPTION GPU BASE N` — set GPU array base index (0, 0.5, or 1; default 0)

Kernel intrinsics (parameterized, 1-indexed dimensions, direct HIP mapping):
- `THREAD_IDX(d)` → `threadIdx.{x,y,z}` — thread index within block
- `BLOCK_IDX(d)` → `blockIdx.{x,y,z}` — block index within grid
- `BLOCK_DIM(d)` → `blockDim.{x,y,z}` — number of threads per block
- `GRID_DIM(d)` → `gridDim.{x,y,z}` — number of blocks in grid

where d = 1 (x), 2 (y), 3 (z). No global thread ID intrinsic; users compute
it explicitly, just like in HIP:
```basic
LET I = BLOCK_IDX(1) * BLOCK_DIM(1) + THREAD_IDX(1)
```

Kernel body restrictions:
- No INPUT, GOTO, GOSUB inside kernels
- No string operations (except PRINT string literals)
- Array access only on kernel parameters
- Arithmetic, IF/THEN, PRINT only
- PRINT emits a single `printf()` per statement for atomic output

GPU execution model (v2 simplifications):
- **Synchronous launches**: GPU GOSUB blocks until kernel completes
  (implicit `hipDeviceSynchronize()` after every launch). Just like CPU
  GOSUB — when it returns, the work is done.
- **No streams**: Single default stream, everything serialized.
- **No LDS/shared memory**: All data through global memory via kernel
  parameters. LDS support (e.g., `GPU SHARED DIM`) deferred to later version.

Implementation: The interpreter translates the kernel AST into HIP C++ source code
and compiles it at runtime using hiprtc. Kernel parameters are mapped to HIP kernel
arguments.

### Execution Modes

```
rocBAS test.bas                              — run a program (single process)
rocBAS                                       — interactive REPL
mpirun -n 4 rocBAS test.bas                  — MPI launch (v3)
mpirun -n 4 rocBAS                           — MPI interactive REPL (v3)
mpirun -n 4 rocBAS --mpi-print=all test.bas  — all ranks print (no prefix)
mpirun -n 4 rocBAS --mpi-print=rank test.bas — all ranks print (rank prefix)
```

### MPI Extensions (v3)

```basic
10 MPI INIT
20 LET RANK = MPI RANK
30 LET SIZE = MPI SIZE
40 REM ... compute ...
50 MPI SEND A TO RANK+1 TAG 1
60 MPI RECV B FROM RANK-1 TAG 1
70 REM Range form: send elements A(1) through A(N)
80 MPI SEND A(1) THRU A(N) TO RANK+1 TAG 2
90 MPI RECV B(1) THRU B(N) FROM RANK-1 TAG 2
100 MPI BARRIER
110 MPI FINALIZE
```

New statements (v3):
- `MPI INIT` — initialize MPI (`MPI_Init`); works without `mpirun` (rank 0, size 1)
- `MPI FINALIZE` — finalize MPI (`MPI_Finalize`)
- `MPI SEND array TO dest TAG tag` — blocking send of whole array
- `MPI SEND array(lo) THRU array(hi) TO dest TAG tag` — blocking send of array range
- `MPI RECV array FROM src TAG tag` — blocking receive into whole array
- `MPI RECV array(lo) THRU array(hi) FROM src TAG tag` — blocking receive into range
- `MPI BARRIER` — synchronize all ranks (`MPI_Barrier`)

New expressions (v3):
- `MPI RANK` — returns this process's rank (`MPI_Comm_rank`). Two-token expression
  (`KW_MPI` + identifier `"RANK"`), parsed in `parse_primary()`. AST node: `MpiIntrinsic`.
- `MPI SIZE` — returns number of ranks (`MPI_Comm_size`). Same mechanism as `MPI RANK`.

New lexer token: `KW_THRU` — used in range-based `MPI SEND`/`MPI RECV`
Note: `RANK` and `SIZE` stay as regular identifiers (not keywords) to avoid reserving common variable names.

Design decisions:
- **Real MPI**: Link against MPI directly (no fake-MPI IPC layer). Build is optional
  (`-DROCBAS_ENABLE_MPI=ON`, `#ifdef ROCBAS_HAS_MPI`), mirroring the HIP pattern.
  Stub runtime when MPI is disabled.
- **SPMD launch**: Programs run via `mpirun -n N rocBAS prog.bas`. Each process
  gets its own interpreter instance with a unique rank.
- **Single-process fallback**: Running without `mpirun` is valid — `MPI INIT` succeeds
  with rank 0, size 1. Programs with MPI statements work as single-process.
- **Blocking only (v3)**: All sends/receives are blocking (`MPI_Send`/`MPI_Recv`).
  Non-blocking (`MPI ISEND`/`MPI IRECV`) deferred to a later version.
- **BARRIER only (v3)**: No collectives beyond `MPI BARRIER` initially. `MPI REDUCE`,
  `MPI ALLREDUCE`, `MPI BCAST` deferred to a later version.
- **No GPU-aware MPI (v3)**: Data must be on host for MPI transfers. Pattern:
  `GPU COPY` to host → `MPI SEND` → `MPI RECV` → `GPU COPY` to device.
  GPU-aware MPI (sending directly from device memory) deferred to a later version.
- Interpreter state is per-rank (each process has its own interpreter instance)
- Boundary exchange pattern: send edge data to neighbors, receive from neighbors
- **MPI print control** (`--mpi-print` flag):
  - `--mpi-print=rank0` (default): Only rank 0 prints. Non-zero ranks write to a null
    stream. Cleanest output for typical programs.
  - `--mpi-print=all`: All ranks print, no prefix. Output may interleave.
  - `--mpi-print=rank`: All ranks print, each line prefixed with `[N] ` (rank number).
    Useful for debugging. Implemented via a filtering streambuf on `out_` that prepends
    the prefix after each newline.
- **MPI interactive REPL**: `mpirun -n N rocBAS` (no file argument) enters interactive
  mode. Rank 0 runs the REPL loop (reads lines, handles LIST/NEW/QUIT). On `RUN`,
  rank 0 broadcasts the program source to all ranks, all parse and execute together
  (SPMD). Non-zero ranks block on `MPI_Bcast` between RUN invocations. INPUT statements
  in MPI mode read on rank 0 and broadcast the value to all ranks.

Target demo: 2D heat diffusion across 4 ranks, each with a GPU tile.

## Architecture

### Directory Structure (planned)
```
src/
  main.cpp              — entry point, REPL and file execution
  lexer.h / lexer.cpp   — tokenizer
  token.h               — token types
  parser.h / parser.cpp — recursive descent parser
  ast.h                 — AST node definitions
  interpreter.h / .cpp  — tree-walk interpreter
  environment.h / .cpp  — variable storage, call stack
  gpu_runtime.h / .cpp  — HIP device management, memory, kernel launch (v2)
  gpu_codegen.h / .cpp  — BASIC kernel AST → HIP C++ source (v2)
  mpi_runtime.h / .cpp  — MPI wrapper: init, send, recv, barrier (v3)
tests/
  lexer_test.cpp        — lexer unit tests
  ast_test.cpp          — AST node construction tests
  parser_test.cpp       — parser unit tests
  interpreter_test.cpp  — interpreter unit tests (incl. OPTION BASE)
  gpu_parser_test.cpp   — GPU statement parsing tests
  gpu_codegen_test.cpp  — GPU kernel codegen tests (incl. OPTION GPU BASE)
  gpu_integration_test.cpp — end-to-end GPU pipeline tests
  mpi_test.cpp          — MPI statement parsing and single-rank tests (v3)
CMakeLists.txt
examples/
  hello.bas
  fibonacci.bas
  guess.bas
  vecadd.bas            — (v2) GPU vector addition
  heat.bas              — (v3) 2D heat diffusion with MPI + GPU
```

### Key Design Choices

1. **Tree-walk interpreter** (not bytecode): Simpler to implement, debug, and extend.
   Performance is irrelevant for a BASIC interpreter.

2. **AST nodes are `std::variant`**: Simpler than class hierarchy, sufficient for
   all statement/expression types including GPU and MPI nodes.

3. **GPU codegen as string building**: Translate kernel AST nodes directly to HIP C++
   strings. No intermediate IR. hiprtc compiles the generated source.

4. **Modular runtime backends**: CPU interpreter, GPU runtime, and future MPI runtime
   are separate modules. The interpreter dispatches to the appropriate backend based on
   statement type.

## Scope

### Expected Files
All files are new (greenfield project).

### Risks
- **hiprtc availability**: Requires ROCm installation. CPU-only interpreter (v1) has no
  external dependencies beyond a C++17 compiler.
- **BASIC grammar ambiguity**: Classic BASIC has context-dependent parsing (e.g., `LET`
  is optional, `IF/THEN` with line number vs statement). Careful lexer/parser design needed.
- **Kernel codegen correctness**: Translating BASIC semantics to HIP C++ correctly,
  especially array indexing (BASIC is 1-indexed, C++ is 0-indexed).
- **MPI testing**: Multi-rank tests require `mpirun`, so they can't run as regular
  unit tests. Need a separate test target or CTest label for MPI tests.

## Plan of Record

### Phase 1: CPU-Only BASIC Interpreter (v1)

TDD workflow for each step: write tests (red) → stub returns "unimplemented" / fails →
implement until tests pass (green) → refactor if needed.

1. [x] **Project scaffolding** — CMake setup with GTest (FetchContent), directory structure,
   main.cpp stub, test harness with a single placeholder test
   - Gate: `cmake --build build/` compiles; `ctest --test-dir build/` runs one passing test
   - Initial state: placeholder test passes, all subsequent tests will fail until implemented

2. [x] **Token types and lexer** — Define tokens, implement tokenizer
   - Tests first: tokenize `10 PRINT "HELLO"`, tokenize keywords, numbers, strings, operators
   - Stub: lexer returns empty token list → tests fail (red)
   - Implement: full lexer → tests pass (green)
   - Gate: `ctest --test-dir build/` — lexer_test passes

3. [x] **AST node definitions** — Define node types for all v1 statements/expressions
   - Tests first: construct AST nodes, verify structure
   - Gate: Compiles; AST node tests pass

4. [x] **Parser: expressions** — Recursive descent expression parsing
   - Tests first: parse `2 + 3 * 4`, parse comparisons, parse function calls
   - Stub: parser throws "unimplemented" → tests fail
   - Implement: expression parser → tests pass
   - Gate: parser expression tests pass

5. [x] **Parser: statements** — Parse LET, PRINT, GOTO, IF/THEN, FOR/NEXT, GOSUB/RETURN,
   DIM, END, REM, INPUT
   - Tests first: parse each statement type, parse multi-line programs
   - Implement: statement parsing → tests pass
   - Gate: parser statement tests pass

6. [x] **Interpreter core** — Environment (variable storage), expression evaluation
   - Tests first: evaluate numeric expressions, variable assignment and retrieval
   - Stub: interpreter throws "unimplemented" → tests fail
   - Implement: environment + expression evaluator → tests pass
   - Gate: interpreter core tests pass

7. [x] **Basic statements** — Implement LET, PRINT, END, REM, GOTO
   - Tests first: run `10 PRINT "HELLO"` / `20 END`, capture output, verify
   - Implement: statement execution → tests pass
   - Gate: basic statement tests pass

8. [x] **Control flow** — IF/THEN, FOR/NEXT, GOSUB/RETURN
   - Tests first: run programs with loops, conditionals, subroutines; verify output
   - Implement: control flow → tests pass
   - Gate: control flow tests pass; Fibonacci program produces correct output

9. [x] **Arrays** — DIM, array element access/assignment
   - Tests first: DIM, read/write elements, bounds checking
   - Implement: array support → tests pass
   - Gate: array tests pass

10. [x] **INPUT and strings** — INPUT statement, string variables, string functions
    - Tests first: string assignment, concatenation, string functions (LEN, LEFT$, etc.)
    - Implement: string support → tests pass (INPUT tested via stream injection)
    - Gate: string tests pass

11. [x] **Built-in functions** — ABS, INT, SQR, RND, SIN, COS, etc.
    - Tests first: each function with known inputs/outputs
    - Implement: built-in function dispatch → tests pass
    - Gate: built-in function tests pass

12. [x] **Error handling** — Line number reporting, type errors, undefined variable errors
    - Tests first: verify specific error messages for bad programs
    - Implement: error reporting → tests pass
    - Gate: error handling tests pass

13. [x] **File execution and REPL** — Load .bas files from command line, interactive REPL mode
    - Tests first: integration tests loading .bas files, verifying output
    - Implement: file loader + REPL → tests pass
    - Gate: `./rocBAS examples/hello.bas` works; integration tests pass

### Phase 2: GPU Extensions (v2)

14. [x] **GPU AST nodes and parser extensions** — New AST nodes for GPU statements; extend lexer/parser
    - Gate: Parses vecadd.bas into AST with GPU nodes

15. [x] **HIP runtime integration** — Device init, error handling, cmake HIP detection
    - Gate: Compiles with HIP; prints GPU device name

16. [x] **GPU memory management** — GPU DIM (hipMalloc), GPU COPY (hipMemcpy), GPU FREE
    - Gate: Allocate device array, copy data round-trip, verify contents

17. [x] **Kernel codegen** — Translate GPU KERNEL AST → HIP C++ source string
    - Gate: Generated HIP source compiles with hiprtc

18. [x] **Kernel launch** — GPU GOSUB implementation, hiprtc compile + hipModuleLaunchKernel
    - Gate: vecadd.bas runs on GPU and produces correct results

19. [x] **Kernel intrinsics and validation** — THREAD_ID/BLOCK_ID/etc., kernel body validation
    - Gate: Kernels using all intrinsics work; invalid kernel bodies produce errors

### Phase 3: MPI Extensions (v3)

20. [x] **MPI design decisions** — Syntax, backend strategy, scope
    - Decided: real MPI, `mpirun` launch, blocking send/recv, whole-array + range (THRU),
      MPI_RANK()/MPI_SIZE() as functions, BARRIER only, no GPU-aware MPI yet
    - Gate: Dossier updated with decisions

21. [ ] **MPI AST nodes and parser extensions** — New tokens (KW_MPI, KW_THRU, etc.),
    AST nodes (MpiInitStmt, MpiSendStmt, MpiRecvStmt, MpiBarrierStmt, MpiFinalizeStmt),
    extend parser and lexer
    - Gate: Parses MPI programs into AST; parser tests pass

22. [ ] **MPI runtime module** — `mpi_runtime.h/.cpp`, CMake MPI detection
    (`-DROCBAS_ENABLE_MPI=ON`), `#ifdef ROCBAS_HAS_MPI` conditional compilation,
    stub runtime when disabled
    - Gate: Compiles with MPI; `MPI_RANK()` returns 0 in single-process mode

23. [ ] **MPI SEND/RECV** — Blocking send/recv for whole arrays and ranges,
    interpreter dispatch to mpi_runtime
    - Gate: Two-rank send/recv test passes under `mpirun -n 2`

24. [ ] **MPI BARRIER** — Synchronize all ranks
    - Gate: Multi-rank barrier test passes

25. [ ] **Heat diffusion demo** — `examples/heat.bas`, 2D heat diffusion
    across 4 ranks with GPU tiles, boundary exchange via MPI SEND/RECV
    - Gate: `mpirun -n 4 rocBAS examples/heat.bas` produces correct output

### Current Step
Phase 3 in progress — MPI design decisions complete (step 20). Next: step 21 (AST/parser).

## Progress Log
<!-- Append updates, don't delete -->

### Session 2026-03-09
- Completed: Design discussion, dossier creation
- Decisions: Classic BASIC dialect, C++17, tree-walk interpreter, hiprtc for GPU kernels
- Next: Step 1 — project scaffolding

### Session 2026-03-09 (continued)
- Completed: All Phase 1 steps (1-13) — full CPU-only BASIC interpreter
- 127 tests passing (24 lexer, 10 AST, 32 parser, 61 interpreter)
- Design decisions resolved: std::variant for AST, LET optional, executable name rocBAS
- Features: lexer, parser, interpreter, environment, arrays, strings, built-in functions,
  control flow (IF/THEN, FOR/NEXT, GOSUB/RETURN), GOTO, INPUT, REPL
- Examples: hello.bas, fibonacci.bas, guess.bas
- Next: Phase 2 — GPU extensions

### Session 2026-03-09 (Phase 2)
- Completed: Steps 14-19 — all GPU extensions implemented
- 160 tests passing at Phase 2 completion (24 lexer, 10 AST, 32 parser, 61 interpreter,
  9 GPU parser, 14 GPU runtime, 7 GPU codegen, 3 GPU integration)
- Design decision: GPU arrays default to 0-based indexing (matching HIP convention),
  CPU-side BASIC arrays default to 1-based; both configurable via OPTION BASE
- Architecture: gpu_runtime.h/.cpp (HIP device management, memory, kernel launch),
  gpu_codegen.h/.cpp (BASIC kernel AST → HIP C++ source)
- vecadd.bas example runs end-to-end on AMD GPU
- CPU-only build still works (stub GPU runtime when ROCBAS_HAS_HIP not defined)

### Session 2026-03-09 (OPTION BASE)
- Added `OPTION BASE N` and `OPTION GPU BASE N` statements (N=0 or 1)
- New tokens: KW_OPTION, KW_BASE; new AST node: OptionBaseStmt
- Environment::flat_index now accepts configurable base parameter
- GPU codegen subtracts gpu_base in array access when non-zero
- THREAD_IDX/BLOCK_IDX offset by +gpu_base; BLOCK_DIM/GRID_DIM unchanged (sizes vs indices)
- GPU printf args cast to (double) — HIP intrinsics return unsigned int, %g expects double
- 175 tests passing (5 new interpreter tests, 3 new GPU codegen tests)

### Session 2026-03-09 (OPTION BASE 0.5)
- Added `OPTION BASE 0.5` and `OPTION GPU BASE 0.5` — half-based array indexing
- AST: `OptionBaseStmt::base` changed from `int` to `double`
- Environment: `flat_index`, `set_array`, `get_array` take `vector<double>` indices and `double` base;
  index arithmetic: `int adj = static_cast<int>(indices[i] - base)` (exact for 0.5 in IEEE 754)
- Parser: `parse_option()` now accepts `FLOAT_LITERAL` in addition to `INTEGER_LITERAL`;
  validates base ∈ {0, 0.5, 1}
- Interpreter: `gpu_base_` changed from `int` to `double`; array index vectors changed from
  `vector<int>` to `vector<double>` (no truncation before passing to environment)
- GPU codegen: `g_gpu_base` changed from `int` to `double`; added `format_base()` helper;
  array offset moved inside `(int)` cast: `(int)(expr - 0.5)` instead of `(int)(expr) - 0.5`
  (latter would produce a double used as a C++ array index)
- 181 tests passing (4 new interpreter tests, 2 new GPU codegen tests)

## Rejected Approaches

- **Fake MPI (IPC-based)**: Originally planned as a first step — `rocBAS --ranks N` would
  fork processes with IPC (pipes/shmem). Rejected in favor of linking against real MPI
  directly, since ROCm environments typically have MPI available and the fake layer would
  be throwaway work.
- **`MPI_RANK()` / `MPI_SIZE()` as built-in functions**: Originally chosen for consistency
  with existing function infrastructure. Reversed in favor of `MPI RANK` / `MPI SIZE`
  two-token expressions for a more authentic BASIC feel. Implementation: parsed in
  `parse_primary()` as `MpiIntrinsic` AST nodes (similar to `GpuIntrinsic`).
- **Double-TO syntax for ranges**: `MPI SEND A(1) TO A(N) TO dest TAG t` — uses `TO`
  for both range end and destination. Rejected in favor of `THRU` keyword
  (`A(1) THRU A(N)`) for clarity.
- **Colon range syntax**: `MPI SEND A(1:N) TO dest` — Fortran-style, clean but introduces
  colon-as-range-operator to the lexer. Rejected in favor of `THRU` for a more BASIC feel.

## Open Questions
- ~~AST representation: `std::variant` vs class hierarchy?~~ **Decided: `std::variant`**
- ~~Should `LET` be optional in assignments?~~ **Decided: yes, optional.**
- ~~0-indexed vs 1-indexed arrays on GPU side?~~ **Decided: Defaults are GPU=0-based,
  host=1-based. Both are now configurable via `OPTION BASE N` (host) and
  `OPTION GPU BASE N` (GPU), where N is 0, 0.5, or 1.**
- ~~Executable name?~~ **Decided: `rocBAS`**
- ~~MPI backend: fake IPC vs real MPI?~~ **Decided: real MPI, optional build.**
- ~~MPI launch model?~~ **Decided: `mpirun -n N rocBAS prog.bas` (SPMD).**
- ~~MPI RANK/SIZE: special syntax vs functions?~~ **Decided: `MPI RANK`, `MPI SIZE` (two-token expressions, not function calls). More BASIC feel.**
- ~~MPI range syntax?~~ **Decided: `THRU` keyword — `A(1) THRU A(N)`.**
- ~~MPI without mpirun?~~ **Decided: works as rank 0, size 1 (single-process MPI).**
- ~~MPI print output from multiple ranks?~~ **Decided: `--mpi-print` flag with three
  modes: `rank0` (default, only rank 0 prints), `all` (all print, no prefix),
  `rank` (all print, `[N] ` prefix).**
- ~~MPI interactive REPL?~~ **Decided: supported. Rank 0 runs REPL, broadcasts source
  on RUN. Non-zero ranks wait at MPI_Bcast.**

## Last Verified
Commit: 7dad397
Date: 2026-03-09
