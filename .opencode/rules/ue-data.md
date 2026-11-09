# Data / Balance Rules

Applies to `Content/Data/**`, DataTables, DataAssets, and any balance data file.

- Gameplay balance values live in DataTables, DataAssets, or JSON consumed by an importer — never in code or Blueprint literals.
- Every row/asset must have a stable, unique ID; never renumber existing IDs (save data may reference them).
- Define a schema (struct) for every DataTable. Validate rows on import or in an automation test.
- Keep block, mob, and tool definitions aligned with `Docs/BLOCK_REGISTRY.md`, `Docs/MOB_REGISTRY.md`, and `Docs/TOOL_REGISTRY.md`.
- New fields need a default value and a migration note for existing save data.
- Balance changes must cite the formula or rationale in the design doc.
- Do not ship debug/test rows in production DataTables; use a separate test asset.
- JSON must be valid UTF-8, no trailing commas, no comments. Validate before committing.
- Curves and formulas must be documented in the GDD (`design/gdd/`) with variable names and units.
