# UI / UMG Rules

Applies to `Content/UI/**`, widget Blueprints (`WBP_*`), and `Source/**/UI/**`.

- UI never owns game state. It reads from a view model / subsystem and sends intent back via delegates.
- Gameplay code never calls a widget directly. Use events, delegates, or a UI subsystem.
- Separate the HUD into focused widgets (vitals, hotbar, crosshair, inventory) instead of one monolithic WBP.
- Use CommonUI for menus and input routing where possible; keep gamepad/keyboard/mouse navigation working.
- All player-facing text must be localizable (`FText` / `NSLOCTEXT` / string tables). No hardcoded strings in widgets.
- Support UI scaling and safe zones; verify at 16:9, 21:9, and 4:3 (and Steam Deck if targeted).
- Do not bind expensive `Tick` logic in widgets. Update on change, not every frame.
- Accessibility minimum: remappable inputs, readable font scale, subtitle size option, colorblind-safe critical indicators.
- Name widgets `WBP_<Area>_<Purpose>` and keep the hierarchy shallow (avoid deep nested canvases).
