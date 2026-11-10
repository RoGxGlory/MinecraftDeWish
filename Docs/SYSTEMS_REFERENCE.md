# Systems Technical Reference — MinecraftDeWish

This document details the C++ subsystem architecture, class hierarchy, delegates, input bindings, and integration pathways.

---

## Architecture Diagram

```
                       ┌─────────────────────────┐
                       │     AWorldGenerator     │
                       └────────────┬────────────┘
                                    │
       ┌────────────────────────────┼────────────────────────────┐
       │                            │                            │
       ▼                            ▼                            ▼
┌──────────────────────┐ ┌──────────────────────┐ ┌──────────────────────┐
│  UMiningQueueSystem  │ │UCraftingSmeltingSyst │ │UDayNightCycleSystem  │
└──────────────────────┘ └──────────────────────┘ └──────────────────────┘
       │                            │                            │
       ▼                            ▼                            ▼
┌──────────────────────┐ ┌──────────────────────┐ ┌──────────────────────┐
│  UMobSpawnerSystem   │ │     AChunkActor      │ │  ABlockHighlightActor│
└──────────────────────┘ └──────────────────────┘ └──────────────────────┘

 Player Pawn Components:
 ├── UBaublesSystem           (Dedicated 5-slot tool equipment)
 ├── UPlayerVitalSystem       (Health, natural regen, XP leveling)
 └── UQuickSlotsInventory     (9-slot quickbar, dynamic opacity, IA_DropItem launch)
```

---

## Subsystem APIs

### 1. `AWorldGenerator`
Core coordinator actor placed in the world.

- **Subsystems Created as Subobjects**:
  - `MiningQueueSystem`: Concurrent block destruction, tool tiers, queue limits.
  - `CraftingSmeltingSystem`: Recipe validation, smelting fuels, tool crafting upgrades.
  - `DayNightCycleSystem`: 20-minute cycle, directional sun rotation, daylight queries.
  - `MobSpawnerSystem`: Spawns hostile mobs in dark areas, torch avoidance, despawning.
- **Key Functions**:
  - `bool BreakBlockAtVoxel(int32 VoxelX, int32 VoxelY, int32 VoxelZ, uint8& OutDroppedBlockID)`: Destroys block, updates meshes, spawns physical Chaos destruction debris, spawns pickup, and consumes Baubles tool durability.
  - `void SpawnChaosBlockDestruction(const FVector& VoxelCenter, uint8 BlockID, const FVector& HitNormal, const FVector& HitPoint)`: Spawns and initializes non-deterministic physical Chaos debris for broken blocks.
  - `bool QueueBlockBreakAtVoxel(int32 VoxelX, int32 VoxelY, int32 VoxelZ, float MiningForceOverride)`: Queues block break, automatically querying `UBaublesSystem` for optimal tool force if override is unset.
  - `bool PlaceBlock(const FVector& WorldLocation, uint8 BlockID)`: Validates placement (including torch resting constraints) and updates meshes.
  - `bool InteractWithTargetBlock()`: Proximity right-click interaction with Crafting Tables and Furnaces.
  - `uint8 GetTargetedBlockType() const`: Returns targeted block ID for UI decisions.
  - `void EnsureDropItemBound()`: Automatically binds `IA_DropItem` on the player pawn's `UEnhancedInputComponent` to execute `DropItemFromInventory`.
  - `int32 SetRenderDistance(int32 NewRenderDistance)`: Runtime render-distance change (Minecraft-style). Clamps to `[MinRenderDistance = 2, MaxRenderDistance = 64]`, persists the value to `GameUserSettings.ini` under `[MinecraftDeWish.GameSettings] RenderDistance`, and re-runs chunk streaming when a player chunk is known. Returns the effective value.
  - `int32 GetRenderDistance() const`: Effective render distance in chunks (radius).
  - `bool SetFastLeaves(bool bNewFastLeaves)`: Stores the player's leaf rendering preference and persists it to `GameUserSettings.ini` under `[MinecraftDeWish.GameSettings] FastLeaves`. **Currently has no visual effect**: leaf-to-leaf culling is disabled because it produced a hollow canopy with the alpha-tested leaf texture, so calling it does not remesh anything. Returns the effective value.
  - `bool GetFastLeaves() const`: Current stored leaf rendering preference.
  - `bool IsBlockTransparent(uint8 BlockID) const`: Both the "can you see through it" query and the face-culling rule — a face is generated when the neighbouring block is transparent. There is no separate `IsFaceVisible`; a face behind a leaf is always generated because the leaf texture is alpha-tested.
  - `const FVoxelFrameStats& GetLastFrameStats() const`: Streaming cost measured during the previous tick (streaming / generation / mesh / remesh milliseconds plus chunk counters). Diagnostic only.
  - `int32 GetActiveChunkCount() const`, `int32 GetPendingGenerationCount() const`, `int32 GetPendingRemeshCount() const`: Streaming queue depths for diagnostics and the benchmark.
  - `void SetBlockGenerated(int32 X, int32 Y, int32 Z, uint8 BlockID)` / `void SetBlockWithMetaGenerated(...)` on `AChunkActor`: world-generation writes that do not flag the chunk as modified (see ADR-0002).
  - `void UpdateCrackMining(const FIntVector& VoxelCoord, float Progress01)`: Blueprint-callable crack-overlay driver. Destroys overlays left on any other voxel, then spawns the overlay for that voxel if needed and advances it to `GetStageFromProgress(Progress01)`. An invalid, air or Bedrock voxel destroys the overlay at that spot instead. See §14 and ADR-0003.
  - `void ClearCrackMining()`: Destroys every live `ABlockCrackActor`. Called when mining is cancelled.
  - `float GetBlockBreakTimeAtVoxel(const FIntVector& VoxelCoord, float MiningForce = 0.0f) const`: `BlueprintPure`. Break time in seconds from the `Durability` column of `Block_DataTable`, using the same formula as the break test. Blueprint divides `BreakTime` by this value to obtain `Progress01`.
- **Settings**:
  - `RenderDistance` (chunks, radius): default `8`, editable in the details panel, clamped 2–64. Restored from `GameUserSettings.ini` at `BeginPlay` before the first streaming pass; a missing entry falls back to the default.
  - `bFastLeaves`: default `true`. Player-facing leaf rendering preference; restored from `GameUserSettings.ini` at `BeginPlay` before the first generation pass. Stored only — it does not change the mesh today (see above).
- **Delegates**:
  - `OnCraftingTableOpened`: Broadcast when player right-clicks a Crafting Table within 4 blocks.
  - `OnFurnaceOpened`: Broadcast when player right-clicks a Furnace within 4 blocks.
  - `OnCrackStageChanged(FIntVector VoxelCoord, int32 Stage)`: Broadcast by `UpdateCrackMining` only when the displayed crack stage actually changes (see §14).

### 1.1 Voxel benchmark (`UVoxelBenchSubsystem`)

Diagnostic harness for the voxel performance plan. It samples one `AWorldGenerator` per frame
and writes a CSV to `Saved/Profiling/VoxelBench-<label>-<timestamp>.csv`.

- **Why it exists**: every optimisation claim in `Docs/architecture/voxel-performance-plan.md`
  must be backed by a measured before/after. This is the measuring instrument.
- **Cost when idle**: none. `IsTickable()` returns false unless a recording is running.
- **Start / stop**: console `mdw.Bench.Start [label]`, `mdw.Bench.Stop`, `mdw.Bench.Marker <text>`;
  Blueprint via `StartRecording` / `StopRecording` / `AddMarker`; automatically with the
  `-VoxelBench` command-line switch or `mdw.Bench.AutoStart 1`.
- **CSV columns**: sample, elapsed seconds, frame ms, active chunks, pending generation, pending
  remesh, chunks generated/remeshed this frame, per-stage milliseconds (streaming, generation,
  mesh, remesh), player position, render distance, fast-leaves flag, optional marker.
- **On stop** it logs the headline summary: frame count, duration, average/p50/p95/p99/max frame
  time, and how many frames exceeded 16.7 ms and 33.3 ms.
- **Scope**: Game and PIE worlds only; the editor preview world has no gameplay streaming.

---

### 2. `AChunkActor`
Renders an individual $16 \times 16 \times 64$ chunk.

- **Components**:
  - `ProceduralMesh`: Solid blocks with `QueryAndPhysics` collision.
  - `NonSolidMesh`: Non-solid blocks (Torches) with `QueryOnly` collision, `Overlap` for Pawns, `Block` for WorldStatic traces.
- **Key Functions**:
  - `GenerateMeshData(FChunkMeshData& SolidMesh, FChunkMeshData& NonSolidMesh)`: Traverses blocks, culls occluded faces, assigns texture indices into `Global_Textures_Array`.
  - `ApplyMeshData(const FChunkMeshData& SolidMesh, const FChunkMeshData& NonSolidMesh)`: Uploads buffers (`UV0` face coordinates + `UV1.X` slice index) to procedural mesh sections.
  - `GetBlockIconTexture(int32 InBlockID)`: Static helper resolving high-res 32×32 Patrix block textures (`/Game/Patrix_Texture_Pack/textures/block/`) with fallback to `/Game/Textures/Blocks/`.
  - `GetBlockIconFromSlice(int32 TextureSliceIndex)`: Resolves 2D icons from the 37 Texture2DArray slices (mapping slice 30 to `oak_leaves`).
  - `GetTargetedBlockBreakTime(float MiningForce)`: Computes exact break duration:
    $$\text{BreakTime} = \frac{\text{BlockDurability}}{\text{EffectiveMiningForce}}$$
    Differentiates Hand mining force ($1.0$) from equipped tool mining speed queried via `BaublesSystem`.
  - `GetHandMiningForce()`: Returns $1.0\text{f}$ (base hand mining force).
  - `GetCurrentBestToolMiningForce()`: Returns the mining force of the best equipped tool in Baubles matching the targeted block.

---

### 3. `UQuickSlotsInventorySystem`
Quickbar inventory management, dynamic opacity, and item drop launching.

- **Functions**:
  - `TryAddItemToPlayerInventory(AActor* PlayerActor, uint8 BlockID, int32 Count, int32& OutRemainingCount)`: Stacks items up to 64, finds first empty slot, and updates HUD.
  - `RefreshQuickSlotVisual(UUserWidget* QuickSlotsWidget, int32 SlotIndex, uint8 BlockID, int32 ItemCount, const FName& BlockName)`:
    - Sets slot texture and item name text.
    - **Dynamic Opacity Rule**: When item present, `SetRenderOpacity(1.0f)` and `SetVisibility(Visible)`. When empty, `SetRenderOpacity(0.0f)` and `SetVisibility(Hidden)`.
  - `DropItemFromInventory(AActor* PlayerActor, int32 SlotIndexOverride)`:
    - Contextual: In gameplay, drops 1 item from the selected quickslot (`SelectedID - 1`). In an inventory/container panel, drops from the mouse-hovered slot.
    - Decrements stack count by 1 (clearing slot when empty).
    - Spawns and launches `ABlockItemPickup` 2.5 blocks forward with realistic 3D launch physics.
  - `UpdateSlotOpacity(UWidget* SlotWidget, bool bHasItem)`: Blueprint-callable helper to set widget opacity to `1.0` or `0.0`.
  - `UpdateAllQuickSlotOpacities(UUserWidget* QuickSlotsWidget)`: Refreshes all 9 quickslots on `WB_QuickSlots`.
  - `SetHoveredInventorySlot(int32 SlotIndex, uint8 BlockID, UObject* SourceContainer)`: Tracks mouse hover for custom container panels (chest, crate, barrel, backpack).
  - `ClearHoveredInventorySlot()`, `GetHoveredSlotIndex()`, `IsInInventoryPanel(AActor* PlayerActor)`.
  - `GetQuickSlotData(int32 SlotIndex)`, `SetQuickSlotData(int32 SlotIndex, const FQuickSlotData& InData)`, `GetAllQuickSlots()`.

---

### 4. `ABlockItemPickup` (`AActor`)
Physical voxel item drops in the world.

- **Visuals**: Procedural mini-voxel cube (25cm) with texture coordinates matching the dropped block's Texture2DArray slice.
- **Dynamics**:
  - `LaunchPickup(const FVector& InVelocity, float InCooldown = 1.2f)`: Launches pickup with initial 3D velocity vector, gravity ($1200\text{ cm/s}^2$), air drag, and floor sweep collision.
  - `PickupCooldown`: Prevents player from instantly picking up dropped items while in mid-air.
  - Magnet pull towards player when within 250cm (after cooldown expires).
  - Merges with nearby pickups of the same BlockID.

---

### 5. `AChaosBlockDestructionActor` (`AActor`)

> **Naming warning.** The class is named "Chaos" but it does **not** use the Chaos
> destruction or rigid-body solver. It is a hand-written ballistic debris simulation
> built on a single `UProceduralMeshComponent`. Do not look for `Chaos::` types in this
> file, and do not document it as Chaos destruction. A rename is tracked as tech debt.

Ballistic voxel debris system. Breaking a block spawns a short-lived burst of small
textured fragments that tumble and settle.

- **Fragmentation**: 32–44 small fragments per broken block, roughly 5–9 cm across
  (`FBlockDestructionParticle::HalfExtents` defaults to 3.5 UU). Fragments are **not**
  convex fracture cells and carry no per-fragment collision component.
- **Visual texture mapping**:
  - Outer faces reuse the broken block's exact sub-UV face coordinates and `Texture2DArray` slice, so debris matches `M_Global`.
  - Freshly exposed faces sample an interior slice (Dirt under Grass, planks under logs, stone under ores), resolved by `ResolveBlockTextures()` against `Block_DataTable`.
- **Simulation**: manual integration, not the physics engine.
  - Per-tick velocity integration with `GravityStrength = 1350 cm/s²`.
  - Per-particle ground sweeps against world geometry; `bIsGrounded` latches when a fragment settles.
  - Initial burst is biased by the mining strike normal and hit point, plus upward pop and randomized angular velocity.
  - All fragments share one `UProceduralMeshComponent`; buffers are rebuilt each tick, so the whole burst costs one draw call.
- **Lifecycle**: `SimulationLifetime = 2.0 s` of tumbling, then a `FadeDuration = 0.8 s` shrink-and-fade, then the actor destroys itself.
- **Tunables** (all `EditAnywhere`): `BlockScale`, `SimulationLifetime`, `FadeDuration`, `GravityStrength`.

---

### 6. `UBaublesSystem` (`UActorComponent`)
Player equipment component managing dedicated tool slots.

- **Slots**:
  - `FToolInstance PickaxeSlot`
  - `FToolInstance ShovelSlot`
  - `FToolInstance AxeSlot`
  - `FToolInstance HoeSlot`
  - `FToolInstance SwordSlot`
- **Key Functions**:
  - `bool EquipTool(const FToolInstance& Tool)`: Equips tool to its dedicated slot based on `Tool.ToolType`.
  - `FToolInstance UnequipTool(EToolType SlotType)`: Removes and returns tool from slot.
  - `float GetBestMiningForce(uint8 BlockID) const`: Returns mining speed multiplier of the equipped tool specialized for `BlockID`.
  - `bool ConsumeToolDurability(uint8 BlockID)`: Deducts 1 durability point from the appropriate tool.
  - `float GetAttackDamage() const`: Returns sword combat damage bonus (or 1.0 fist damage).
  - `void ToggleBaublesUI()`: Toggles UI state and broadcasts `OnBaublesToggled`.
- **Delegates**:
  - `OnToolEquipped(EToolType SlotType, const FToolInstance& ToolData)`
  - `OnToolUnequipped(EToolType SlotType, const FToolInstance& ToolData)`
  - `OnBaublesToggled(bool bIsOpen)`

---

### 7. `FToolInstance` (`ToolTypes.h`)
Structure representing an individual tool instance.

- **Properties**: `ToolType`, `ToolTier`, `CurrentDurability`, `MaxDurability`.
- **Key Functions**:
  - `static UTexture2D* GetToolIconTexture(EToolType Type, EToolTier Tier)`: Returns 32×32 Patrix item icons for all tool combinations (`wooden_pickaxe` to `netherite_pickaxe`).
  - `UTexture2D* GetIconTexture() const`: Instance helper.
  - `float GetBaseMiningForce() const`: Returns tier multiplier (Wood: 2×, Stone: 4×, Iron: 6×, Diamond: 10×, Obsidian: 15×).
  - `float GetEffectiveMiningForce(uint8 BlockID) const`: Evaluates tool specialization against block category.

---

### 8. `UPlayerVitalSystem` (`UActorComponent`)
Player health and progression manager.

- **Properties**:
  - `MaxHealth = 20.0f`, `CurrentHealth = 20.0f`
  - `bRegenEnabled = true`, `RegenRate = 0.25f` (1 HP every 4s)
  - `CurrentXP = 0`, `XPToNextLevel = 7`, `Level = 0`, `XPProgress = 0.0f`
- **Key Functions**:
  - `float ApplyDamage(float DamageAmount, AActor* DamageSource)`: Deals damage, updates health, triggers death if $\le 0$.
  - `float Heal(float HealAmount)`: Restores health up to `MaxHealth`.
  - `void AddXP(int32 Amount)`: Awards XP, advances level using $XP = 7 + (Level \times 3)$.
  - `void Respawn(const FVector& SpawnLocation)`: Restores full health and marks alive.
- **Delegates**:
  - `OnHealthChanged(float NewHealth, float MaxHealth, float DamageAmount)`
  - `OnXPChanged(int32 CurrentXP, int32 XPToNextLevel, int32 Level)`
  - `OnLevelUp(int32 NewLevel)`
  - `OnPlayerDied()`

---

### 9. `UDayNightCycleSystem` (`UActorComponent`)
Manages planetary daylight progression and sunlight calculations.

- **Properties**:
  - `CycleDurationSeconds = 1200.0f` (20 minutes)
  - `TimeOfDay` in $[0.0, 1.0)$
  - `SunLightActor`: Reference to Directional Light actor rotated with the sun.
- **Key Functions**:
  - `bool IsDay() const`: True if `TimeOfDay` $\in [0.0, 0.5)$.
  - `bool IsNight() const`: True if `TimeOfDay` $\in [0.5, 1.0)$.
  - `float GetSkyLightIntensity() const`: Returns daylight factor $[0.0 - 1.0]$.
  - `float GetSunPitchAngle() const`: Calculates sun pitch in degrees.
  - `FString GetTimeString() const`: Formatted clock string (e.g., "Day 2, 14:30").

---

### 10. `UMobSpawnerSystem` (`UActorComponent`)
Handles hostile creature life cycles and ecological density.

- **Properties**:
  - `MaxMobCap = 20`
  - `MinSpawnDistance = 2400.0f` (24 blocks), `MaxSpawnDistance = 12800.0f` (128 blocks)
  - `SpawnInterval = 2.0f`
  - `TorchLightRadius = 5` blocks
- **Key Functions**:
  - `int32 CalculateLightLevel(const FIntVector& VoxelPos) const`: Combines torch proximity with sky exposure to compute 0–15 light level.
  - `AMobCharacter* SpawnMob(EMobType MobType, const FVector& Location)`: Instantiates mob actor with type definition.
  - `ProcessSpawnCycle()`: Spawns mobs in dark areas (light level $< 7$) on valid solid surfaces.
  - `ProcessDespawn()`: Cleans up mobs further than 128m from the player.

---

### 11. `AMobCharacter` (`ACharacter`)
Autonomous hostile creature pawn.

- **Mob Types**: `Zombie`, `Skeleton`, `Spider`, `Creeper`.
- **Key Functions**:
  - `static void GetMobTexturePaths(EMobType InType, FString& OutBaseColor, FString& OutNormal, FString& OutSpecular)`: Returns Patrix entity PBR texture paths.
- **Key Behaviors**:
  - **Sunlight Burning**: Mobs with `bBurnsInSunlight` take 1 HP/s when exposed to open sky during daytime.
  - **Spider Wall Climbing**: Raycasts ahead; when obstructed by vertical blocks, applies upward vertical climbing velocity.
  - **Creeper Detonation**: Activates 1.5s fuse timer near player, detonates with radial damage and a 3-block crater voxel break.
  - **Death & Drops**: Releases loot items from `FMobDropEntry` tables and spawns `AXPOrbPickup` physical actors.

---

### 12. `AXPOrbPickup` (`AActor`)
Physical experience orb in the world.

- **Visuals**: Procedural diamond octahedron mesh with glowing green-yellow vertex colors.
- **Dynamics**: Physics parabolic bounce, idle rotational bobbing, and player magnet attraction within 3.5m.
- **Interaction**: Deposits experience into `UPlayerVitalSystem` on contact.

---

### 13. PBR Master Material (`M_Global`)
Unified terrain shader sampling 32×32 texture arrays via `UV0` (face coordinates) and `UV1.X` (slice index):
- **`Textures_Array`** (TextureSampleParameter2DArray): Base Color.
- **`Normals_Array`** (TextureSampleParameter2DArray, Sampler: Normal): Tangent-space normals.
- **`Specular_Array`** (TextureSampleParameter2DArray, Sampler: Linear Color):
  - Red Channel (Smoothness) $\rightarrow$ `OneMinus` $\rightarrow$ Roughness.
  - Green Channel (Reflectance) $\rightarrow$ Metallic.
  - Alpha Channel $\rightarrow$ Multiply with Base Color $\rightarrow$ Emissive.

#### Vertex attribute contract (mesh $\rightarrow$ material)

| Attribute | Channel | Meaning |
|---|---|---|
| `UV0.XY` | – | Face UVs (0..1), rotated per block and phase-shifted on isotropic tops |
| `UV1.X` | – | Primary `Texture2DArray` slice index |
| `UV1.Y` | – | Secondary (neighbouring biome) slice index |
| `VertexColor.R` | R | Slice index (legacy path, mirrors `UV1.X`) |
| `VertexColor.G` | G | Tiling-breakup mask (`1` = natural terrain, `0` = constructed blocks) |
| `VertexColor.B` | B | Biome blend weight `0..0.5`, interpolated per face corner |

#### Biome cross-fade

`M_Global` adds a second pair of array samples (`Base_Color_Blend`, `Normals_Blend`) whose slice comes from `UV1.Y`:

```
ComponentMask(G) of UV1 --> AppendVector(UV0, UV1.G) --> Base_Color_Blend / Normals_Blend
Primary BaseColor chain --Lerp(A)--> LinearInterpolate_2 --> BaseColor
Base_Color_Blend --------Lerp(B)-->            ^ (Alpha = VertexColor.B)
Primary normal chain -----Lerp(A)--> LinearInterpolate_3 --> FlattenNormal.Normal
Normals_Blend -----------Lerp(B)-->            ^ (Alpha = VertexColor.B)
```

- Blocks outside the natural surface set (Grass / Dirt / Sand / Stone) write `UV1.Y = UV1.X` and `VertexColor.B = 0`, so the extra samples resolve to the primary slice and the lerp is a no-op.
- The weight is capped at `0.5`, so a block's own texture always stays dominant and the transition is symmetric on both sides of a border.
- All six faces blend, not just tops: the target material's texture is looked up per face, so cliffs and terrain steps fade vertically as well.
- Driven by `AWorldGenerator::GetSurfaceMaterialWeightGrid()` (chunk-wide, field samples shared, one-cell apron) and sampled per face corner in `AChunkActor::GenerateMeshData()`.
- **Surface-only.** The fade is gated to each column's top four blocks (`Z + 3 >= ColumnTopZ`), so cave walls, cliff faces and deep stone keep their own material instead of fading halfway toward grass.
- **8-bit vertex colour is sRGB-decoded.** Both `CreateMeshSection_LinearColor` calls pass `bSRGBConversion = true`. The material's `VertexColor` node decodes the stored byte as sRGB; with the default (`false`) a linear `0.5` weight reaches the shader as `sRGBToLinear(0.5) ≈ 0.21`. That turns the whole fade into `pow(weight, 2.2)`: the border collapses into a hard step and only `1.0` reads as a full blend.

**Alignment rule (important).** The 50% point of the fade is derived from the *signed distance to the exact thresholds* `GetBiomeAt()` uses (`Temp > 0.60`, `Moisture < 0.40`, `Continental > 0.35`), and both paths sample the same `SampleBiomeFields()`. The block ID flips exactly on those contours, so both sides of a border meet at `0.5` and the seam is continuous. Applying a warp/jitter to the fade fields *only* slides the fade off the flip line and reintroduces a hard edge — that was the original bug. A flat `0.75`-block plateau guarantees the shared corner between two differently-classified blocks lands on `0.5`.

**Width in blocks.** The signed distance is divided by the local field gradient (central differences on the apron), giving `BlocksToBorder`, and the fade runs `SmoothStep(0.75, 0.75 + BiomeBlendWidth, BlocksToBorder)`. Measuring in blocks keeps the transition the same physical width everywhere; in field units it stretched to 30–40 blocks on flat ground and shrank to ~2 blocks on steep slopes.

**Per-material membership.** Each field yields a 0..1 membership value: `0.5` exactly on the contour, `1` well inside the region and `0` well outside. The three material weights are then `Sand = S*(1-R)`, `Stone = R*(1-S)`, `Grass = (1-S)*(1-R)` (normalized). A fade must not cap the *dominant* material at 0.5 — doing so leaves the whole region as a permanent 50/50 wash and turns the fade's outer edge into the visible "line".

**Organic shape.** `SampleBiomeFields()` warps the field coordinates by `BiomeBlendWarp` (two octaves) and adds `BiomeBlendNoise` field jitter. Because the hard biome lookup goes through the same sample, the *block-level* border meanders organically instead of following the raw noise contour.

**Terrain height.** `GetTerrainHeight(..., DesertBlend)` ramps the desert flattening with `GetDesertBlendAt()` instead of switching on the discrete biome, so a desert no longer meets its neighbours with a cliff. That ramp is in field units (`BiomeHeightRamp`), since it shapes the terrain rather than the texture.

#### Tiling-grid dithering

`AChunkActor::bStochasticTopUVs` gives every isotropic top face a deterministic per-block UV phase offset (derived from the block's world position). The UVs are intentionally left outside `[0,1]` so the sampler's Wrap mode tiles them; applying `Frac` would collapse the quad onto a single texel.

Tunables (`AWorldGenerator`): `BiomeBlendWidth` (fade half-width in **blocks**, default 4), `BiomeBlendNoise` (border interlock), `BiomeBlendWarp` (large-scale meander, blocks), `BiomeBlendStrength` (0 disables), `BiomeHeightRamp` (desert height-ramp half-width, field units).

---

### 14. `ABlockCrackActor` (`AActor`)

Vanilla-style progressive crack overlay. While a block is being mined, one of the ten
`destroy_stage_0..9` textures is drawn over its faces so the player sees the damage
accumulate. It is **not** part of `AChaosBlockDestructionActor` (§5): the two systems share
no code and the `destroy_stage_*` textures are used here only.

**Progress source.** The actor holds no timer and no `Tick`. Blueprint owns mining state, so
`BP_HorrorCharacter` computes the ratio and pushes it:

```
Progress01 = BreakTime / AWorldGenerator::GetBlockBreakTimeAtVoxel(LastTargetedBlockVoxel, Current_Mining_Force)
```

`GetBlockBreakTimeAtVoxel` reuses the break test's own formula (`UBaublesSystem::GetBestMiningForce`
→ `GetToolMiningForce` → `UMiningQueueSystem::CalculateBreakTime` over the `Durability` column),
so stage 9 lands on the same frame as the break instead of one frame later. Wiring and rationale
are recorded in ADR-0003.

- **Stage selection**: `GetStageFromProgress(Progress)` = `FloorToInt(Clamp(Progress, 0, 1) * 10)`,
  clamped to `[0, 9]`. Texture parameter writes happen only when the index changes, so a
  mining tick that stays in the same stage costs nothing.
- **Geometry** (`UProceduralMeshComponent`, root, collision disabled, shadows off):
  - Cube blocks: six per-face quads with 0..1 UVs, pushed `0.75 UU` outward along each face
    normal to win the depth test against the chunk surface without z-fighting. All six faces are
    generated; hidden faces are not culled because the overlay is a separate actor.
  - Torches: two crossed vertical diagonal planes mirroring `AChunkActor::AddTorchMesh`, each
    double-sided and offset `0.75 UU` along both normal directions. Local Z spans
    `-0.5·S .. +0.3·S` and X/Y `±0.35·S`, so the overlay follows the torch shape instead of
    drawing a cube around it.
  - Actor location is the block centre: `(Voxel + 0.5) × BlockScale`, identical to the conversion
    `AChunkActor` uses when it builds chunk meshes.
- **Material**: a dynamic instance of `M_BlockCrack` (Masked / Unlit / TwoSided).
  `OpacityMask = CrackTex.A`, `Emissive = CrackTex.RGB × 0.55`, `OpacityMaskClipValue = 0.15`.
  The mask must come from the **alpha** channel: the source PNGs are near-greyscale in RGB
  (measured max 126/162/176 for stages 1/4/9) while alpha is exactly 0 or 255 with a mean of
  23.9/83.7/195.2. Deriving the mask from luminance — or from `OneMinus` — inverts the overlay.
- **Lifecycle**:
  - First mining tick → `AWorldGenerator::UpdateCrackMining` spawns the overlay.
  - `IA_Break.Canceled|Completed` → `Clear and Invalidate Timer by Handle` →
    `ClearCrackMining()` → every overlay destroyed. (`Event ResetBreakEvent` in
    `BP_HorrorCharacter` is dead code — it is never invoked.)
  - `BreakBlockAtVoxel` destroys the overlay of the voxel it turns to air, then the next mining
    tick re-targets the block below.
  - `UpdateCrackMining` with an invalid voxel, air, or **Bedrock** destroys the overlay at that
    spot instead of drawing on it.
  - Overlays are keyed **per voxel** in `BlockCrackOverlays` (`TMap<FIntVector, TWeakObjectPtr<ABlockCrackActor>>`),
    but only one may be alive: every `UpdateCrackMining` first destroys the overlays of all other
    voxels (`ShouldKeepCrackOverlay`). Changing target without releasing therefore moves the overlay
    instead of stacking a second one.
  - `OnCrackStageChanged(VoxelCoord, Stage)` is broadcast only on a stage transition, so HUD or
    debug consumers do not need to poll.
- **Tunables**:
  - `StageTextures[0..9]` — resolved in the constructor from
    `/Game/Patrix_Texture_Pack/textures/block/destroy_stage_<n>`; no other system hard-references them.
  - `CrackMaterialTemplate` — `M_BlockCrack`, overridden per instance.
  - Geometry offset — `const float Epsilon = 0.75f` in both mesh builders (compile-time, not a
    property). Raise it if z-fighting appears at extreme view distances.
  - `CurrentStage`, `VoxelCoord`, `BlockID` are `BlueprintReadOnly` for HUD/debug consumers.
