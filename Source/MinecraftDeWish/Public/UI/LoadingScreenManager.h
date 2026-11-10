// LAMPLIGHTER — loading screen manager: shows a UMG widget during level travel.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LoadingScreenManager.generated.h"

class ULoadingScreenWidget;

UCLASS()
class MINECRAFTDEWISH_API ULoadingScreenManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	void OnPreLoadMap(const FString& MapName);
	void OnPostLoadMap(UWorld* LoadedWorld);

	UPROPERTY()
	TObjectPtr<ULoadingScreenWidget> LoadingWidget;
};
