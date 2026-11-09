# Architecture Decision Records

Binding technical decisions for MinecraftDeWish. Each ADR records the context, the decision,
the alternatives that were rejected and why, and how the decision will be validated.

Format: `adr-NNNN-short-slug.md`, four-digit, never renumbered. An ADR is immutable once
`Accepted`; a reversal is a new ADR with `Supersedes` / `Superseded by` in the Status field.

## Index

| ADR | Title | Status | Date |
| --- | --- | --- | --- |
| [0001](adr-0001-lumen-mandatory.md) | Lumen is mandatory on every quality level | Accepted | 2026-11-09 |
| [0002](adr-0002-generated-vs-player-voxel-writes.md) | Generated voxel writes are not player edits | Accepted | 2026-11-09 |

## Strategy documents

| Document | Purpose |
| --- | --- |
| [voxel-performance-plan.md](voxel-performance-plan.md) | Diagnosis, target architecture, lots and budgets for the voxel streaming rewrite |

## Planned ADRs

See §5 of the performance plan. Write each one **with** its lot, not before: an ADR that
describes unimplemented work drifts within a week.

## Template

```markdown
# ADR-NNNN: Title

## Status
Proposed | Accepted | Deprecated | Superseded by ADR-XXXX

## Date
YYYY-MM-DD

## Context
### Problem Statement
### Constraints
### Requirements

## Decision
### Architecture Diagram
### Key Interfaces

## Alternatives Considered

## Consequences
### Positive
### Negative
### Risks

## Performance Implications

## Migration Plan

## Validation Criteria

## Related Decisions
```

## Rules

- One decision per ADR. If you need the word "and" twice in the title, split it.
- Numbers need a source (measurement, formula, or an explicit "estimate — to be measured").
- Write the rejected alternatives. An ADR without alternatives is a changelog entry, not a decision.
- Update `Docs/SYSTEMS_REFERENCE.md` when a decision changes an implemented system.
