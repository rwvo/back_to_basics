# rocBAS

A classic BASIC interpreter with AMD GPU compute extensions.
Write line-numbered BASIC like it's 1978, then launch GPU kernels like it's 2026.

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

**With GPU support** (requires [ROCm](https://rocm.docs.amd.com/)):

```sh
cmake -B build -DCMAKE_PREFIX_PATH=/opt/rocm
cmake --build build
```

**CPU-only** (no ROCm needed):

```sh
cmake -B build -DROCBAS_ENABLE_HIP=OFF
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

```
rocBAS v0.1 - BASIC interpreter with GPU extensions
Type BASIC lines with line numbers. Type RUN to execute.
Type LIST to show program. Type NEW to clear. Type QUIT to exit.

> 10 PRINT "Hello, World!"
> 20 END
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
| `OPTION BASE N` | Set host array base index (0 or 1; default 1) |
| `OPTION GPU BASE N` | Set GPU array base index (0 or 1; default 0) |

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

By default, GPU arrays are 0-indexed, matching HIP conventions. Thread indices
start at 0, so array access like `A(I)` where `I = THREAD_IDX(1)` naturally
maps to `A[I]` in the generated HIP code.

If you prefer 1-indexed GPU arrays (to match host-side BASIC), use
`OPTION GPU BASE 1`. The codegen will adjust array accesses to subtract 1:

```basic
5 OPTION GPU BASE 1
10 GPU KERNEL FILL(A, N)
20   LET I = BLOCK_IDX(1) * BLOCK_DIM(1) + THREAD_IDX(1) + 1
30   IF I <= N THEN LET A(I) = I * 10
40 END KERNEL
```

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

## Examples

The `examples/` directory contains ready-to-run programs:

| File | Description |
|------|-------------|
| `hello.bas` | Hello, World! |
| `fibonacci.bas` | First 20 Fibonacci numbers |
| `guess.bas` | Number guessing game |
| `vecadd.bas` | GPU vector addition (requires ROCm) |

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
tests/                  GTest unit and integration tests
examples/               Example .bas programs
```

## License

See [LICENSE](LICENSE) for details.
