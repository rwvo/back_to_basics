# rocBAS

A classic BASIC interpreter with AMD GPU compute extensions and MPI support.
Write line-numbered BASIC like it's 1978, then launch GPU kernels and distribute
work across nodes like it's 2026.

```basic
10 REM Vector addition on the GPU
20 DIM A(1024), B(1024), C(1024)
30 FOR I = 1 TO 1024
40   A(I) = I
50   B(I) = I * 2
60 NEXT I
70 GPU DIM GA(1024), GB(1024), GC(1024)
80 GPU COPY A TO GA
90 GPU COPY B TO GB
100 GPU KERNEL VECADD(X, Y, Z, N)
110   LET I = BLOCK_IDX(1) * BLOCK_DIM(1) + THREAD_IDX(1)
120   IF I < N THEN LET Z(I) = X(I) + Y(I)
130 END KERNEL
140 GPU GOSUB VECADD(GA, GB, GC, 1024) WITH 4 BLOCKS OF 256
150 GPU COPY GC TO C
160 PRINT "C(1) = "; C(1)
170 PRINT "C(512) = "; C(512)
180 GPU FREE GA
190 GPU FREE GB
200 GPU FREE GC
210 END
```

## Building

Requires a C++17 compiler and CMake 3.14+.

**With GPU and MPI support** (requires [ROCm](https://rocm.docs.amd.com/) and
an MPI implementation such as [OpenMPI](https://www.open-mpi.org/)):

```sh
cmake -B build -DCMAKE_PREFIX_PATH=/opt/rocm
cmake --build build
```

Both HIP and MPI are auto-detected. To disable either:

```sh
cmake -B build -DROCBAS_ENABLE_HIP=OFF   # CPU-only, no GPU
cmake -B build -DROCBAS_ENABLE_MPI=OFF   # no MPI
```

**CPU-only** (no ROCm or MPI needed):

```sh
cmake -B build -DROCBAS_ENABLE_HIP=OFF -DROCBAS_ENABLE_MPI=OFF
cmake --build build
```

## Running

```sh
# Run a program from a file
./build/rocBAS examples/vecadd.bas

# Interactive REPL
./build/rocBAS
```

The REPL supports the classic workflow: enter numbered lines, then `RUN` to
execute, `LIST` to display the program, `NEW` to clear, and `QUIT` to exit.
Use `SAVE` and `LOAD` to store and retrieve programs from disk (C64-style).

| Command | Description |
|---------|-------------|
| `RUN` | Execute the current program |
| `LIST` | Display all program lines |
| `NEW` | Clear the program |
| `SAVE "file.bas"` | Save the program to a file |
| `LOAD "file.bas"` | Load a program from a file (clears current program) |
| `QUIT` / `EXIT` | Exit the REPL |

```
> 10 PRINT "Hello, World!"
> 20 END
> SAVE "hello.bas"
Program saved to hello.bas
> NEW
Program cleared.
> LOAD "hello.bas"
Program loaded from hello.bas
> RUN
Hello, World!
>
```

## Tests

```sh
ctest --test-dir build --output-on-failure
```

## Language Reference

### Statements

| Statement | Description |
|-----------|-------------|
| `LET var = expr` | Assignment (LET is optional) |
| `PRINT expr [; expr ...]` | Output; `;` suppresses newline between items |
| `INPUT ["prompt";] var` | Read from stdin |
| `GOTO line` | Unconditional jump |
| `GOSUB line` / `RETURN` | Subroutine call and return |
| `IF expr THEN stmt` | Conditional (single-line) |
| `IF expr THEN line` | Conditional GOTO |
| `FOR var = expr TO expr [STEP expr]` | Loop start |
| `NEXT var` | Loop end |
| `DIM var(size [, size ...])` | Declare array |
| `REM text` | Comment |
| `END` | Halt execution |
| `OPTION BASE N` | Set host array base index (0, 0.5, or 1; default 1) |
| `OPTION GPU BASE N` | Set GPU array base index (0, 0.5, or 1; default 0) |

### Expressions

**Arithmetic**: `+`, `-`, `*`, `/`, `^` (exponentiation)

**Comparison**: `=`, `<>`, `<`, `>`, `<=`, `>=`

**Logical**: `AND`, `OR`, `NOT`

**String concatenation**: `+`

### Variables

- Undecorated names are numeric (float64): `X`, `COUNT`, `I`
- Names ending in `$` are strings: `N$`, `LINE$`
- Undefined numeric variables default to `0`; strings default to `""`

### Built-in Functions

| Function | Description |
|----------|-------------|
| `ABS(x)` | Absolute value |
| `INT(x)` | Floor |
| `SQR(x)` | Square root |
| `SIN(x)`, `COS(x)`, `TAN(x)` | Trigonometry |
| `RND(x)` | Random number 0..1 (argument ignored) |
| `LEN(s$)` | String length |
| `LEFT$(s$, n)` | Left substring |
| `RIGHT$(s$, n)` | Right substring |
| `MID$(s$, start [, len])` | Middle substring (1-indexed) |
| `CHR$(n)` | Character from ASCII code |
| `ASC(s$)` | ASCII code of first character |
| `VAL(s$)` | Parse string to number |
| `STR$(n)` | Number to string |
| `TAB(n)` | N spaces (for print formatting) |

### Arrays and OPTION BASE

By default, host arrays are 1-indexed (classic BASIC convention):

```basic
10 DIM A(5)
20 LET A(1) = 10
30 LET A(5) = 50
40 PRINT A(1); A(5)
50 END
```
Output: `1050`

Use `OPTION BASE 0` for 0-indexed host arrays:

```basic
5 OPTION BASE 0
10 DIM A(5)
20 LET A(0) = 10
30 LET A(4) = 40
40 PRINT A(0); A(4)
50 END
```
Output: `1040`

Use `OPTION BASE 0.5` for half-based indexing — a tongue-in-cheek mode where
the first element is `A(0.5)`, the second is `A(1.5)`, and so on:

```basic
5 OPTION BASE 0.5
10 DIM A(5)
20 LET A(0.5) = 10
30 LET A(1.5) = 20
40 LET A(4.5) = 50
50 PRINT A(0.5); A(1.5); A(4.5)
60 END
```
Output: `102050`

This works in loops too — just add 0.5 to your loop variable:

```basic
5 OPTION BASE 0.5
10 DIM A(5)
20 FOR I = 0 TO 4
30   LET A(I + 0.5) = (I + 1) * 10
40 NEXT I
50 PRINT A(0.5); A(2.5); A(4.5)
60 END
```
Output: `103050`

Multi-dimensional arrays are also supported:

```basic
10 DIM GRID(3, 3)
20 LET GRID(2, 3) = 42
30 PRINT GRID(2, 3)
40 END
```

## GPU Extensions

rocBAS extends classic BASIC with statements for GPU programming via AMD ROCm/HIP.
GPU operations are explicit: you allocate device memory, copy data, define kernels,
launch them, and copy results back.

### GPU Statements

| Statement | Description |
|-----------|-------------|
| `GPU DIM var(size)` | Allocate device memory (1D only) |
| `GPU COPY src TO dst` | Copy data between host and device |
| `GPU KERNEL name(params) ... END KERNEL` | Define a kernel |
| `GPU GOSUB name(args) WITH g BLOCKS OF b` | Launch kernel (1D grid) |
| `GPU GOSUB name(args) WITH (gx,gy,gz) BLOCKS OF (bx,by,bz)` | Launch kernel (3D grid) |
| `GPU FREE var` | Free device memory |

Direction of `GPU COPY` is inferred from the arguments: if the source is a host
array, data goes host-to-device; if the source is a GPU array, data goes
device-to-host.

### Kernel Intrinsics

Inside `GPU KERNEL` bodies, these intrinsics map directly to HIP:

| Intrinsic | HIP Equivalent | Description |
|-----------|---------------|-------------|
| `THREAD_IDX(1)` | `threadIdx.x` | Thread index within block |
| `THREAD_IDX(2)` | `threadIdx.y` | |
| `THREAD_IDX(3)` | `threadIdx.z` | |
| `BLOCK_IDX(d)` | `blockIdx.{x,y,z}` | Block index within grid |
| `BLOCK_DIM(d)` | `blockDim.{x,y,z}` | Threads per block |
| `GRID_DIM(d)` | `gridDim.{x,y,z}` | Blocks in grid |

Compute a global thread ID the same way you would in HIP:

```basic
LET I = BLOCK_IDX(1) * BLOCK_DIM(1) + THREAD_IDX(1)
```

### Kernel Body

Kernel bodies support a subset of BASIC:

- `LET` assignments (scalars and 1D array elements)
- `IF ... THEN` conditionals
- `PRINT` (emits `printf` on the GPU; useful for debugging)
- Arithmetic, comparison, and logical operators
- Math functions: `ABS`, `INT`, `SQR`, `SIN`, `COS`, `TAN`

Not supported inside kernels: `INPUT`, `GOTO`, `GOSUB`, `FOR/NEXT`, string
operations, or multi-dimensional arrays.

### GPU Array Indexing and OPTION GPU BASE

By default, GPU arrays are 0-indexed, matching HIP conventions. `THREAD_IDX`
and `BLOCK_IDX` start at 0, so array access like `A(I)` where
`I = THREAD_IDX(1)` naturally maps to `A[I]` in the generated HIP code.

If you prefer 1-indexed GPU kernels (to match host-side BASIC), use
`OPTION GPU BASE 1`. This shifts both intrinsics and array accesses:
`THREAD_IDX` and `BLOCK_IDX` start at 1, and array indices are offset
accordingly. `BLOCK_DIM` and `GRID_DIM` are unaffected (they are sizes,
not indices).

```basic
5 OPTION GPU BASE 1
10 GPU KERNEL FILL(A, N)
20   LET I = THREAD_IDX(1)
30   IF I <= N THEN LET A(I) = I * 10
40 END KERNEL
```

The multi-block global ID formula changes slightly with base 1:

```basic
REM GPU BASE 0 (default):  I = BLOCK_IDX(1) * BLOCK_DIM(1) + THREAD_IDX(1)
REM GPU BASE 1:            I = (BLOCK_IDX(1) - 1) * BLOCK_DIM(1) + THREAD_IDX(1)
```

`OPTION GPU BASE 0.5` is also supported — `THREAD_IDX` and `BLOCK_IDX`
are offset by 0.5, and array indices subtract 0.5 before indexing into
device memory.

### Execution Model

- **Synchronous**: `GPU GOSUB` blocks until the kernel completes, just like
  CPU `GOSUB`. When it returns, the work is done.
- **No streams**: Everything runs on the default stream, serialized.
- **Explicit memory**: Host and device memory are separate. Use `GPU COPY`
  to transfer data.

### Full GPU Example

```basic
10 REM Scale an array by a constant on the GPU
20 LET N = 512
30 DIM A(512)
40 FOR I = 1 TO N
50   A(I) = I
60 NEXT I
70 GPU DIM GA(512)
80 GPU COPY A TO GA
90 GPU KERNEL SCALE(A, N, FACTOR)
100   LET I = BLOCK_IDX(1) * BLOCK_DIM(1) + THREAD_IDX(1)
110   IF I < N THEN LET A(I) = A(I) * FACTOR
120 END KERNEL
130 GPU GOSUB SCALE(GA, 512, 3.0) WITH 2 BLOCKS OF 256
140 GPU COPY GA TO A
150 PRINT "A(1) = "; A(1)
160 PRINT "A(256) = "; A(256)
170 PRINT "A(512) = "; A(512)
180 GPU FREE GA
190 END
```

## MPI Extensions

rocBAS supports MPI (Message Passing Interface) for distributing work across
multiple processes. Each rank runs the same BASIC program independently, with
explicit communication via send/receive.

### Building with MPI

MPI is auto-detected by CMake when an MPI implementation (OpenMPI, MPICH, etc.)
is installed. No extra flags are needed beyond what is shown in the Building
section above.

### Running MPI Programs

```sh
mpirun -n 4 ./build/rocBAS examples/heat.bas
```

Programs also work without `mpirun` — they run as a single process with
`MPI RANK` = 0 and `MPI SIZE` = 1.

### MPI Statements

| Statement | Description |
|-----------|-------------|
| `MPI INIT` | Initialize MPI (must be called before any other MPI statement) |
| `MPI FINALIZE` | Shut down MPI (call once, at program end) |
| `MPI SEND A TO rank TAG tag` | Send entire array A to a destination rank |
| `MPI SEND A(lo) THRU A(hi) TO rank TAG tag` | Send a range of elements |
| `MPI RECV A FROM rank TAG tag` | Receive into entire array A |
| `MPI RECV A(lo) THRU A(hi) FROM rank TAG tag` | Receive into a range of elements |
| `MPI BARRIER` | Block until all ranks reach this point |

### MPI Expressions

| Expression | Description |
|------------|-------------|
| `MPI RANK` | This process's rank (0 to size-1) |
| `MPI SIZE` | Total number of processes |

These are expressions, not function calls — use them directly in assignments or
comparisons:

```basic
10 LET R = MPI RANK
20 LET S = MPI SIZE
30 IF R = 0 THEN PRINT "I am the root rank out of "; S
```

### Communication Details

- **Blocking**: All sends and receives are blocking (synchronous).
- **Numeric only**: MPI transfers arrays of doubles. String data cannot be sent.
- **Tags**: Each send/receive pair must use matching tags. Tags are arbitrary
  integers you choose to distinguish different messages.
- **THRU syntax**: `MPI SEND A(lo) THRU A(hi)` sends elements `lo` through `hi`
  (inclusive) from array A. The receive side must specify a matching range size.

### Full MPI + GPU Example: Heat Diffusion

This example simulates 2D heat diffusion on a 32x32 grid, distributed across
4 MPI ranks. A hot wall (T=100) at the top and a cold wall (T=0) at the bottom
drive heat flow downward. Each rank owns 8 rows and uses a GPU kernel for the
stencil computation.

**Domain decomposition**: The grid is split into horizontal strips (1D
decomposition along rows). Each rank's local domain includes two ghost rows —
one above and one below — that hold copies of the neighboring rank's boundary
data.

**Per-timestep flow**:
1. GPU kernel computes the 5-point stencil on interior points
2. `GPU COPY` brings results to host memory
3. `MPI SEND/RECV` exchanges ghost rows between neighbors
4. `GPU COPY` uploads the corrected state back to the device

```basic
5 OPTION GPU BASE 0
10 REM === 2D Heat Diffusion with GPU and MPI ===
20 REM Hot top wall (T=100), cold bottom wall (T=0)
30 REM 32x32 grid, 4 ranks, 1D row decomposition
40 REM Run: mpirun -n 4 rocBAS examples/heat.bas
100 MPI INIT
110 LET R = MPI RANK
120 LET S = MPI SIZE
140 LET NX = 32
150 LET NY = 32
160 LET LR = NX / S
170 LET LROWS = LR + 2
180 LET SZ = LROWS * NY
190 LET NT = 1000
195 LET ALPHA = 0.1
200 REM --- Row index bounds (host 1-based) ---
210 LET B1 = NY + 1
220 LET E1 = 2 * NY
230 LET BL = LR * NY + 1
240 LET EL = (LR + 1) * NY
250 LET BG = (LR + 1) * NY + 1
260 LET EG = (LR + 2) * NY
310 DIM U(SZ), U2(SZ)
410 FOR I = 1 TO SZ
420 LET U(I) = 0
430 NEXT I
450 IF R > 0 THEN GOTO 490
460 FOR J = 1 TO NY
470 LET U(J) = 100
480 NEXT J
490 REM --- GPU setup ---
500 GPU DIM GU(SZ), GU2(SZ)
510 GPU COPY U TO GU
600 REM --- Heat stencil kernel (1D arrays, stride = NY) ---
610 GPU KERNEL HEAT(UOLD, UNEW, W, H, COEFF, N)
620 LET I = BLOCK_IDX(1) * BLOCK_DIM(1) + THREAD_IDX(1)
630 LET ROW = INT(I / W)
640 LET COL = I - ROW * W
650 IF I < N THEN IF ROW > 0 THEN IF ROW < H - 1 THEN IF COL > 0 THEN IF COL < W - 1 THEN LET UNEW(I) = UOLD(I) + COEFF * (UOLD(I - W) + UOLD(I + W) + UOLD(I - 1) + UOLD(I + 1) - 4 * UOLD(I))
660 END KERNEL
710 FOR T = 1 TO NT
730 GPU GOSUB HEAT(GU, GU2, NY, LROWS, ALPHA, SZ) WITH 2 BLOCKS OF 256
750 GPU COPY GU2 TO U2
770 IF R >= S - 1 THEN GOTO 790
780 MPI SEND U2(BL) THRU U2(EL) TO R + 1 TAG 1
790 IF R <= 0 THEN GOTO 820
810 MPI RECV U2(1) THRU U2(NY) FROM R - 1 TAG 1
820 IF R <= 0 THEN GOTO 850
840 MPI SEND U2(B1) THRU U2(E1) TO R - 1 TAG 2
850 IF R >= S - 1 THEN GOTO 880
870 MPI RECV U2(BG) THRU U2(EG) FROM R + 1 TAG 2
880 MPI BARRIER
900 IF R > 0 THEN GOTO 940
910 FOR J = 1 TO NY
920 LET U2(J) = 100
930 NEXT J
940 GPU COPY U2 TO GU
960 NEXT T
1010 GPU COPY GU TO U
1020 MPI BARRIER
1030 IF R > 0 THEN GOTO 1060
1040 PRINT "=== 2D Heat Diffusion ("; NX; "x"; NY; ", "; NT; " steps) ==="
1060 MPI BARRIER
1070 LET PR = 0
1080 IF R <> PR THEN GOTO 1110
1090 PRINT "Rank "; R; ": top="; U(NY + INT(NY / 2)); " bot="; U(LR * NY + INT(NY / 2))
1110 MPI BARRIER
1120 LET PR = PR + 1
1130 IF PR < S THEN GOTO 1080
1210 GPU FREE GU
1220 GPU FREE GU2
1230 MPI FINALIZE
1240 END
```

Output (after 1000 timesteps):

```
=== 2D Heat Diffusion (32x32, 1000 steps) ===
Rank 0: top=92.9296 bot=48.8183
Rank 1: top=43.795 bot=18.5433
Rank 2: top=16.174 bot=5.51292
Rank 3: top=4.61155 bot=0.447149
```

Temperature decreases monotonically from the hot wall (rank 0) to the cold wall
(rank 3), confirming that heat is diffusing correctly across MPI rank boundaries.

The GPU kernel uses 1D flat arrays with a stride argument (`W = NY`) for
row-major 2D indexing — the same pattern used in real HIP/CUDA code. `INT(I / W)`
extracts the row index, `I - ROW * W` extracts the column.

## Examples

The `examples/` directory contains ready-to-run programs:

| File | Description |
|------|-------------|
| `hello.bas` | Hello, World! |
| `fibonacci.bas` | First 20 Fibonacci numbers |
| `guess.bas` | Number guessing game |
| `vecadd.bas` | GPU vector addition (requires ROCm) |
| `heat.bas` | 2D heat diffusion with GPU + MPI (requires ROCm and MPI) |

```sh
./build/rocBAS examples/fibonacci.bas
```

## Project Structure

```
src/
  main.cpp              Entry point and REPL
  token.h               Token types
  lexer.h / lexer.cpp   Tokenizer
  parser.h / parser.cpp Recursive descent parser
  ast.h                 AST node definitions (std::variant)
  interpreter.h / .cpp  Tree-walk interpreter
  environment.h / .cpp  Variable and array storage
  gpu_codegen.h / .cpp  BASIC kernel AST -> HIP C++ source
  gpu_runtime.h / .cpp  HIP device management and kernel launch
  mpi_runtime.h / .cpp  MPI init/finalize, send/recv, barrier
tests/                  GTest unit and integration tests
examples/               Example .bas programs
```

## License

See [LICENSE](LICENSE) for details.
