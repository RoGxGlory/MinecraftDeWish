# Design Document Rules

Applies to `design/**` and new design documents in `Docs/**`.

## Required sections

Every mechanic/system GDD must contain these 8 sections:

1. **Overview** — one-paragraph summary
2. **Player Fantasy** — intended feeling and experience
3. **Detailed Rules** — unambiguous mechanics
4. **Formulas** — all math with named variables and units
5. **Edge Cases** — unusual situations and how they resolve
6. **Dependencies** — other systems and documents
7. **Tuning Knobs** — configurable values, ranges, and defaults
8. **Acceptance Criteria** — testable success conditions

## Rules

- One document per mechanic/system in `design/gdd/<system>.md`.
- Cross-reference existing implementation docs (`Docs/SYSTEMS_REFERENCE.md`) instead of duplicating them.
- No implementation detail unless it constrains the design (call it out explicitly).
- Numbers must have a formula or rationale; "feels right" is not a tuning knob.
- Link balance values to their source formula or data table.
- Update the document when the implementation deviates; do not let docs drift.
- Get a `/design-review` before handing a GDD to programmers.
