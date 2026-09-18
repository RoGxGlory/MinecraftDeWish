#include "QuickSlotsInventorySystem.h"
#include "ChunkActor.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "UObject/UObjectIterator.h"

// Static inventory tracking for the player's 9 quickslots
static TArray<FQuickSlotData> GPlayerQuickSlots;

UQuickSlotsInventorySystem::UQuickSlotsInventorySystem()
{
	QuickSlots.SetNum(NUM_QUICK_SLOTS);
	for (int32 i = 0; i < NUM_QUICK_SLOTS; ++i)
	{
		QuickSlots[i].MaxStackSize = MAX_STACK_SIZE;
	}
}

FQuickSlotData UQuickSlotsInventorySystem::GetSlotData(int32 SlotIndex) const
{
	if (QuickSlots.IsValidIndex(SlotIndex))
	{
		return QuickSlots[SlotIndex];
	}
	return FQuickSlotData();
}

void UQuickSlotsInventorySystem::SetSlotData(int32 SlotIndex, const FQuickSlotData& InData)
{
	if (QuickSlots.IsValidIndex(SlotIndex))
	{
		QuickSlots[SlotIndex] = InData;
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

	const FName VarName(*FString::Printf(TEXT("ImageItem%d"), SlotIndex));
	const FName DisplayName(*FString::Printf(TEXT("Image Item %d"), SlotIndex));

	SlotImage = Cast<UImage>(QuickSlotsWidget->GetWidgetFromName(VarName));
	if (!SlotImage)
	{
		SlotImage = Cast<UImage>(QuickSlotsWidget->GetWidgetFromName(DisplayName));
	}
	if (!SlotImage)
	{
		if (FObjectProperty* Prop = CastField<FObjectProperty>(QuickSlotsWidget->GetClass()->FindPropertyByName(VarName)))
		{
			SlotImage = Cast<UImage>(Prop->GetObjectPropertyValue_InContainer(QuickSlotsWidget));
		}
	}

	if (SlotImage)
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

			SlotImage->SetVisibility(ESlateVisibility::Visible);
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
		if (ItemCount > 0)
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
			CountProp->SetPropertyValue_InContainer(QuickSlotsWidget, ItemCount);
			break;
		}
	}

	// If this slot is currently selected in WB_QuickSlots, update the ItemName text
	int32 SelectedSlotID = 1;
	for (TFieldIterator<FProperty> PropIt(QuickSlotsWidget->GetClass()); PropIt; ++PropIt)
	{
		if (PropIt->GetName().Contains(TEXT("SelectedID"), ESearchCase::IgnoreCase))
		{
			if (FIntProperty* IP = CastField<FIntProperty>(*PropIt))
			{
				SelectedSlotID = IP->GetPropertyValue_InContainer(QuickSlotsWidget);
			}
			else if (FByteProperty* BP = CastField<FByteProperty>(*PropIt))
			{
				SelectedSlotID = BP->GetPropertyValue_InContainer(QuickSlotsWidget);
			}
			break;
		}
	}

	if (SelectedSlotID == SlotIndex)
	{
		UTextBlock* NameTextBlock = Cast<UTextBlock>(QuickSlotsWidget->GetWidgetFromName(FName(TEXT("ItemName"))));
		if (!NameTextBlock)
		{
			if (FObjectProperty* TextProp = CastField<FObjectProperty>(QuickSlotsWidget->GetClass()->FindPropertyByName(FName(TEXT("ItemName")))))
			{
				NameTextBlock = Cast<UTextBlock>(TextProp->GetObjectPropertyValue_InContainer(QuickSlotsWidget));
			}
		}

		if (NameTextBlock && !BlockName.IsNone())
		{
			NameTextBlock->SetText(FText::FromName(BlockName));
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
