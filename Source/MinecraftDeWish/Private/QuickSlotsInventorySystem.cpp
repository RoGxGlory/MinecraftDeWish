#include "QuickSlotsInventorySystem.h"
#include "BlockItemPickup.h"
#include "BaublesSystem.h"
#include "ChunkActor.h"
#include "Engine/World.h"
#include "Engine/DataTable.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "UObject/UObjectIterator.h"

// Static inventory tracking for the player's 9 quickslots
static TArray<FQuickSlotData> GPlayerQuickSlots;
static int32 GHoveredInventorySlot = -1;
static uint8 GHoveredBlockID = 0;
static TWeakObjectPtr<UObject> GHoveredSourceContainer = nullptr;

UQuickSlotsInventorySystem::UQuickSlotsInventorySystem()
{
	QuickSlots.SetNum(NUM_QUICK_SLOTS);
	for (int32 i = 0; i < NUM_QUICK_SLOTS; ++i)
	{
		QuickSlots[i].MaxStackSize = MAX_STACK_SIZE;
	}
}

UUserWidget* UQuickSlotsInventorySystem::ResolveQuickSlotsWidget(AActor* PlayerActor)
{
	if (!PlayerActor)
	{
		return nullptr;
	}

	UWorld* World = PlayerActor->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	// 1. Check AC_Inventory ActorComponent directly on the player
	for (UActorComponent* Comp : PlayerActor->GetComponents())
	{
		if (Comp && Comp->GetClass()->GetName().Contains(TEXT("AC_Inventory"), ESearchCase::IgnoreCase))
		{
			// Read the HUD property from AC_Inventory
			if (FObjectProperty* HUDProp = CastField<FObjectProperty>(Comp->GetClass()->FindPropertyByName(FName(TEXT("HUD")))))
			{
				UObject* HUDObj = HUDProp->GetObjectPropertyValue_InContainer(Comp);
				if (HUDObj)
				{
					// From WB_HUD, find the WB_QuickSlots property
					if (FObjectProperty* QSProp = CastField<FObjectProperty>(HUDObj->GetClass()->FindPropertyByName(FName(TEXT("WB_QuickSlots")))))
					{
						UUserWidget* QSWidget = Cast<UUserWidget>(QSProp->GetObjectPropertyValue_InContainer(HUDObj));
						if (QSWidget && !QSWidget->HasAnyFlags(RF_ClassDefaultObject))
						{
							return QSWidget;
						}
					}
				}
			}
		}
	}

	// 2. Check BPI_Inventory interface function GetHUDRef on PlayerActor
	UFunction* GetHUDFunc = PlayerActor->FindFunction(FName(TEXT("GetHUDRef")));
	if (GetHUDFunc)
	{
		struct FGetHUDRefParms
		{
			UUserWidget* ReturnHUD = nullptr;
		} Parms;

		PlayerActor->ProcessEvent(GetHUDFunc, &Parms);
		if (Parms.ReturnHUD)
		{
			if (FObjectProperty* QSProp = CastField<FObjectProperty>(Parms.ReturnHUD->GetClass()->FindPropertyByName(FName(TEXT("WB_QuickSlots")))))
			{
				UUserWidget* QSWidget = Cast<UUserWidget>(QSProp->GetObjectPropertyValue_InContainer(Parms.ReturnHUD));
				if (QSWidget && !QSWidget->HasAnyFlags(RF_ClassDefaultObject))
				{
					return QSWidget;
				}
			}
		}
	}

	// 3. Viewport search fallback: find active, non-CDO widget in viewport
	for (TObjectIterator<UUserWidget> It; It; ++It)
	{
		UUserWidget* Widget = *It;
		if (!Widget || Widget->HasAnyFlags(RF_ClassDefaultObject) || Widget->GetWorld() != World)
		{
			continue;
		}

		if (Widget->IsInViewport() && Widget->GetClass()->GetName().Contains(TEXT("WB_QuickSlots")))
		{
			return Widget;
		}
	}

	return nullptr;
}

void UQuickSlotsInventorySystem::RefreshQuickSlotVisual(
	UUserWidget* QuickSlotsWidget,
	int32 SlotIndex, // 1 to 9
	uint8 BlockID,
	int32 ItemCount,
	const FName& BlockName
)
{
	if (!QuickSlotsWidget || SlotIndex < 1 || SlotIndex > NUM_QUICK_SLOTS)
	{
		return;
	}

	// Resolve the slot's UImage widget
	UImage* SlotImage = nullptr;

	// 1. Check ItemImages / Item Images array property (from WB_QuickSlots Setup Images)
	const FName ArrayNames[] = {
		FName(TEXT("ItemImages")),
		FName(TEXT("Item Images")),
		FName(TEXT("Item_Images"))
	};

	for (const FName& ArrName : ArrayNames)
	{
		if (FArrayProperty* ArrProp = CastField<FArrayProperty>(QuickSlotsWidget->GetClass()->FindPropertyByName(ArrName)))
		{
			FScriptArrayHelper ArrayHelper(ArrProp, ArrProp->ContainerPtrToValuePtr<void>(QuickSlotsWidget));
			const int32 TargetIdx = SlotIndex - 1; // 0-based
			if (ArrayHelper.IsValidIndex(TargetIdx))
			{
				if (FObjectPropertyBase* ObjInner = CastField<FObjectPropertyBase>(ArrProp->Inner))
				{
					UObject* ItemObj = ObjInner->GetObjectPropertyValue(ArrayHelper.GetRawPtr(TargetIdx));
					SlotImage = Cast<UImage>(ItemObj);
					if (SlotImage)
					{
						break;
					}
				}
			}
		}
	}

	// 2. Fallback to direct widget names
	if (!SlotImage)
	{
		const FName VarNames[] = {
			FName(*FString::Printf(TEXT("ImageItem%d"), SlotIndex)),
			FName(*FString::Printf(TEXT("Image Item %d"), SlotIndex)),
			FName(*FString::Printf(TEXT("Image_Item_%d"), SlotIndex)),
			FName(*FString::Printf(TEXT("ItemImage%d"), SlotIndex)),
			FName(*FString::Printf(TEXT("Item Image %d"), SlotIndex)),
			FName(*FString::Printf(TEXT("Slot%d"), SlotIndex)),
			FName(*FString::Printf(TEXT("ImageSlot%d"), SlotIndex))
		};

		for (const FName& VarName : VarNames)
		{
			SlotImage = Cast<UImage>(QuickSlotsWidget->GetWidgetFromName(VarName));
			if (SlotImage)
			{
				break;
			}
			if (FObjectProperty* Prop = CastField<FObjectProperty>(QuickSlotsWidget->GetClass()->FindPropertyByName(VarName)))
			{
				SlotImage = Cast<UImage>(Prop->GetObjectPropertyValue_InContainer(QuickSlotsWidget));
				if (SlotImage)
				{
					break;
				}
			}
		}
	}

	const bool bHasItem = (BlockID != 0 && ItemCount > 0);

	if (SlotImage)
	{
		if (bHasItem)
		{
			UTexture2D* Icon = AChunkActor::GetBlockIconTexture(BlockID);
			if (Icon)
			{
				SlotImage->SetBrushFromTexture(Icon, true);
				FSlateBrush Brush = SlotImage->GetBrush();
				Brush.SetResourceObject(Icon);
				Brush.TintColor = FSlateColor(FLinearColor::White);
				Brush.DrawAs = ESlateBrushDrawType::Image;
				SlotImage->SetBrush(Brush);
			}
			SlotImage->SetRenderOpacity(1.0f);
			SlotImage->SetColorAndOpacity(FLinearColor::White);
			SlotImage->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			SlotImage->SetRenderOpacity(0.0f);
			SlotImage->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, 0.0f));
			SlotImage->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	// Resolve the slot's count text widget (e.g. ItemCount1, Count1, TextCount1, etc.)
	UTextBlock* CountTextBlock = nullptr;
	const TArray<FName> CountWidgetNames = {
		FName(*FString::Printf(TEXT("ItemCount%d"), SlotIndex)),
		FName(*FString::Printf(TEXT("ItemCount_%d"), SlotIndex)),
		FName(*FString::Printf(TEXT("ItemCount %d"), SlotIndex)),
		FName(*FString::Printf(TEXT("Count%d"), SlotIndex)),
		FName(*FString::Printf(TEXT("Count_%d"), SlotIndex)),
		FName(*FString::Printf(TEXT("Count %d"), SlotIndex)),
		FName(*FString::Printf(TEXT("TextCount%d"), SlotIndex)),
		FName(*FString::Printf(TEXT("Text_Count%d"), SlotIndex)),
		FName(*FString::Printf(TEXT("TextItemCount%d"), SlotIndex)),
		FName(*FString::Printf(TEXT("Quantity%d"), SlotIndex)),
		FName(*FString::Printf(TEXT("Amount%d"), SlotIndex))
	};

	for (const FName& Name : CountWidgetNames)
	{
		CountTextBlock = Cast<UTextBlock>(QuickSlotsWidget->GetWidgetFromName(Name));
		if (CountTextBlock)
		{
			break;
		}
		if (FObjectProperty* TextProp = CastField<FObjectProperty>(QuickSlotsWidget->GetClass()->FindPropertyByName(Name)))
		{
			CountTextBlock = Cast<UTextBlock>(TextProp->GetObjectPropertyValue_InContainer(QuickSlotsWidget));
			if (CountTextBlock)
			{
				break;
			}
		}
	}

	if (CountTextBlock)
	{
		if (bHasItem)
		{
			CountTextBlock->SetText(FText::AsNumber(ItemCount));
			CountTextBlock->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			CountTextBlock->SetText(FText::GetEmpty());
			CountTextBlock->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	// Also update integer properties on the widget for BP binding support
	const FName IntPropNames[] = {
		FName(*FString::Printf(TEXT("Count%d"), SlotIndex)),
		FName(*FString::Printf(TEXT("ItemCount%d"), SlotIndex)),
		FName(*FString::Printf(TEXT("Amount%d"), SlotIndex))
	};
	for (const FName& PropName : IntPropNames)
	{
		if (FIntProperty* CountProp = CastField<FIntProperty>(QuickSlotsWidget->GetClass()->FindPropertyByName(PropName)))
		{
			CountProp->SetPropertyValue_InContainer(QuickSlotsWidget, bHasItem ? ItemCount : 0);
			break;
		}
	}

	// If this slot is currently selected in WB_QuickSlots, update the ItemName text
	const int32 SelectedSlotIndex0 = GetSelectedQuickSlotIndex(QuickSlotsWidget);
	if (SelectedSlotIndex0 == (SlotIndex - 1))
	{
		UTextBlock* NameTextBlock = Cast<UTextBlock>(QuickSlotsWidget->GetWidgetFromName(FName(TEXT("ItemName"))));
		if (!NameTextBlock)
		{
			if (FObjectProperty* TextProp = CastField<FObjectProperty>(QuickSlotsWidget->GetClass()->FindPropertyByName(FName(TEXT("ItemName")))))
			{
				NameTextBlock = Cast<UTextBlock>(TextProp->GetObjectPropertyValue_InContainer(QuickSlotsWidget));
			}
		}

		if (NameTextBlock)
		{
			if (bHasItem && !BlockName.IsNone())
			{
				NameTextBlock->SetText(FText::FromName(BlockName));
			}
			else
			{
				NameTextBlock->SetText(FText::GetEmpty());
			}
		}
	}
}

bool UQuickSlotsInventorySystem::TryAddItemToPlayerInventory(
	AActor* PlayerActor,
	uint8 BlockID,
	int32 Count,
	int32& OutRemainingCount
)
{
	OutRemainingCount = Count;

	if (!PlayerActor || BlockID == 0 || Count <= 0)
	{
		return false;
	}

	// Initialize global player slots if empty
	if (GPlayerQuickSlots.Num() != NUM_QUICK_SLOTS)
	{
		GPlayerQuickSlots.SetNum(NUM_QUICK_SLOTS);
		for (int32 i = 0; i < NUM_QUICK_SLOTS; ++i)
		{
			GPlayerQuickSlots[i].MaxStackSize = MAX_STACK_SIZE;
		}
	}

	// 1. Attempt to merge into existing slots containing the same BlockID with remaining capacity
	int32 TargetSlotIndex = -1; // 0-based
	for (int32 i = 0; i < NUM_QUICK_SLOTS; ++i)
	{
		if (GPlayerQuickSlots[i].BlockID == BlockID && GPlayerQuickSlots[i].ItemCount < GPlayerQuickSlots[i].MaxStackSize)
		{
			TargetSlotIndex = i;
			break;
		}
	}

	// 2. If no matching slot with space, find the first empty slot
	if (TargetSlotIndex < 0)
	{
		for (int32 i = 0; i < NUM_QUICK_SLOTS; ++i)
		{
			if (GPlayerQuickSlots[i].IsEmpty())
			{
				TargetSlotIndex = i;
				break;
			}
		}
	}

	// If all 9 slots are full to capacity (64 each), inventory cannot accept more items
	if (TargetSlotIndex < 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("QuickSlotsInventorySystem: Player quickslots are completely full!"));
		return false;
	}

	// Calculate how many items can be accepted into TargetSlotIndex
	FQuickSlotData& Slot = GPlayerQuickSlots[TargetSlotIndex];
	const int32 SpaceAvailable = Slot.MaxStackSize - Slot.ItemCount;
	const int32 AmountToAdd = FMath::Min(SpaceAvailable, Count);

	Slot.BlockID = BlockID;
	Slot.ItemCount += AmountToAdd;
	OutRemainingCount = Count - AmountToAdd;

	// Lookup block row and name from Block_DataTable
	FName BlockRowName = NAME_None;
	const uint8* FoundRowData = nullptr;

	UDataTable* BlockDataTable = LoadObject<UDataTable>(nullptr, TEXT("/Game/Data/Block_DataTable.Block_DataTable"));
	if (BlockDataTable)
	{
		const UScriptStruct* RowStruct = BlockDataTable->GetRowStruct();
		if (RowStruct)
		{
			for (auto It = BlockDataTable->GetRowMap().CreateConstIterator(); It; ++It)
			{
				const uint8* RowData = It.Value();
				if (!RowData) continue;

				for (TFieldIterator<FProperty> PropIt(RowStruct); PropIt; ++PropIt)
				{
					if (PropIt->GetName().Contains(TEXT("BlockID"), ESearchCase::IgnoreCase))
					{
						int32 RowBlockID = -1;
						if (FIntProperty* IP = CastField<FIntProperty>(*PropIt))
							RowBlockID = IP->GetPropertyValue_InContainer(RowData);
						else if (FByteProperty* BP = CastField<FByteProperty>(*PropIt))
							RowBlockID = BP->GetPropertyValue_InContainer(RowData);

						if (RowBlockID == static_cast<int32>(BlockID))
						{
							BlockRowName = It.Key();
							FoundRowData = RowData;
							break;
						}
					}
				}
				if (FoundRowData) break;
			}
		}
	}

	// Resolve active on-screen WB_QuickSlots widget via AC_Inventory / WB_HUD / Viewport
	UUserWidget* QuickSlotsWidget = ResolveQuickSlotsWidget(PlayerActor);
	if (QuickSlotsWidget)
	{
		// Refresh visual texture and name on the widget for TargetSlotIndex (1-based: 1..9)
		RefreshQuickSlotVisual(QuickSlotsWidget, TargetSlotIndex + 1, BlockID, Slot.ItemCount, BlockRowName);

		// Also invoke ProcessBlock custom event with S_Block_Data if available on WB_QuickSlots
		if (FoundRowData)
		{
			UFunction* ProcessFunc = QuickSlotsWidget->FindFunction(FName(TEXT("ProcessBlock")));
			if (ProcessFunc)
			{
				uint8* ParamsBuffer = (uint8*)FMemory_Alloca(ProcessFunc->ParmsSize);
				FMemory::Memzero(ParamsBuffer, ProcessFunc->ParmsSize);

				for (TFieldIterator<FProperty> PropIt(ProcessFunc); PropIt; ++PropIt)
				{
					if (FStructProperty* StructProp = CastField<FStructProperty>(*PropIt))
					{
						StructProp->CopyCompleteValue_InContainer(ParamsBuffer, FoundRowData);
						break;
					}
				}

				QuickSlotsWidget->ProcessEvent(ProcessFunc, ParamsBuffer);
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("QuickSlotsInventorySystem: Could not resolve WB_QuickSlots widget on player HUD."));
	}

	UE_LOG(LogTemp, Log, TEXT("QuickSlotsInventorySystem: Added %d x BlockID %d to QuickSlot %d (Total in slot: %d/64). Remaining: %d"),
		AmountToAdd, BlockID, TargetSlotIndex + 1, Slot.ItemCount, OutRemainingCount);

	return true;
}

bool UQuickSlotsInventorySystem::DropItemFromInventory(AActor* PlayerActor, int32 SlotIndexOverride)
{
	if (!PlayerActor)
	{
		return false;
	}

	UWorld* World = PlayerActor->GetWorld();
	if (!World)
	{
		return false;
	}

	// Ensure global player quickslots are initialized
	if (GPlayerQuickSlots.Num() != NUM_QUICK_SLOTS)
	{
		GPlayerQuickSlots.SetNum(NUM_QUICK_SLOTS);
		for (int32 i = 0; i < NUM_QUICK_SLOTS; ++i)
		{
			GPlayerQuickSlots[i].MaxStackSize = MAX_STACK_SIZE;
		}
	}

	UUserWidget* QuickSlotsWidget = ResolveQuickSlotsWidget(PlayerActor);

	int32 TargetSlotIndex = -1; // 0-based

	if (SlotIndexOverride >= 0 && SlotIndexOverride < NUM_QUICK_SLOTS)
	{
		TargetSlotIndex = SlotIndexOverride;
	}
	else if (IsInInventoryPanel(PlayerActor))
	{
		// In inventory panel: drop from hovered slot
		if (GHoveredInventorySlot >= 0 && GHoveredInventorySlot < NUM_QUICK_SLOTS)
		{
			TargetSlotIndex = GHoveredInventorySlot;
		}
		else if (QuickSlotsWidget)
		{
			// Check if mouse is hovering any of the 9 slot widgets
			for (int32 i = 0; i < NUM_QUICK_SLOTS; ++i)
			{
				const FName VarNames[] = {
					FName(*FString::Printf(TEXT("ImageItem%d"), i + 1)),
					FName(*FString::Printf(TEXT("Image Item %d"), i + 1)),
					FName(*FString::Printf(TEXT("Slot%d"), i + 1)),
					FName(*FString::Printf(TEXT("ItemSlot%d"), i + 1))
				};
				for (const FName& VarName : VarNames)
				{
					if (UWidget* SlotW = QuickSlotsWidget->GetWidgetFromName(VarName))
					{
						if (SlotW->IsHovered())
						{
							TargetSlotIndex = i;
							break;
						}
					}
				}
				if (TargetSlotIndex >= 0)
				{
					break;
				}
			}
		}

		if (TargetSlotIndex < 0)
		{
			UE_LOG(LogTemp, Verbose, TEXT("QuickSlotsInventorySystem: Drop attempted in inventory panel, but no slot is hovered."));
			return false;
		}
	}
	else
	{
		// In regular gameplay: drop from currently selected quickslot
		TargetSlotIndex = GetSelectedQuickSlotIndex(QuickSlotsWidget);
	}

	if (!GPlayerQuickSlots.IsValidIndex(TargetSlotIndex))
	{
		return false;
	}

	FQuickSlotData& Slot = GPlayerQuickSlots[TargetSlotIndex];
	if (Slot.IsEmpty() || Slot.ItemCount <= 0 || Slot.BlockID == 0)
	{
		UE_LOG(LogTemp, Verbose, TEXT("QuickSlotsInventorySystem: QuickSlot %d is empty, nothing to drop."), TargetSlotIndex + 1);
		return false;
	}

	const uint8 DroppedBlockID = Slot.BlockID;

	// Drop exactly 1 item
	Slot.ItemCount -= 1;
	if (Slot.ItemCount <= 0)
	{
		Slot.BlockID = 0;
		Slot.ItemCount = 0;
	}

	// Lookup block row name for UI refresh if needed
	FName BlockRowName = NAME_None;
	if (Slot.BlockID != 0)
	{
		UDataTable* BlockDataTable = LoadObject<UDataTable>(nullptr, TEXT("/Game/Data/Block_DataTable.Block_DataTable"));
		if (BlockDataTable)
		{
			const UScriptStruct* RowStruct = BlockDataTable->GetRowStruct();
			if (RowStruct)
			{
				for (auto It = BlockDataTable->GetRowMap().CreateConstIterator(); It; ++It)
				{
					const uint8* RowData = It.Value();
					if (!RowData) continue;

					for (TFieldIterator<FProperty> PropIt(RowStruct); PropIt; ++PropIt)
					{
						if (PropIt->GetName().Contains(TEXT("BlockID"), ESearchCase::IgnoreCase))
						{
							int32 RowBlockID = -1;
							if (FIntProperty* IP = CastField<FIntProperty>(*PropIt))
								RowBlockID = IP->GetPropertyValue_InContainer(RowData);
							else if (FByteProperty* BP = CastField<FByteProperty>(*PropIt))
								RowBlockID = BP->GetPropertyValue_InContainer(RowData);

							if (RowBlockID == static_cast<int32>(Slot.BlockID))
							{
								BlockRowName = It.Key();
								break;
							}
						}
					}
					if (!BlockRowName.IsNone()) break;
				}
			}
		}
	}

	// Refresh UI widget
	if (QuickSlotsWidget)
	{
		RefreshQuickSlotVisual(QuickSlotsWidget, TargetSlotIndex + 1, Slot.BlockID, Slot.ItemCount, BlockRowName);
	}

	// Compute launch trajectory:
	// 2.5 blocks = 250 units away
	FVector ViewLoc;
	FRotator ViewRot;

	APlayerController* PC = Cast<APlayerController>(PlayerActor->GetInstigatorController());
	if (!PC)
	{
		PC = World->GetFirstPlayerController();
	}

	if (PC && PC->PlayerCameraManager)
	{
		ViewLoc = PC->PlayerCameraManager->GetCameraLocation();
		ViewRot = PC->PlayerCameraManager->GetCameraRotation();
	}
	else if (APawn* Pawn = Cast<APawn>(PlayerActor))
	{
		ViewLoc = Pawn->GetPawnViewLocation();
		ViewRot = Pawn->GetViewRotation();
	}
	else
	{
		ViewLoc = PlayerActor->GetActorLocation() + FVector(0.0f, 0.0f, 60.0f);
		ViewRot = PlayerActor->GetActorRotation();
	}

	const FVector ForwardDir = ViewRot.Vector();
	const FVector SpawnLoc = ViewLoc + ForwardDir * 40.0f;

	FVector HorizontalDir = FVector(ForwardDir.X, ForwardDir.Y, 0.0f).GetSafeNormal();
	if (HorizontalDir.IsNearlyZero())
	{
		HorizontalDir = PlayerActor->GetActorForwardVector();
	}

	// Horizontal velocity 380 cm/s, upward velocity 170 cm/s
	// Over ~0.65s flight time lands ~250 cm (2.5 blocks) in front
	const FVector LaunchVel = HorizontalDir * 380.0f + FVector(0.0f, 0.0f, 170.0f);

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ABlockItemPickup* DroppedPickup = World->SpawnActor<ABlockItemPickup>(
		ABlockItemPickup::StaticClass(),
		SpawnLoc,
		ViewRot,
		SpawnParams
	);

	if (DroppedPickup)
	{
		DroppedPickup->InitializePickup(DroppedBlockID, 1);
		DroppedPickup->LaunchPickup(LaunchVel, 1.2f);
		UE_LOG(LogTemp, Log, TEXT("QuickSlotsInventorySystem: Dropped 1 of BlockID %d from slot %d (remaining: %d). Launched 2.5 blocks forward."),
			DroppedBlockID, TargetSlotIndex + 1, Slot.ItemCount);
	}

	return true;
}

void UQuickSlotsInventorySystem::SetHoveredInventorySlot(int32 SlotIndex, uint8 BlockID, UObject* SourceContainer)
{
	GHoveredInventorySlot = SlotIndex;
	GHoveredBlockID = BlockID;
	GHoveredSourceContainer = SourceContainer;
}

void UQuickSlotsInventorySystem::ClearHoveredInventorySlot()
{
	GHoveredInventorySlot = -1;
	GHoveredBlockID = 0;
	GHoveredSourceContainer = nullptr;
}

int32 UQuickSlotsInventorySystem::GetHoveredSlotIndex()
{
	return GHoveredInventorySlot;
}

bool UQuickSlotsInventorySystem::IsInInventoryPanel(AActor* PlayerActor)
{
	if (!PlayerActor)
	{
		return false;
	}

	UWorld* World = PlayerActor->GetWorld();
	if (!World)
	{
		return false;
	}

	APlayerController* PC = Cast<APlayerController>(PlayerActor->GetInstigatorController());
	if (!PC)
	{
		PC = World->GetFirstPlayerController();
	}

	if (PC && PC->bShowMouseCursor)
	{
		return true;
	}

	if (UBaublesSystem* Baubles = PlayerActor->FindComponentByClass<UBaublesSystem>())
	{
		if (Baubles->bIsBaublesOpen)
		{
			return true;
		}
	}

	// Check if any inventory / container widgets are active in viewport
	const TCHAR* ContainerKeywords[] = {
		TEXT("WB_Inventory"),
		TEXT("WB_Baubles"),
		TEXT("WB_Crafting"),
		TEXT("WB_Furnace"),
		TEXT("WB_Chest"),
		TEXT("WB_Barrel"),
		TEXT("WB_Crate"),
		TEXT("WB_Backpack")
	};

	for (TObjectIterator<UUserWidget> It; It; ++It)
	{
		UUserWidget* Widget = *It;
		if (!Widget || Widget->HasAnyFlags(RF_ClassDefaultObject) || Widget->GetWorld() != World)
		{
			continue;
		}

		if (Widget->IsInViewport())
		{
			const FString ClassName = Widget->GetClass()->GetName();
			for (const TCHAR* Kw : ContainerKeywords)
			{
				if (ClassName.Contains(Kw, ESearchCase::IgnoreCase))
				{
					return true;
				}
			}
		}
	}

	return false;
}

void UQuickSlotsInventorySystem::UpdateSlotOpacity(UWidget* SlotWidget, bool bHasItem)
{
	if (!SlotWidget)
	{
		return;
	}

	const float TargetOpacity = bHasItem ? 1.0f : 0.0f;
	SlotWidget->SetRenderOpacity(TargetOpacity);

	if (UImage* Img = Cast<UImage>(SlotWidget))
	{
		Img->SetColorAndOpacity(bHasItem ? FLinearColor::White : FLinearColor(1.0f, 1.0f, 1.0f, 0.0f));
		Img->SetVisibility(bHasItem ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
	else
	{
		SlotWidget->SetVisibility(bHasItem ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
}

void UQuickSlotsInventorySystem::UpdateAllQuickSlotOpacities(UUserWidget* QuickSlotsWidget)
{
	if (!QuickSlotsWidget)
	{
		return;
	}

	if (GPlayerQuickSlots.Num() != NUM_QUICK_SLOTS)
	{
		GPlayerQuickSlots.SetNum(NUM_QUICK_SLOTS);
		for (int32 i = 0; i < NUM_QUICK_SLOTS; ++i)
		{
			GPlayerQuickSlots[i].MaxStackSize = MAX_STACK_SIZE;
		}
	}

	for (int32 i = 0; i < NUM_QUICK_SLOTS; ++i)
	{
		const FQuickSlotData& Slot = GPlayerQuickSlots[i];
		RefreshQuickSlotVisual(QuickSlotsWidget, i + 1, Slot.BlockID, Slot.ItemCount, NAME_None);
	}
}

FQuickSlotData UQuickSlotsInventorySystem::GetQuickSlotData(int32 SlotIndex)
{
	if (GPlayerQuickSlots.IsValidIndex(SlotIndex))
	{
		return GPlayerQuickSlots[SlotIndex];
	}
	return FQuickSlotData();
}

void UQuickSlotsInventorySystem::SetQuickSlotData(int32 SlotIndex, const FQuickSlotData& InData)
{
	if (GPlayerQuickSlots.Num() != NUM_QUICK_SLOTS)
	{
		GPlayerQuickSlots.SetNum(NUM_QUICK_SLOTS);
		for (int32 i = 0; i < NUM_QUICK_SLOTS; ++i)
		{
			GPlayerQuickSlots[i].MaxStackSize = MAX_STACK_SIZE;
		}
	}

	if (GPlayerQuickSlots.IsValidIndex(SlotIndex))
	{
		GPlayerQuickSlots[SlotIndex] = InData;
	}
}

TArray<FQuickSlotData> UQuickSlotsInventorySystem::GetAllQuickSlots()
{
	if (GPlayerQuickSlots.Num() != NUM_QUICK_SLOTS)
	{
		GPlayerQuickSlots.SetNum(NUM_QUICK_SLOTS);
		for (int32 i = 0; i < NUM_QUICK_SLOTS; ++i)
		{
			GPlayerQuickSlots[i].MaxStackSize = MAX_STACK_SIZE;
		}
	}
	return GPlayerQuickSlots;
}

int32 UQuickSlotsInventorySystem::GetSelectedQuickSlotIndex(UUserWidget* QuickSlotsWidget)
{
	if (!QuickSlotsWidget)
	{
		return 0;
	}

	for (TFieldIterator<FProperty> PropIt(QuickSlotsWidget->GetClass()); PropIt; ++PropIt)
	{
		if (PropIt->GetName().Contains(TEXT("SelectedID"), ESearchCase::IgnoreCase))
		{
			int32 RawVal = 1;
			if (FIntProperty* IP = CastField<FIntProperty>(*PropIt))
			{
				RawVal = IP->GetPropertyValue_InContainer(QuickSlotsWidget);
			}
			else if (FByteProperty* BP = CastField<FByteProperty>(*PropIt))
			{
				RawVal = BP->GetPropertyValue_InContainer(QuickSlotsWidget);
			}
			// SelectedID is typically 1..9, so return 0..8
			return FMath::Clamp(RawVal - 1, 0, NUM_QUICK_SLOTS - 1);
		}
	}

	return 0;
}

