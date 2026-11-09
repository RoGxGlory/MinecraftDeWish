# Studio Hooks — Inactive in OpenCode V2

The upstream repository documents eight shell hooks (`validate-commit.sh`, `session-start.sh`,
`detect-gaps.sh`, `pre-compact.sh`, `session-stop.sh`, `log-agent.sh`, `validate-assets.sh`,
`validate-push.sh`) wired through a Claude Code `.opencode/settings.json`.

**That `settings.json` is not shipped, and OpenCode V2 does not read it.** The scripts are
kept under `~/.config/opencode/hooks/` for reference only. They do **not** run automatically.

What this means in practice:

- Do not claim that commit validation, push protection, or asset validation ran.
- Do not claim that a session-start context block was injected.
- Use the project rules (`~/.config/opencode/rules`, `<project>/.opencode/rules`) and run
  `/code-review`, `/asset-audit`, and `/gate-check` explicitly instead.
- If you want equivalent automation later, port the scripts to an OpenCode V2 plugin in
  `~/.config/opencode/plugins/` (semantic hooks are exposed by the plugin API), or add a
  pre-commit Git hook in `.git/hooks/`.
