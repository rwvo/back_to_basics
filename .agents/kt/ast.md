# AST

## Responsibility
Defines all AST node types for the BASIC language using `std::variant`. This is a
header-only data definition module — no logic, just types.

## Core Concepts
- **Expression**: `std::variant` of `NumberLiteral`, `StringLiteral`, `Variable`, `ArrayAccess`, `BinaryExpr`, `UnaryExpr`, `FunctionCall`, `GpuIntrinsic`.
- **Statement**: Line-numbered `std::variant` of all statement types (LET, PRINT, IF, FOR, GPU DIM, GPU KERNEL, OPTION BASE, etc.).
- **ExprPtr / StmtPtr**: `std::unique_ptr<Expression>` / `std::unique_ptr<Statement>` — owning pointers throughout the AST.
- **Program**: `std::vector<StmtPtr>` sorted by line number.

## Key Invariants
- Every `Statement` carries a `line_number` (the BASIC line number).
- `GpuKernelStmt` contains a `std::vector<StmtPtr> body` — kernel body statements.
- `OptionBaseStmt::base` is `double` (supports 0, 0.5, 1).
- `ArrayAccess::indices` are expressions (evaluated at runtime).

## Interfaces
- All types in `src/ast.h`
- `make_expr(double)`, `make_expr(const std::string&)`, `make_var(const std::string&)` — convenience constructors — `src/ast.h:93-103`

## Dependencies
- None (header-only, standard library only).

## Also Load
- [Parser](parser.md) — constructs AST nodes.
- [Interpreter](interpreter.md) — evaluates AST nodes.

## Known Limitations
- No MPI AST nodes yet (v3 step 21).
- `DataStmt`, `ReadStmt`, `RestoreStmt` defined but not fully implemented.

## Last Verified
Commit: a315030
Date: 2026-04-01
