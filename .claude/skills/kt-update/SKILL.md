---
name: kt-update
description: Persist learnings to Knowledge Tree at session end. Use after completing work to update the KT with new knowledge.
disable-model-invocation: false
allowed-tools: Read, Edit, Write, Glob, Grep, Bash
---

# Knowledge Tree Update

Persist learnings from the current session into the Knowledge Tree.

## Instructions

Follow these steps as specified in `.agents/kt_workflows.md`:

### 1. Review What Changed

Check what was modified during this session:
- Files modified/created (use `git status` and `git diff`)
- New understanding gained
- Decisions made
- Approaches tried and rejected

### 2. Update Relevant Dossiers

For each subsystem that was worked on, update the corresponding dossier in `.agents/kt/`:

- **Add new interfaces/symbols** if significant functions/classes were added
- **Update invariants** if constraints changed
- **Add to "Rejected Approaches"** if structural dead-ends were discovered (not anecdotal failures)
- **Update "Known Limitations"** or "Open Questions"** as needed
- **Update "Last Verified"** with current commit hash and date (YYYY-MM-DD)

### 3. Update Architecture (if needed)

Update `.agents/kt/architecture.md` if:
- New subsystems were added
- Data flows changed
- System-wide invariants changed

### 4. Update Sub-project KTs (if applicable)

If work was done in a sub-project:
- Update the sub-project's own `.agents/kt/` dossiers
- Update top-level integration dossier (`sub_<name>.md`) if integration behavior changed
- Keep sub-project KT focused on internals; top-level focuses on integration

### 5. Granularity Check

- If a dossier exceeds ~300 lines, consider splitting it
- Remove stale information rather than accumulating
- Don't document micro-decisions or debugging steps

### 6. Negative Knowledge

Record structural impossibilities, not anecdotal failures:
- **Good**: "Approach X violates invariant Y because Z"
- **Bad**: "Tried X, got error, gave up"

### 7. Portability Check

**CRITICAL**: Never use machine-specific absolute paths in KT documents.
- **WRONG**: `/work1/amd/rvanoo/repos/omniprobe` or `/home/user/project`
- **CORRECT**: "repository root", `<repo-root>`, or relative paths from repo root
- Code examples should work for anyone who clones the repository
- Remember: KT documents are version-controlled and shared

### 8. Confirm

Briefly tell the user what was updated in the KT.

## Example

After a session where retry logic was added to comms_mgr:

1. Check `git diff` to see what changed
2. Edit `.agents/kt/comms_mgr.md`:
   - Add retry logic interface to Interfaces section
   - Update Last Verified date
3. Confirm: "Updated comms_mgr.md with retry logic details."

## Notes

- Update at session end, not during coding
- If unsure what changed, review git status/diff
- Use Edit tool to modify existing dossiers (preferred over rewriting)
