# Directory Structure

This project keeps the Unreal layout and adds the studio's design/production folders.
Do not move existing Unreal folders to match a generic template.

```text
MinecraftDeWish/
├── AGENTS.md                       # Studio instructions for this project
├── MinecraftDeWish.uproject        # UE 5.8 project descriptor
├── Source/
│   └── MinecraftDeWish/
│       ├── Public/                 # C++ headers
│       └── Private/                # C++ implementations
├── Content/                        # Unreal assets, Blueprints, DataTables, maps
├── Config/                         # DefaultEngine.ini, DefaultGame.ini, DefaultInput.ini
├── Docs/                           # Existing design and technical documentation
│   ├── GAME_DESIGN.md
│   ├── SYSTEMS_REFERENCE.md
│   ├── BLOCK_REGISTRY.md
│   ├── MOB_REGISTRY.md
│   ├── TOOL_REGISTRY.md
│   ├── architecture/               # ADRs
│   └── engine-reference/unreal/     # Version-pinned UE API notes
├── design/                         # New studio-authored design documents
│   ├── gdd/                        # Per-system GDDs and game concept
│   ├── narrative/                  # Story, lore, dialogue
│   ├── levels/                     # Level design documents
│   └── balance/                    # Balance spreadsheets and data notes
├── production/                     # Production management
│   ├── sprints/
│   ├── milestones/
│   ├── releases/
│   ├── session-state/              # Ephemeral, gitignored
│   └── session-logs/               # Ephemeral, gitignored
├── prototypes/                     # Throwaway prototypes (isolated from Source/)
└── .opencode/                      # Project-level studio config, docs, rules
```

## Rules of Thumb

- Existing docs in `Docs/` are the current source of truth. New system GDDs go to `design/gdd/`
  and should cross-reference `Docs/SYSTEMS_REFERENCE.md`.
- Every new gameplay system gets a GDD in `design/gdd/<system>.md` before implementation.
- Every cross-system technical decision gets an ADR in `Docs/architecture/`.
- Sprint plans, milestones and releases go to `production/`, never to `Docs/`.
- Prototypes are throwaway: they may skip production standards but must not be merged into
  gameplay code without a rewrite and a design review.
