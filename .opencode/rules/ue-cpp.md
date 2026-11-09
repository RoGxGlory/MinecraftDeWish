# Unreal C++ Rules

Applies to `Source/MinecraftDeWish/**`.

- Use `UCLASS()`, `USTRUCT()`, `UENUM()`, `UPROPERTY()`, `UFUNCTION()` and `GENERATED_BODY()` correctly.
- Never `new` / `delete` a `UObject`. Use `NewObject<>`, `CreateDefaultSubobject<>`, `TObjectPtr<>`.
- Prefer `TObjectPtr<>` for `UObject` members; never expose a raw `UObject*` to the GC without `UPROPERTY()`.
- Use the Unreal naming prefixes: `A` (Actor), `U` (UObject/Component), `F` (struct), `E` (enum), `I` (interface).
- Use `FName` for identifiers, `FText` for player-facing text, `FString` for manipulation only.
- Prefer `TArray`, `TMap`, `TSet` over STL containers.
- Avoid `Tick` unless the system genuinely needs per-frame work. Prefer timers, delegates, and event-driven updates.
- Mark functions `const` when they do not mutate state. Use `FORCEINLINE` sparingly and only with evidence.
- Guard pointers with `IsValid()` before use; do not assume an actor survived a frame.
- Use `SCOPE_CYCLE_COUNTER` and Unreal Insights for profiling critical paths.
- Use soft references (`TSoftObjectPtr`, `TSoftClassPtr`) for assets that are not always needed.
- Do not put gameplay balance values in code. Read them from DataTables/DataAssets.
- Log with a project log category, not `UE_LOG(LogTemp, ...)` in production code.
- Every new gameplay rule needs an automation test (see `test-standards.md`).
- Changes to public headers need a rebuild of the editor target and a smoke test in PIE.
