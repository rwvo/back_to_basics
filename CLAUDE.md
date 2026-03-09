# Back to Basics - Claude Code Instructions

## Knowledge Tree
This project uses a knowledge tree in `.agents/kt/` for structured project understanding.

**New to the KT?** Start with `.agents/kt_overview.md`
**Quick reference**: `.agents/kt_usage.md`
**Full specifications**: `.agents/kt_workflows.md`

**Session workflow**:
- **Start**: `/session-init` — prime permissions and load KT context
- **During**: Just code. Use `/kt-reflect` if KT feels too coarse or too detailed.
- **End**: `/kt-update` — persist learnings to KT

**Refactoring**: Use `/kt-refactor start|suspend|resume|finish|list` for multi-session refactors.

If the knowledge tree doesn't exist yet, run `/kt-init` first.
Use `/kt-validate` to check for stale dossiers after skipping updates.

## Key Conventions

- **Guardrails** are in `.claude/guardrails.md`.
- **Cost tracking**: `/update-cost-report` writes to `.untracked/session-costs.md`.
