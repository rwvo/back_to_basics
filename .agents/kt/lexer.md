# Lexer

## Responsibility
Tokenizes BASIC source code into a flat token stream. Handles line numbers, keywords,
identifiers (including `$`-suffixed string identifiers), literals, operators, and delimiters.

## Core Concepts
- **Token**: A typed lexeme with a BASIC line number and source line number for error reporting.
- **TokenType**: Enum covering literals, identifiers, operators, keywords (CPU + GPU), and special tokens.
- **String identifier**: Identifiers ending in `$` (e.g., `N$`) are emitted as `STRING_IDENTIFIER`.

## Key Invariants
- Each input line must start with an integer line number.
- Keywords are case-insensitive (source is uppercased during scanning).
- GPU intrinsics (`THREAD_IDX`, `BLOCK_IDX`, `BLOCK_DIM`, `GRID_DIM`) are lexed as keywords, not identifiers.
- `NEWLINE` tokens separate logical lines; `END_OF_FILE` terminates the stream.

## Data Flow
```
std::string source → Lexer::tokenize() → std::vector<Token>
```

## Interfaces
- `Lexer(const std::string& source)` — constructor — `src/lexer.h:12`
- `std::vector<Token> tokenize()` — main entry point — `src/lexer.h:13`
- `TokenType` enum — `src/token.h:8`
- `Token` struct — `src/token.h:84`

## Dependencies
- None (self-contained, no other subsystem dependencies).

## Also Load
- [Parser](parser.md) — immediate downstream consumer of the token stream.

## Known Limitations
- No support for multi-line string literals.
- No KW_THRU / MPI tokens yet (v3 pending).

## Last Verified
Commit: a315030
Date: 2026-04-01
