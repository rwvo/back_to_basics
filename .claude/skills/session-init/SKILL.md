---
name: session-init
description: Prime session permissions and load KT context. Run at the start of each session.
disable-model-invocation: false
allowed-tools: Read, Write, Edit, Glob, Grep, Bash, Skill
---

# Session Init

Prime session-scoped permissions and load Knowledge Tree context in one step.

## Options

Arguments starting with `-` are options; everything else is passed to `kt-load` as the work
area. Options can appear in any order before the KT area.

| Option | Effect |
|--------|--------|
| `-su` | Super-user mode: skip permission priming, read & acknowledge guardrails instead |
| `-no-kt` | Skip KT loading entirely (no `kt-load` invocation, no prompts about KT) |

### Examples

| Invocation | Permissions | Guardrails | KT |
|---|---|---|---|
| `/session-init` | Prime | Skip | Load (asks user for area) |
| `/session-init memory_analysis` | Prime | Skip | Load `memory_analysis` |
| `/session-init -su` | Skip | Read & confirm | Load (asks user for area) |
| `/session-init -su memory_analysis` | Skip | Read & confirm | Load `memory_analysis` |
| `/session-init -no-kt` | Prime | Skip | Skip |
| `/session-init -su -no-kt` | Skip | Read & confirm | Skip |

## Instructions

Follow these steps in order:

### 1. Parse Arguments

Separate options from positional args:
- `-su` → enable super-user mode
- `-no-kt` → disable KT loading
- Everything else → KT area (passed through to `kt-load`)

### 2a. Normal Mode (no `-su`): Prime Session Permissions

Read `.claude/session_init_primes.json`. For each entry, execute the priming action:

- **write-and-delete**: Write a trivial temp file to the `path`, then immediately delete it
  with `rm`. This triggers the "allow during this session" prompt. Example:
  ```
  Write "session-prime" to the path, then rm it.
  ```
- **edit-roundtrip**: Read the file at `path`, then Edit it by replacing its first line with
  itself (a no-op edit). This triggers session-scoped permission prompts that only fire on
  Edit (not Write). Example:
  ```
  Read the file, then Edit old_string=first_line new_string=first_line.
  ```
- **env**: Set an environment variable for the session. The entry has `name` and `value` fields.
  Remember these variables and prefix all subsequent Bash commands with them (e.g.,
  `TRITON_REPO=/path/to/triton your_command`). Report the env vars to the user in the
  confirmation step.

Tell the user: "Approve with 'Yes, allow all edits during this session' to unlock writes for
this session."

After priming, confirm which permissions were unlocked.

### 2b. Super-user Mode (`-su`): Load Guardrails

Skip all permission priming. Instead:

1. Read `.claude/guardrails.md`
2. Internalize the rules and restrictions in that file
3. Explicitly confirm to the user that you have read and will abide by every rule and
   restriction listed in the guardrails file. Summarize the key constraints so the user can
   verify you understood them.

### 3. Load Knowledge Tree (unless `-no-kt`)

If `-no-kt` was specified, skip this step entirely — do not invoke `kt-load` and do not ask
the user about KT initialization.

Otherwise, invoke the `kt-load` skill, passing through the KT area arguments (if any).

For example:
- `/session-init` → call `kt-load` with no args
- `/session-init memory_analysis` → call `kt-load memory_analysis`
- `/session-init -su` → call `kt-load` with no args
- `/session-init -su memory_analysis` → call `kt-load memory_analysis`
- `/session-init -no-kt` → skip `kt-load`
- `/session-init -su -no-kt` → skip `kt-load`

### 4. Confirm

Tell the user the session is initialized:
- **Normal mode**: Which permissions were primed + KT context loaded (or "KT skipped")
- **Super-user mode**: Guardrails acknowledged + KT context loaded (or "KT skipped")

## Adding New Primes

**Important: session-scoped vs permanent permissions**

Not all permission prompts belong in session-init. Only add **session-scoped** prompts:

- **Session-scoped** (add to primes): Prompts with "Yes, allow _during this session_" — these
  reset every session and need re-priming. Example: "allow all edits during this session."
- **Permanent** (do NOT add): Prompts with "Yes, and _always_ allow..." — these persist across
  sessions via `settings.local.json` and never need priming. If the user sees one of these,
  recommend they choose the permanent option instead.

When the user encounters a session-scoped prompt during normal work and says "add that to
session-init", add a new entry to `.claude/session_init_primes.json`. Entry format:

```json
{
  "tool": "<tool name>",
  "action": "<action type>",
  "path": "<path if relevant>",
  "note": "<what this unlocks>"
}
```

Action types:
- `write-and-delete` — write a temp file then delete it
- `edit-roundtrip` — read a file then do a no-op Edit (triggers Edit-specific permission prompts)
- `read` — read a file at the path
- `glob` — glob a pattern at the path
- `bash` — run a trivial command (stored in `command` field)
- `env` — set an environment variable (`name` and `value` fields); remember it for all subsequent Bash calls
