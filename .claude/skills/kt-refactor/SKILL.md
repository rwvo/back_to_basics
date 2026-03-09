---
name: kt-refactor
description: Manage refactor dossiers for multi-session refactoring tasks. Use with subcommands start, suspend, resume, finish, list.
disable-model-invocation: false
allowed-tools: Read, Write, Edit, Glob, Grep, Bash
argument-hint: <start|suspend|resume|finish|list> [args]
---

# Knowledge Tree Refactor Management

Manage refactor dossiers for multi-session refactoring tasks.

## Subcommands

- `kt-refactor start <goal>` — begin new refactor
- `kt-refactor suspend` — checkpoint and pause current refactor
- `kt-refactor resume <slug>` — continue existing refactor
- `kt-refactor finish` — finalize completed refactor
- `kt-refactor list` — show all refactors with status

## Instructions

Follow these steps as specified in `.agents/kt_workflows.md`:

### `kt-refactor start <goal>`

**Input**: A high-level change request from $ARGUMENTS.

**Steps**:

1. **Create refactor dossier**:
   - Generate slug from goal description (lowercase, hyphens)
   - Location: `.agents/kt/refactors/rf_<slug>.md`
   - Use the refactor dossier template (see below)

2. **Survey the code**:
   - Load relevant subsystem dossier(s) from KT
   - Read key source files to identify affected symbols
   - Map out call graph impact
   - Identify risks and hidden coupling

3. **Discuss with user** to establish contract:
   - Confirm goal interpretation
   - Ask about invariants: "Does ABI need to be preserved?" "Any performance constraints?"
   - Agree on verification gates: "What tests should pass?"
   - Clarify any ambiguities before proceeding

4. **Draft the dossier** (interactively):
   - Goal, Non-goals/Invariants, Verification gates
   - Affected symbols, Expected files, Risks
   - Micro-step plan with compile/test gates

5. **Get user approval** on the dossier

6. **Begin execution** if approved:
   - Set status to "In Progress"
   - Start with first micro-step

### `kt-refactor suspend`

**When**: End of session when refactor is not complete.

**Steps**:

1. Update dossier:
   - Mark completed steps
   - Record gates passed/failed
   - Note current state and blockers
   - Add rejected approaches discovered this session
   - Identify next step clearly

2. Add progress log entry with session date, accomplishments, discoveries, next step

3. Update "Last Verified" date

### `kt-refactor resume <slug>`

**Input**: Slug of existing refactor from $ARGUMENTS, or omit if only one active.

**Steps**:

1. Find and load `.agents/kt/refactors/rf_<slug>.md` with status "In Progress"
2. Load relevant subsystem dossiers referenced in scope
3. Review state: summarize objective, current step, recent progress
4. Confirm understanding with user
5. Continue execution from current step

**Note**: Only considers "In Progress" refactors (not "Done" or "Blocked"). Completed refactors are in `refactors/done/` and cannot be resumed.

### `kt-refactor finish`

**When**: All micro-steps done, refactor complete.

**Steps**:

1. Verify completion:
   - Confirm all micro-steps checked off
   - Run final verification gates (full build, all tests)

2. Update dossier:
   - Set status to "Done"
   - Add final progress log entry
   - Update "Last Verified"

3. Update subsystem dossiers if structure changed:
   - Update affected interfaces/symbols
   - Add any new invariants discovered
   - Remove outdated information

4. Archive to done/ subdirectory:
   - Move from `.agents/kt/refactors/` to `.agents/kt/refactors/done/`
   - Keeps active refactors list clean

### `kt-refactor list`

**Output** all refactors with status:

```
Active refactors (.agents/kt/refactors/):

  rf_extract-handler-interface
    Status: In Progress (step 3/5)
    Last active: 2026-03-01
    Next step: Remove duplicate definition from dh_comms.h

  rf_raii-buffer-management
    Status: Blocked
    Last active: 2026-02-28
    Blocker: Waiting for dh_comms API review

Completed refactors (.agents/kt/refactors/done/):

  rf_rename-dispatch-count
    Status: Done
    Completed: 2026-02-15
```

### Execution: Micro-step Discipline

During any active refactor (after start or resume):

1. Execute current micro-step (minimal change)
2. Run compile/test gate
3. Handle result:
   - If passes: mark step done, proceed
   - If fails: diagnose and fix before proceeding (don't accumulate breakage)
4. Update dossier as you go

## Refactor Dossier Template

Location: `.agents/kt/refactors/rf_<slug>.md`

```markdown
# Refactor: <Short Title>

## Status
- [ ] TODO
- [ ] In Progress
- [ ] Blocked
- [ ] Done

### Blocker (if blocked)
Describe what is blocking progress and when it can be unblocked.

## Objective
What this refactor accomplishes. Be specific.

## Refactor Contract

### Goal
Precise description of the change.

### Non-Goals / Invariants
What must NOT change:
- ABI compatibility: [yes/no/n/a]
- API compatibility: [yes/no/n/a]
- Performance constraints: [describe]
- Threading model: [describe]
- Other invariants: [list]

### Verification Gates
How to prove correctness:
- Build: `make` or `ninja`
- Tests: `ctest` or specific test commands
- Runtime: [any runtime sanity checks]

## Scope

### Affected Symbols
- `ClassName::methodName()` — what changes
- `free_function()` — what changes

### Expected Files
- `path/to/file.cpp` — [confirmed/hypothesis]
- `path/to/header.h` — [confirmed/hypothesis]

### Call Graph Impact
Which callers are affected and how.

### Risks
- Risk 1: [description and mitigation]
- Risk 2: [description and mitigation]

## Plan of Record

### Micro-steps
1. [ ] Step 1: [description] — Gate: [compile/test]
2. [ ] Step 2: [description] — Gate: [compile/test]
3. [ ] Step 3: [description] — Gate: [compile/test]

### Current Step
Step N: [description]

## Progress Log
<!-- Append updates, don't delete -->

### Session YYYY-MM-DD
- Completed: [steps]
- Gates: [passed/failed]
- Discovered: [new constraints or issues]
- Next: [next step]

## Rejected Approaches
Structural approaches that won't work, with reasons.
- **Approach**: Why it's not viable

## Open Questions
Unresolved issues blocking progress.

## Last Verified
Commit: <hash or "N/A">
Date: <YYYY-MM-DD>
```

## Notes

- Multiple concurrent refactors are allowed (use suspend/resume to switch)
- Avoid overlapping scope (same files)
- Use "Blocked" status when waiting on external factors
- Never proceed past a failing gate
