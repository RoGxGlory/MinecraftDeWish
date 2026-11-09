# Blueprint / C++ Boundary Rules

Applies to `Content/**` and Blueprint-facing C++ APIs.

## Default split

- **C++** owns gameplay systems, data processing, performance-critical loops, replication, and save/load.
- **Blueprint** owns content assembly, level wiring, visual effects, designer tuning, and prototyping.
- A Blueprint graph with more than ~20 nodes in one function is a signal to move logic to C++.

## Blueprint rules

- Expose tuning knobs with `UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=...)` and sane `ClampMin` / `ClampMax`.
- Expose designer-callable functions with `UFUNCTION(BlueprintCallable)`; use `BlueprintPure` for getters.
- Use `BlueprintNativeEvent` / `BlueprintImplementableEvent` for designer hooks, not ad-hoc casts.
- Keep Blueprint graphs small, commented, and free of duplicated logic.
- Use Data Assets / Data Tables for content variation (mobs, blocks, tools), not copied Blueprints.
- Blueprint naming: `BP_` actors, `WBP_` widgets, `DA_` data assets, `DT_` data tables, `M_` materials, `MI_` material instances, `T_` textures, `S_` static meshes.
- Never hardcode balance values in a Blueprint; put them in the Data Asset/Data Table.
- Do not reference UI widgets directly from gameplay Blueprints; use delegates/events.

## C++ API rules for Blueprints

- Do not expose raw pointers; expose `TObjectPtr<>` or value types.
- Mark functions `const` / `BlueprintPure` when they do not mutate state.
- Use `meta=(DisplayName="...")` when the C++ name is not designer-friendly.
- Deprecate Blueprint-exposed APIs with `meta=(DeprecatedFunction, DeprecationMessage="...")` instead of silently removing them.
