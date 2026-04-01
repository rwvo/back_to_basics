# Parser

## Responsibility
Recursive-descent parser that consumes the token stream and produces an AST (`Program`).
Handles precedence climbing for expressions and dispatches to specialized parsers for
CPU statements, GPU statements, and OPTION BASE.

## Core Concepts
- **Program**: A vector of `StmtPtr` sorted by line number — the top-level AST.
- **Precedence climbing**: Expression parsing uses a chain: `or → and → not → comparison → addition → multiplication → exponentiation → unary → primary`.
- **Built-in functions**: Recognized by name during `parse_primary()` — dispatched as `FunctionCall` AST nodes.

## Key Invariants
- Each parsed line must begin with an `INTEGER_LITERAL` (the BASIC line number).
- `LET` is optional — bare identifier followed by `=` is treated as assignment.
- GPU statements are prefixed by `KW_GPU` and dispatched via `parse_gpu_statement()`.
- `GPU KERNEL ... END KERNEL` spans multiple BASIC lines; the parser collects body statements until `KW_END` + `KW_KERNEL`.
- `OPTION BASE` accepts 0, 0.5, or 1 (integer or float literal); validated at parse time.

## Data Flow
```
std::vector<Token> → Parser::parse() → Program (AST)
```

## Interfaces
- `Parser(const std::vector<Token>& tokens)` — constructor — `src/parser.h:12`
- `Program parse()` — main entry point — `src/parser.h:13`
- `parse_gpu_statement()` — GPU statement dispatcher — `src/parser.h:43`
- `parse_option()` — OPTION BASE parser — `src/parser.h:40`

## Dependencies
- **Lexer** — provides the token stream.
- **AST** — defines all node types.

## Also Load
- [Lexer](lexer.md) — upstream token provider.
- [AST](ast.md) — node type definitions.

## Known Limitations
- No MPI statement parsing yet (v3 step 21).
- `IF/THEN` is single-line only (no `ELSE`, no multi-line `IF` blocks).
- No `DATA`/`READ`/`RESTORE` implementation (AST nodes exist but parser support incomplete).

## Last Verified
Commit: a315030
Date: 2026-04-01
