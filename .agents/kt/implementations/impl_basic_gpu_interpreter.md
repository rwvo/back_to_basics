# Implementation: BASIC Interpreter with GPU Extensions

## Status
- [x] In Progress
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
- Arrays declared with DIM, 1-indexed by default

### GPU Extensions (v2)

```basic
80 GPU DIM GA(1024), GB(1024), GC(1024)
90 GPU COPY A TO GA
100 GPU COPY B TO GB
110 GPU KERNEL VECADD(X, Y, Z, N)
120   LET I = BLOCK_IDX(1) * BLOCK_DIM(1) + THREAD_IDX(1)
130   IF I < N THEN LET Z(I) = X(I) + Y(I)
140 END KERNEL
150 GPU LAUNCH VECADD(GA, GB, GC, 1024) WITH 4 BLOCKS OF 256
160 GPU COPY GC TO C
```

New statements:
- `GPU DIM var(size)` — allocate device memory
- `GPU COPY hostvar TO devvar` / `GPU COPY devvar TO hostvar` — transfer data
- `GPU KERNEL name(params) ... END KERNEL` — define kernel (multi-line)
- `GPU LAUNCH name(args) WITH n BLOCKS OF m` — launch kernel (1D grid)
- `GPU LAUNCH name(args) WITH (nx,ny,nz) BLOCKS OF (bx,by,bz)` — launch (3D grid)
- `GPU FREE var` — deallocate device memory (optional, auto-free at END)

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
- No PRINT, INPUT, GOTO, GOSUB inside kernels
- No string operations
- Array access only on kernel parameters
- Arithmetic and IF/THEN only

Implementation: The interpreter translates the kernel AST into HIP C++ source code
and compiles it at runtime using hiprtc. Kernel parameters are mapped to HIP kernel
arguments.

### Execution Modes

```
rocBAS test.bas              — run a BASIC program (single process)
rocBAS                       — interactive REPL
rocBAS --ranks 4 test.bas    — fake MPI: fork 4 processes with IPC (v3)
```

Future option (if linked against real MPI):
```
mpirun -n 4 rocBAS test.bas  — real MPI launch (optional, v3+)
```

### MPI Extensions (future v3, design notes only)

```basic
10 MPI INIT
20 LET RANK = MPI RANK
30 LET SIZE = MPI SIZE
40 REM ... compute ...
50 MPI SEND A(1) TO A(N) TO RANK+1 TAG 1
60 MPI RECV B(1) TO B(N) FROM RANK-1 TAG 1
70 MPI BARRIER
80 MPI FINALIZE
```

Architecture considerations for v3:
- **Fake MPI first**: `rocBAS --ranks N` forks N child processes, sets up IPC
  between them (pipes, shared memory, or sockets). Each child runs the same .bas
  program with a different `MPI RANK` value. No MPI dependency required.
- **Abstract backend**: BASIC-level MPI statements (`MPI SEND`, `MPI RECV`, etc.)
  dispatch through a backend interface, so the IPC mechanism is swappable.
  Start with fake IPC; optionally add real MPI backend later without changing the
  language.
- **Real MPI (optional, later)**: If rocBAS is compiled with MPI support and
  launched via `mpirun`, detect this at startup (MPI_Init succeeds) and use the
  real MPI backend instead of fake IPC. Same BASIC programs work either way.
- Interpreter state is per-rank (each process has its own interpreter instance)
- Boundary exchange pattern: send edge data to neighbors, receive from neighbors
- GPU+MPI: GPU COPY from device to host, MPI SEND, MPI RECV, GPU COPY to device

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
tests/
  lexer_test.cpp        — lexer unit tests
  parser_test.cpp       — parser unit tests
  interpreter_test.cpp  — interpreter unit tests
  integration_test.cpp  — end-to-end .bas program tests
CMakeLists.txt
examples/
  hello.bas
  fibonacci.bas
  guess.bas
  vecadd.bas            — (v2) GPU vector addition
```

### Key Design Choices

1. **Tree-walk interpreter** (not bytecode): Simpler to implement, debug, and extend.
   Performance is irrelevant for a BASIC interpreter.

2. **AST nodes are `std::variant` or class hierarchy**: TBD during implementation.
   Variant is simpler; class hierarchy is more extensible for GPU/MPI nodes.

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

14. [ ] **GPU AST nodes and parser extensions** — New AST nodes for GPU statements; extend lexer/parser
    - Gate: Parses vecadd.bas into AST with GPU nodes

15. [ ] **HIP runtime integration** — Device init, error handling, cmake HIP detection
    - Gate: Compiles with HIP; prints GPU device name

16. [ ] **GPU memory management** — GPU DIM (hipMalloc), GPU COPY (hipMemcpy), GPU FREE
    - Gate: Allocate device array, copy data round-trip, verify contents

17. [ ] **Kernel codegen** — Translate GPU KERNEL AST → HIP C++ source string
    - Gate: Generated HIP source compiles with hiprtc

18. [ ] **Kernel launch** — GPU LAUNCH implementation, hiprtc compile + hipModuleLaunchKernel
    - Gate: vecadd.bas runs on GPU and produces correct results

19. [ ] **Kernel intrinsics and validation** — THREAD_ID/BLOCK_ID/etc., kernel body validation
    - Gate: Kernels using all intrinsics work; invalid kernel bodies produce errors

### Phase 3: MPI Extensions (v3, future)

20. [ ] Design MPI syntax and semantics (detailed dossier TBD)
21. [ ] Implement MPI runtime module
22. [ ] Heat diffusion demo across multiple ranks

### Current Step
Step 14: GPU AST nodes and parser extensions (Phase 2)

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

## Rejected Approaches
(None yet)

## Open Questions
- ~~AST representation: `std::variant` vs class hierarchy?~~ **Decided: `std::variant`**
- ~~Should `LET` be optional in assignments?~~ **Decided: yes, optional.**
- 0-indexed vs 1-indexed arrays on GPU side? BASIC is 1-indexed but HIP is 0-indexed.
  Kernel codegen will need to handle the translation.
- ~~Executable name?~~ **Decided: `rocBAS`**

## Last Verified
Commit: 328154e
Date: 2026-03-09
