# Test Standards

- Use the Unreal Automation Test Framework (`IMPLEMENT_SIMPLE_AUTOMATION_TEST`, `IMPLEMENT_COMPLEX_AUTOMATION_TEST`).
- Test pure logic separately from actors/world where possible; use `FAutomationEditorCommonUtils` or a test world for actor behavior.
- Naming: `MinecraftDeWish.<System>.<Behavior>` (e.g. `MinecraftDeWish.Crafting.RecipeMatches`).
- Every new gameplay rule gets at least one test for its core rule and one for its main edge case.
- Required coverage areas: voxel generation/chunk boundaries, crafting/smelting, player vitals, inventory/quick slots, mob spawning, save/load.
- Tests must be deterministic: seed random generators, avoid wall-clock time, clean up spawned actors.
- A failing test blocks the feature; do not disable it to "fix" the build.
- Run the automation suite from the editor (`Tools > Test Automation`) or the command line before claiming done.
- For UI changes, attach a before/after screenshot or a precise visual verification note.
- For performance changes, include Unreal Insights captures or `stat` measurements before/after.
