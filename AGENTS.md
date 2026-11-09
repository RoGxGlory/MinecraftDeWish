# MinecraftDeWish — OpenCode Game Studio

This project uses the **OpenCode Game Studio** hierarchy (directors → leads → specialists)
installed globally in `~/.config/opencode`. Project-specific configuration lives in this
repository.

**Language:** the user communicates in French. Reply in French unless they write in another
language. Code, identifiers, commit messages, and design docs stay in English.

## Technology Stack

- **Engine:** Unreal Engine 5.8 (`EngineAssociation` in `MinecraftDeWish.uproject`)
- **Language:** C++ (primary), Blueprint (content, prototyping, designer-facing tuning)
- **Module:** `MinecraftDeWish` (Runtime) — `Source/MinecraftDeWish/{Public,Private}`
- **Plugins enabled:** `ModelingToolsEditorMode`, `GameplayStateTree`
- **Physics:** Chaos
- **VCS:** Git, trunk-based development

> UE 5.8 is newer than the bundled engine reference (5.7) and the model's training data.
> Before writing engine API code, read `Docs/engine-reference/unreal/VERSION.md` and verify
> against the official 5.8 documentation or the installed engine source.

## Project Layout

```text
Source/MinecraftDeWish/Public/       C++ headers (UCLASS, USTRUCT, interfaces)
Source/MinecraftDeWish/Private/      C++ implementations
Content/                             Unreal assets (Blueprints, meshes, materials, data)
Config/                              DefaultEngine.ini, DefaultGame.ini, DefaultInput.ini
Docs/                                Existing design & technical documentation
Docs/engine-reference/unreal/        Version-pinned Unreal API reference notes
Docs/architecture/                   Architecture Decision Records (ADRs)
design/                              New studio GDDs (gdd/, narrative/, levels/, balance/)
production/                          Sprints, milestones, releases, session state
prototypes/                          Throwaway prototypes (isolated from Source/)
.opencode/                           Project-level studio config, docs and rules
```

### Existing documentation

Read these before proposing design or architecture changes:

- `Docs/GAME_DESIGN.md` — game vision, core loop, design pillars
- `Docs/SYSTEMS_REFERENCE.md` — implemented gameplay systems
- `Docs/BLOCK_REGISTRY.md`, `Docs/MOB_REGISTRY.md`, `Docs/TOOL_REGISTRY.md` — content registries
- `PRE_RELEASE.md`, `RELEASE_v0.2.0-alpha.md` — release context

Existing docs stay where they are. New system GDDs go to `design/gdd/`, new ADRs to
`Docs/architecture/`, sprint/milestone artifacts to `production/`.

## Required Reading (project standards)

These files are loaded automatically as instructions:

- `.opencode/docs/technical-preferences.md` — engine, naming, budgets, forbidden patterns
- `.opencode/docs/coding-standards.md` — code and document standards
- `.opencode/docs/coordination-rules.md` — studio delegation rules
- `.opencode/rules/ue-cpp.md` — Unreal C++ rules
- `.opencode/rules/ue-blueprint.md` — Blueprint / C++ boundary
- `.opencode/rules/ue-ui.md` — UMG / CommonUI rules
- `.opencode/rules/ue-network.md` — replication and multiplayer rules
- `.opencode/rules/ue-data.md` — DataTable / DataAsset / balance data rules
- `.opencode/rules/design-docs.md` — GDD standards
- `.opencode/rules/test-standards.md` — automation test standards

## Studio Workflow

Slash commands are available globally (they load the matching studio skill):

| Need | Command |
| --- | --- |
| Onboarding / where to start | `/start` |
| Analyze an existing project | `/project-stage-detect` |
| Reverse-document existing code | `/reverse-document` |
| Decompose the concept into systems | `/map-systems` |
| Write a system GDD | `/design-system` |
| Plan / review a sprint | `/sprint-plan`, `/milestone-review` |
| Review a design or code | `/design-review`, `/code-review` |
| Validate phase readiness | `/gate-check` |
| Release work | `/release-checklist`, `/launch-checklist`, `/changelog`, `/patch-notes` |
| Full team pipeline | `/team-combat`, `/team-narrative`, `/team-ui`, `/team-release`, `/team-polish`, `/team-audio`, `/team-level` |

Selectable primary agents: the three directors (`creative-director`, `technical-director`,
`producer`), the department leads (`game-designer`, `lead-programmer`, `art-director`,
`audio-director`, `narrative-director`, `qa-lead`, `release-manager`, `localization-lead`).
All other agents are subagents: ask the current agent to launch one explicitly, for example
"Use the `unreal-specialist` subagent to review this replication code" or "Use the
`ue-gas-specialist` subagent to design these abilities".

For this project, the default engine specialists are:

- `unreal-specialist` — Blueprint vs C++, UE subsystems, packaging, best practices
- `ue-gas-specialist` — Gameplay Ability System, effects, attributes, tags
- `ue-blueprint-specialist` — BP architecture, BP/C++ boundary, graph standards
- `ue-replication-specialist` — property replication, RPCs, prediction, relevancy
- `ue-umg-specialist` — UMG, CommonUI, widget hierarchy, data binding

## Collaboration Protocol

**User-driven collaboration, not autonomous execution.**
Every task follows: **Question → Options → Decision → Draft → Approval**

- Ask "May I write this to [filepath]?" before using Write/Edit tools.
- Show drafts or detailed summaries before requesting approval.
- Multi-file changes require explicit approval for the full changeset.
- No commits, pushes, or branch changes without explicit user instruction.
- The user makes all final design and architecture decisions.

## Unreal Guardrails (short version)

- C++ for systems and performance-critical code; Blueprint for content, wiring and tuning.
- Never `new`/`delete` a `UObject`; use `NewObject<>`, `CreateDefaultSubobject<>`, `TObjectPtr<>`.
- Mark reflected members with `UPROPERTY()` / `UFUNCTION()`; use `GENERATED_BODY()`.
- Avoid `Tick` unless necessary; prefer timers, delegates and event-driven systems.
- Use `FName` for identifiers, `FText` for player-facing text, `FString` for manipulation.
- Use soft references (`TSoftObjectPtr` / `TSoftClassPtr`) for assets not always needed.
- Gameplay balance values come from DataTables/DataAssets, never magic numbers in code.
- Combat, buffs and stats go through GAS; do not mutate attributes directly.
- Multiplayer is server-authoritative; replicate only what is necessary.
- Profile with Unreal Insights, not just FPS counters.

## Known Limitations

- The studio's shell hooks (`~/.config/opencode/hooks`) are **not active** in OpenCode V2:
  they relied on a Claude Code `settings.json` that the repository does not ship. Do not
  pretend validation hooks ran. Use `/code-review` and the rules instead.
- The bundled engine reference snapshot is for UE 5.7. Treat UE 5.8 API details as
  unverified until checked against official docs or engine source.
