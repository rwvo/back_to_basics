---
name: kt-load
description: Load Knowledge Tree context at session start. Use when starting work on the codebase.
disable-model-invocation: false
allowed-tools: Read, Glob, Grep
---

# Knowledge Tree Load

Rehydrate context from the Knowledge Tree at session start.

## Instructions

Follow these steps as specified in `.agents/kt_workflows.md`:

### 1. Always Load Architecture

Read `.agents/kt/architecture.md` first. This provides the system overview.

### 2. Determine Session Scope

Check if the user specified what they're working on:
- If they mentioned a specific subsystem/area: load that dossier
- If they mentioned a task/goal: infer relevant subsystems and load those dossiers
- If unclear: ask the user which area they're working on

Arguments to this command (if provided) indicate the work area. For example:
- `kt-load memory_analysis` → load memory_analysis.md
- `kt-load for dh_comms bugfix` → load dh_comms integration dossier and sub-project KT

### 3. Load Relevant Dossiers

Based on the scope, read the appropriate dossier files from `.agents/kt/`:
- Subsystem dossiers: `<subsystem>.md`
- Sub-project integration: `sub_<name>.md`
- If a dossier has an "Also Load" section, load those dossiers too

### 4. Load Sub-project KTs (if relevant)

If working on a sub-project (dh_comms, kerneldb, instrument-amdgpu-kernels):
- Load the sub-project's `.agents/kt/architecture.md`
- Load relevant sub-project dossiers based on task scope
- The top-level integration dossier tells you which sub-project KT to load

### 5. Note Session Intent

Remember what subsystems are in scope and what kind of work this is (feature, bugfix, refactor, exploration). This will be useful for kt-update later.

### 6. Acknowledge

Briefly confirm to the user what context you've loaded and that you're ready to proceed.

## Example

User: `kt-load, working on memory analysis`

You should:
1. Read `.agents/kt/architecture.md`
2. Read `.agents/kt/memory_analysis.md`
3. Check if memory_analysis.md has an "Also Load" section and load those
4. Confirm: "Loaded architecture + memory_analysis context. Ready to work."

## Notes

- Load only what's needed for the current task (scoped loading)
- Don't read the entire KT at session start
- Source code is truth - if KT conflicts with code, the code wins
