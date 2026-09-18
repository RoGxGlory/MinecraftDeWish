# Systems Technical Reference — MinecraftDeWish

This document details the C++ subsystem architecture, class hierarchy, delegates, and integration pathways.

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
 ├── UBaublesSystem        (Dedicated 5-slot tool equipment)
 ├── UPlayerVitalSystem    (Health, natural regen, XP leveling)
 └── QuickSlotsInventory   (9-slot quickbar item pickup/stacking)
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
  - `bool BreakBlockAtVoxel(int32 VoxelX, int32 VoxelY, int32 VoxelZ, uint8& OutDroppedBlockID)`: Destroys block, updates meshes, spawns pickup, and consumes Baubles tool durability.
  - `bool QueueBlockBreakAtVoxel(int32 VoxelX, int32 VoxelY, int32 VoxelZ, float MiningForceOverride)`: Queues block break, automatically querying `UBaublesSystem` for optimal tool force if override is unset.
  - `bool PlaceBlock(const FVector& WorldLocation, uint8 BlockID)`: Validates placement (including torch resting constraints) and updates meshes.
  - `bool InteractWithTargetBlock()`: Proximity right-click interaction with Crafting Tables and Furnaces.
  - `uint8 GetTargetedBlockType() const`: Returns targeted block ID for UI decisions.
- **Delegates**:
  - `OnCraftingTableOpened`: Broadcast when player right-clicks a Crafting Table within 4 blocks.
  - `OnFurnaceOpened`: Broadcast when player right-clicks a Furnace within 4 blocks.

---

### 2. `AChunkActor`
Renders an individual $16 \times 16 \times 64$ chunk.

- **Components**:
  - `ProceduralMesh`: Solid blocks with `QueryAndPhysics` collision.
  - `NonSolidMesh`: Non-solid blocks (Torches) with `QueryOnly` collision, `Overlap` for Pawns, `Block` for WorldStatic traces.
- **Key Functions**:
  - `GenerateMeshData(FChunkMeshData& SolidMesh, FChunkMeshData& NonSolidMesh)`: Traverses blocks, culls occluded faces, assigns texture indices into Texture2DArray.
  - `ApplyMeshData(const FChunkMeshData& SolidMesh, const FChunkMeshData& NonSolidMesh)`: Uploads buffers to procedural mesh sections.
  - `GetBlockIconTexture(int32 InBlockID)`: Static helper mapping block IDs to UI icon textures (matches `EBlockType` exactly).
  - `GetBlockDataAtLocation(...)`: Interfaced by `AC_DestroySystem` and `WB_BlockInfo`.

---

### 3. `UBaublesSystem` (`UActorComponent`)
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

### 4. `UPlayerVitalSystem` (`UActorComponent`)
Player health and progression manager.

- **Properties**:
  - `MaxHealth = 20.0f`, `CurrentHealth = 20.0f`
  - `bRegenEnabled = true`, `RegenRate = 0.25f` (1 HP every 4s)
  - `CurrentXP = 0`, `XPToNextLevel = 7`, `Level = 0`, `XPProgress = 0.0f`
- **Key Functions**:
  - `float ApplyDamage(float DamageAmount, AActor* DamageSource)`: Deals damage, updates health, triggers death if $\le 0$.
  - `float Heal(float HealAmount)`: Restores health up to `MaxHealth`.
  - `void AddXP(int32 Amount)`: Awards XP, advances level, recalculates thresholds using $XP = 7 + (Level \times 3)$.
  - `void Respawn(const FVector& SpawnLocation)`: Restores full health and marks alive.
- **Delegates**:
  - `OnHealthChanged(float NewHealth, float MaxHealth, float DamageAmount)`
  - `OnXPChanged(int32 CurrentXP, int32 XPToNextLevel, int32 Level)`
  - `OnLevelUp(int32 NewLevel)`
  - `OnPlayerDied()`

---

### 5. `UDayNightCycleSystem` (`UActorComponent`)
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

### 6. `UMobSpawnerSystem` (`UActorComponent`)
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

### 7. `AMobCharacter` (`ACharacter`)
Autonomous hostile creature pawn.

- **Mob Types**: `Zombie`, `Skeleton`, `Spider`, `Creeper`.
- **Key Behaviors**:
  - **Sunlight Burning**: Mobs with `bBurnsInSunlight` take 1 HP/s when exposed to open sky during daytime.
  - **Spider Wall Climbing**: Raycasts ahead; when obstructed by vertical blocks, applies upward vertical climbing velocity.
  - **Creeper Detonation**: Activates 1.5s fuse timer near player, detonates with radial damage and a 3-block crater voxel break.
  - **Death & Drops**: Releases loot items from `FMobDropEntry` tables and spawns `AXPOrbPickup` physical actors.

---

### 8. `AXPOrbPickup` (`AActor`)
Physical experience orb in the world.

- **Visuals**: Procedural diamond octahedron mesh with glowing green-yellow vertex colors.
- **Dynamics**: Physics parabolic bounce, idle rotational bobbing, and player magnet attraction within 3.5m.
- **Interaction**: Deposits experience into `UPlayerVitalSystem` on contact.

---

### 9. `UQuickSlotsInventorySystem`
Quickbar inventory management and HUD synchronization.

- **Functions**:
  - `TryAddItemToPlayerInventory(...)`: Stacks items up to 64, finds first empty slot, updates HUD.
  - `RefreshQuickSlotVisual(...)`: Sets slot texture, updates `ItemCount` text block, and synchronizes Blueprint integer properties.
