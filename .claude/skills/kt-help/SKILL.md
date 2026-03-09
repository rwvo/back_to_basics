---
name: kt-help
description: Show available Knowledge Tree commands
disable-model-invocation: false
---

# Knowledge Tree Help

Display available Knowledge Tree commands and quick reference.

## Instructions

Show the user this help information:

```
Session & Knowledge Tree Commands:

  session-init  Prime permissions + load KT (recommended session start)

  kt-help       Show this help
  kt-init       Create KT structure for new project
  kt-load       Load context at session start
  kt-update     Persist learnings at session end
  kt-validate   Check for stale dossiers
  kt-reflect    Assess KT granularity for current task

  kt-refactor start <goal>   Begin new refactor
  kt-refactor suspend        Checkpoint current refactor
  kt-refactor resume [slug]  Continue existing refactor
  kt-refactor finish         Finalize completed refactor
  kt-refactor list           Show all refactors

See .agents/kt_usage.md for quick reference.
See .agents/kt_workflows.md for detailed specifications.
```
