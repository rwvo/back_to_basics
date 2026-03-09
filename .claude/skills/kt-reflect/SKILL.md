---
name: kt-reflect
description: Assess whether KT granularity was appropriate for the current task. Use mid-session or end-of-session to evaluate KT effectiveness.
disable-model-invocation: false
allowed-tools: Read, Glob, Grep
---

# Knowledge Tree Reflection

Self-assess whether Knowledge Tree granularity was appropriate for the current session's work.

## Instructions

Follow these steps as specified in `.agents/kt_workflows.md`:

### Reflection Questions

Answer these questions internally by analyzing your own work during the session:

1. **Coverage gaps** — Did I read source files that weren't covered by any dossier?
   - If yes → consider adding a dossier or expanding existing one

2. **Unused loads** — Did I load dossiers that I never actually referenced?
   - If yes → maybe those dossiers are at wrong granularity or "Also Load" is too aggressive

3. **Insufficient depth** — Did a dossier exist but lack detail I needed?
   - If yes → consider expanding that dossier's Interfaces or adding subsections

4. **Excessive detail** — Did a dossier contain information that was noise for this task?
   - If yes → consider splitting task-specific details into separate dossier
   - Note: This is context-dependent; detail useful for one task may be noise for another

5. **Missing connections** — Did I discover dependencies between subsystems not documented in "Also Load" or architecture?
   - If yes → update cross-references

### Output Format

Present findings to the user like this:

```
KT Granularity Reflection for this session:

Task type: [bugfix | feature | refactor | exploration]
Subsystems touched: [list]

Observations:
  - Coverage gap: src/config_receiver.cc not covered by any dossier
  - Insufficient depth: memory_analysis.md lacks detail on conflict_set internals
  - Good fit: interceptor.md provided exactly what was needed

Suggestions:
  - Consider adding dossier: config_receiver.md (or fold into existing)
  - Consider expanding: memory_analysis.md → add conflict_set section
  - No changes needed: interceptor.md, comms_mgr.md
```

### Granularity Principles (Task-Relative)

| Task Type | Ideal Granularity |
|-----------|-------------------|
| High-level feature | Coarse: architecture + integration dossiers |
| Subsystem refactor | Medium: subsystem dossier + key interfaces |
| Specific bugfix | Fine: detailed interfaces, maybe per-class |
| Exploration | Coarse initially, refine as focus narrows |

### Action

Reflection informs `kt-update`. Don't restructure KT mid-session; note suggestions and apply during kt-update.

## Notes

- This is a self-assessment tool for the AI agent
- Use when KT feels too coarse or too detailed
- Results inform what changes to make during kt-update
