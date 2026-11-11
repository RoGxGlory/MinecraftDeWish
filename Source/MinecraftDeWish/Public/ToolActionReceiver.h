#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ToolActionReceiver.generated.h"

/**
 * IToolActionReceiver
 *
 * Implemented by actors that want to react to animation notifies emitted by
 * tool swing montages (ADR-0005 / ADR-0006):
 *   - ActionWindow: damage/mining window opened
 *   - PlayCue: VFX/SFX at the correct frame
 *   - ActionComplete: swing finished
 *   - MiningLoop: sustained mining loop begin/end/tick
 */
UINTERFACE(BlueprintType)
class MINECRAFTDEWISH_API UToolActionReceiver : public UInterface
{
	GENERATED_BODY()
};

class MINECRAFTDEWISH_API IToolActionReceiver
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ToolAction")
	void HandleActionWindow();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ToolAction")
	void HandlePlayCue(FName CueName);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ToolAction")
	void HandleActionComplete();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ToolAction")
	void HandleMiningLoopBegin();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ToolAction")
	void HandleMiningLoopEnd();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ToolAction")
	void HandleMiningLoopTick(float FrameDeltaTime);
};