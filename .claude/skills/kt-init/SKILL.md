---
name: kt-init
description: Create initial Knowledge Tree structure for a project. Use when setting up KT for the first time.
disable-model-invocation: true
allowed-tools: Read, Write, Glob, Grep, Bash
---

# Knowledge Tree Initialization

Create initial Knowledge Tree for a project that doesn't have one.

## Instructions

Follow these steps as specified in `.agents/kt_workflows.md`:

### 1. Survey the Codebase

- Identify major subsystems (directories, libraries, components)
- Identify sub-projects (git submodules, external dependencies with source)
- Note primary entry points and data flows
- **Limit exploration**: max ~20 files sampled, focus on structure not details
- **Source priority**: Treat source code as ground truth. Documentation (READMEs, docs/) may be outdated; use it for hints but verify against code. Build files (CMakeLists.txt, etc.) are reliable for understanding project structure.

### 2. Create `architecture.md`

Create `.agents/kt/architecture.md` with:
- High-level system map (ASCII diagram if helpful)
- List of subsystems with one-liner descriptions
- List of sub-projects with locations and purpose
- Primary data flows
- Key invariants (system-wide constraints)
- Links to subsystem dossiers

### 3. Create Subsystem Dossiers

Create one `.agents/kt/<subsystem>.md` per major subsystem using the dossier template:
- Keep to ~100-200 lines; split if larger
- One dossier per component that would merit its own README
- For sub-projects: create a brief integration dossier at top-level (`sub_<name>.md`), defer internals to sub-project's own KT

Use this template structure:

```markdown
# <Subsystem Name>

## Responsibility
One-paragraph description of what this subsystem does.

## Core Concepts
- **Term1**: Definition
- **Term2**: Definition

## Key Invariants
- Constraint 1
- Constraint 2

## Data Flow
How data enters, transforms, and exits this subsystem.

## Interfaces
Key entry points with file:line anchors.
- `function_name()` — purpose — `path/to/file.cpp:42`
- `ClassName` — purpose — `path/to/file.h:15`

## Dependencies
Other subsystems this one interacts with.

## Also Load
Dossiers to load alongside this one for full context.

## Performance Constraints
(If relevant)

## Known Limitations
Current constraints or technical debt.

## Rejected Approaches
Structural approaches that won't work, with reasons.
- **Approach**: Why it's not viable

## Open Questions
Unresolved design questions.

## Last Verified
Commit: <hash or "N/A">
Date: <YYYY-MM-DD>
```

### 4. Initialize Sub-project KTs (optional)

If sub-projects exist:
- Run `kt-init` in each sub-project's directory, OR
- Defer until that sub-project is actively worked on

### 5. Create `glossary.md` (if needed)

Create `.agents/kt/glossary.md` if the domain has non-obvious terminology:
- Term → definition mapping
- Keep concise

### 6. Create Refactors Directory

```bash
mkdir -p .agents/kt/refactors/done
```

### Granularity Heuristics

- One dossier per subsystem, not per file
- If a component has 3+ files working together, it's a subsystem
- If you'd explain it separately to a new team member, it deserves a dossier
- Sub-projects get their own KT; top-level describes integration only

### Portability Requirements

**CRITICAL**: Never use machine-specific absolute paths in KT documents.
- Use generic references: "repository root", `<repo-root>`, or relative paths from repo root
- Code examples and commands must work for anyone cloning the repository
- Remember: KT is version-controlled and shared with all contributors

## Notes

- This is typically run once per project
- Focus on structure, not exhaustive detail
- Can be refined over time with kt-update
