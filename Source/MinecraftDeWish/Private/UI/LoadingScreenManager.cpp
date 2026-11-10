#include "UI/LoadingScreenManager.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "UI/LoadingScreenWidget.h"

void ULoadingScreenManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UE_LOG(LogTemp, Log, TEXT("LoadingScreenManager: initialized"));

	FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &ULoadingScreenManager::OnPreLoadMap);
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ULoadingScreenManager::OnPostLoadMap);
}

void ULoadingScreenManager::Deinitialize()
{
	FCoreUObjectDelegates::PreLoadMap.RemoveAll(this);
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);

	if (LoadingWidget)
	{
		LoadingWidget->RemoveFromParent();
		LoadingWidget = nullptr;
	}

	Super::Deinitialize();
}

void ULoadingScreenManager::OnPreLoadMap(const FString& MapName)
{
	UE_LOG(LogTemp, Log, TEXT("LoadingScreenManager: PreLoadMap %s"), *MapName);

	if (!LoadingWidget)
	{
		LoadingWidget = CreateWidget<ULoadingScreenWidget>(GetGameInstance(), ULoadingScreenWidget::StaticClass());
	}

	if (LoadingWidget)
	{
		LoadingWidget->AddToViewport(100);
	}
}

void ULoadingScreenManager::OnPostLoadMap(UWorld* LoadedWorld)
{
	UE_LOG(LogTemp, Log, TEXT("LoadingScreenManager: PostLoadMap"));

	if (LoadingWidget)
	{
		LoadingWidget->RemoveFromParent();
	}
}
