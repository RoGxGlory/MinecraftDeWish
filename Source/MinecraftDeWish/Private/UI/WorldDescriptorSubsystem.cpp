#include "UI/WorldDescriptorSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"

UWorldDescriptorSubsystem* UWorldDescriptorSubsystem::Get(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	const UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UWorldDescriptorSubsystem>() : nullptr;
}

void UWorldDescriptorSubsystem::SetPendingWorld(const FWorldDescriptor& Descriptor)
{
	PendingWorld = Descriptor;
	LastWorldName = Descriptor.WorldName;
	bHasPendingWorld = true;
}

FWorldDescriptor UWorldDescriptorSubsystem::ConsumePendingWorld()
{
	FWorldDescriptor Result = PendingWorld;
	PendingWorld = FWorldDescriptor();
	bHasPendingWorld = false;
	return Result;
}
