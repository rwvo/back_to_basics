---
name: update-cost-report
description: Generate or update the session cost report with AI-powered summaries.
disable-model-invocation: false
allowed-tools: Read, Write, Edit, Bash, Glob, Grep
---

# Update Cost Report

Generate or update the session cost report with AI-powered summaries.

## Instructions

This skill produces `.untracked/session-costs.md` — an incremental report of Claude Code
session costs and summaries for this project.

### Prerequisites

- `.untracked/cost.txt` — output of the `/cost` command (pricing data)
- `scripts/session-costs.py` — extraction script

### Steps

#### 1. Read the existing report (if any)

Read `.untracked/session-costs.md`. Extract the list of session IDs already present in
the report. These are in the `<!-- session:UUID -->` HTML comments before each session entry.

If no report exists, start fresh.

#### 2. Extract new sessions

Run the Python extraction script to get JSON data for sessions not yet in the report:

```bash
python3 scripts/session-costs.py --extract --skip-ids <comma-separated-existing-ids>
```

This outputs a JSON array of session objects, each containing:
- `session_id`, `start`, `end`, `duration_seconds`, `turn_duration_seconds`, `total_cost`, `model_costs`
- `input_tokens`, `output_tokens`, `cache_read_tokens`, `cache_write_tokens`
- `user_messages` — cleaned excerpts of what the user asked
- `assistant_texts` — cleaned excerpts of assistant responses
- `commit_messages` — git commits made during the session
- `files_modified` — files written/edited
- `refactor_dossiers` — `rf_*.md` files referenced
- `skill_invocations` — skills used (e.g., `kt-update`, `kt-refactor`)
- `first_user_msg` — short version of the first user message
- `git_branch` — branch worked on

#### 3. Identify the active session

The current (active) session will be the one whose `session_id` matches the current
JSONL file being written to. It may also be identifiable as the session with the latest
`end` timestamp that is very recent (within the last few minutes). Flag it in the report
with "(active — costs incomplete)".

#### 4. Generate summaries

For each new session, compose a **1-2 sentence summary** of what was accomplished, based
on the conversation excerpts. Focus on:

- What the user asked for (from `user_messages`)
- What was built/fixed/explored (from `assistant_texts`, `commit_messages`, `files_modified`)
- Refactor dossiers worked on (from `refactor_dossiers`)
- Key outcomes

Keep summaries concise and factual. Use project-specific terminology (e.g., "CCOB support",
"library-filter", "hipBLASLt instrumentation") rather than generic descriptions.

If excerpts are too sparse (e.g., only "[Request interrupted by user]"), write a minimal
summary based on files modified and costs.

#### 5. Write the report

Write `.untracked/session-costs.md` with this structure:

```markdown
# Claude Code Session Costs

Generated: YYYY-MM-DD HH:MM
Sessions: N

## Totals

Side-by-side table: metrics on the left, cost-by-model on the right.

| Metric | Value | | Tokens | Count | | Model | Cost | % |
|--------|------:|-|--------|------:|-|-------|-----:|--:|
| **Wall time** | Xd Xh Xm | | **Input** | X.Xk | | Opus4.6 | $X.XX | X% |
| **Turn time** | Xh Xm | | **Output** | X.Xk | | Opus4.5 | $X.XX | X% |
| **Sessions** | N | | **Cache read** | X.XM | | Sonnet4.5 | $X.XX | X% |
| **Date range** | YYYY-MM-DD — MM-DD | | **Cache write** | X.XM | | **Total** | **$X.XX** | |

- **Wall time** = first to last timestamp (includes idle between prompts). Use `Xd Xh Xm` format.
- **Turn time** = sum of turn_duration entries (active processing; closest proxy for API time)
- **Model costs**: Haiku subagent tokens recorded under parent model. Only `/cost` CLI shows true split.

## Sessions

Newest sessions first. Each session entry:

<!-- session:UUID -->
### #N — YYYY-MM-DD HH:MM–HH:MM (Wall Xh0Ym, Turn Xh0Ym) — $X.XX
**Models**: Opus4.6: $X.XX
**Tokens**: in X.Xk, out X.Xk, cache read X.XM, cache write X.XM
**Branch**: main
**Summary**: One or two sentences about what was accomplished.
**Files**: file1.cpp, file2.h (shortened list, max 10)
**Commits**: First commit message (if any)
**Refactors**: rf_foo.md, rf_bar.md (if any)

---

## Model Pricing (from cost.txt)

| Model | Input | Output | Cache Read | Cache Write |
|-------|------:|-------:|-----------:|------------:|
| ... | ... | ... | ... | ... |

## Caveat: Haiku Subagent Costs

The session JSONL files do not reliably distinguish between tokens
served by the main model (Opus/Sonnet) and tokens served by Haiku
subagents. All tokens are recorded under the parent model name.
As a result, Haiku tokens are priced at the parent model's rates,
**overstating the actual cost**. Sessions that heavily use subagents
(Explore, claude-code-guide, etc.) will show higher costs than reality.

For accurate per-model costs, use the `/cost` command in Claude Code.
```

#### 6. Incremental updates

When sessions already exist in the report:
- Keep existing session entries and their summaries unchanged
- Add new sessions at the top of the Sessions section (newest first)
- Recalculate the Totals section to include all sessions
- Update the "Generated" timestamp

### Notes

- The report goes in `.untracked/` — it is NOT version-controlled
- Session numbering is chronological (#1 = oldest)
- Totals wall time format: `Xd Xh Xm` (days, hours, minutes)
- Per-session time format: `Xh0Ym` — no spaces, 2-digit minutes (e.g., `Wall 3h38m, Turn 1h29m`)
- Token counts: use `X.Xk` for thousands, `X.XM` for millions
- Cost precision: 2 decimal places
- If `cost.txt` is missing, tell the user to run `/model` and save the output
- **Pricing stability**: When `cost.txt` is updated with new rates, do NOT recalculate costs
  for existing sessions. Their costs were computed at the pricing in effect when they were
  first added to the report. Only newly extracted sessions use the current `cost.txt` rates.
  The Totals section sums whatever costs are in the report (mixing old and new rates is fine).
