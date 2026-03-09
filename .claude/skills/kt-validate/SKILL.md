---
name: kt-validate
description: Check Knowledge Tree for staleness and broken references. Use when you suspect the KT may be outdated.
disable-model-invocation: false
allowed-tools: Read, Edit, Glob, Grep, Bash
---

# Knowledge Tree Validation

Check if Knowledge Tree is stale relative to code, and optionally fix issues.

## Instructions

Follow these steps as specified in `.agents/kt_workflows.md`:

### 1. Check Each Dossier for Staleness

For each dossier in `.agents/kt/*.md`:
- Read the "Last Verified" date/commit
- Check modification times of files covered by the dossier
- Flag dossiers where source files changed after last verification

Use `git log` and file modification times to determine staleness.

### 2. Verify File:Line Anchors

For each reference like `path/to/file.cpp:42` in "Interfaces" sections:
- Check that the file exists
- Check that the line number is within the file's line count
- Flag broken anchors

### 3. Verify Key Symbols

For functions/classes mentioned in "Interfaces" sections:
- Use Grep to confirm the symbol still exists in the referenced file
- Flag missing symbols

### 4. Report Findings

Display a report like:
```
Stale dossiers:
  - interceptor.md: src/interceptor.cc modified 2026-02-20, Last Verified 2026-02-01
  - memory_analysis.md: 3 broken file:line anchors

Broken anchors:
  - memory_analysis.md: `src/memory_analysis_handler.cc:142` — file has 138 lines

Missing symbols:
  - comms_mgr.md: `growBufferPool()` not found in src/comms_mgr.cc
```

### 5. Offer to Fix

Prompt the user: "Found N issues. Fix them? [y/n]"

If user agrees (or if `--fix` flag was provided):
- For each stale dossier:
  - Re-read relevant source files
  - Update "Interfaces" section with corrected file:line anchors
  - Flag structural changes that need human review (add to "Open Questions")
  - Update "Last Verified" date to current date

If user declines, exit with report only.

## Arguments

- No arguments: validate all dossiers
- `<dossier-name>`: validate specific dossier (e.g., `kt-validate interceptor.md`)
- `--fix`: auto-fix without prompting

## Notes

- Cannot detect semantic drift (e.g., invariants that are no longer true but code still exists)
- Structural changes are flagged for review, not auto-resolved
- Use this after skipping kt-update or when KT feels outdated
