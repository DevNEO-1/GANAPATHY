// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/GanapatiWorldLevelHelper.h"
#include "WorldPartition/WorldPartition.h"
#include "WorldPartition/WorldPartitionRuntimeHash.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "UObject/UnrealType.h"

bool UGanapatiWorldLevelHelper::ConfigureWorldPartitionGrid(UWorld* World, int32 NewCellSize, float NewLoadingRange)
{
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GanapatiWorldLevelHelper] World is null!"));
		return false;
	}

	UWorldPartition* WP = World->GetWorldPartition();
	if (!WP && World->GetWorldSettings())
	{
		WP = World->GetWorldSettings()->GetWorldPartition();
	}

	if (!WP)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GanapatiWorldLevelHelper] WorldPartition is null on World %s!"), *World->GetName());
		return false;
	}

	if (!WP->RuntimeHash)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GanapatiWorldLevelHelper] WP->RuntimeHash is null!"));
		return false;
	}

	UObject* HashObj = WP->RuntimeHash;
	UE_LOG(LogTemp, Warning, TEXT("[GanapatiWorldLevelHelper] Configuring grid on RuntimeHash class: %s (%s)"),
		*HashObj->GetClass()->GetName(), *HashObj->GetName());

	// --- Case 1: UWorldPartitionRuntimeHashSet (UE 5.8 default) ---
	FArrayProperty* RuntimePartitionsProp = CastField<FArrayProperty>(HashObj->GetClass()->FindPropertyByName(TEXT("RuntimePartitions")));
	if (RuntimePartitionsProp)
	{
		FScriptArrayHelper ArrayHelper(RuntimePartitionsProp, RuntimePartitionsProp->ContainerPtrToValuePtr<void>(HashObj));
		FStructProperty* DescStructProp = CastField<FStructProperty>(RuntimePartitionsProp->Inner);

		if (DescStructProp && DescStructProp->Struct)
		{
			FObjectProperty* MainLayerProp = CastField<FObjectProperty>(DescStructProp->Struct->FindPropertyByName(TEXT("MainLayer")));
			FNameProperty* NameProp = CastField<FNameProperty>(DescStructProp->Struct->FindPropertyByName(TEXT("Name")));

			bool bAnyConfigured = false;
			for (int32 i = 0; i < ArrayHelper.Num(); ++i)
			{
				uint8* StructData = ArrayHelper.GetRawPtr(i);
				FName PartName = NameProp ? NameProp->GetPropertyValue_InContainer(StructData) : NAME_None;
				UObject* MainLayer = MainLayerProp ? MainLayerProp->GetObjectPropertyValue_InContainer(StructData) : nullptr;

				if (MainLayer)
				{
					FNumericProperty* CellSizeProp = CastField<FNumericProperty>(MainLayer->GetClass()->FindPropertyByName(TEXT("CellSize")));
					FNumericProperty* LoadingRangeProp = CastField<FNumericProperty>(MainLayer->GetClass()->FindPropertyByName(TEXT("LoadingRange")));

					if (CellSizeProp)
					{
						CellSizeProp->SetIntPropertyValue(CellSizeProp->ContainerPtrToValuePtr<void>(MainLayer), static_cast<int64>(NewCellSize));
					}
					if (LoadingRangeProp)
					{
						LoadingRangeProp->SetIntPropertyValue(LoadingRangeProp->ContainerPtrToValuePtr<void>(MainLayer), static_cast<int64>(NewLoadingRange));
					}

					MainLayer->MarkPackageDirty();
					bAnyConfigured = true;

					UE_LOG(LogTemp, Warning, TEXT("[GanapatiWorldLevelHelper] RuntimeHashSet partition '%s' MainLayer (%s): CellSize set to %d, LoadingRange set to %.1f"),
						*PartName.ToString(), *MainLayer->GetClass()->GetName(), NewCellSize, NewLoadingRange);
				}
			}

			if (bAnyConfigured)
			{
				HashObj->MarkPackageDirty();
				World->MarkPackageDirty();
				return true;
			}
		}
	}

	// --- Case 2: UWorldPartitionRuntimeSpatialHash ---
	FArrayProperty* GridsProp = CastField<FArrayProperty>(HashObj->GetClass()->FindPropertyByName(TEXT("Grids")));
	if (GridsProp)
	{
		FScriptArrayHelper ArrayHelper(GridsProp, GridsProp->ContainerPtrToValuePtr<void>(HashObj));
		if (ArrayHelper.Num() == 0)
		{
			ArrayHelper.AddValue();
		}

		FStructProperty* InnerProp = CastField<FStructProperty>(GridsProp->Inner);
		if (InnerProp && InnerProp->Struct)
		{
			FNameProperty* NameProp = CastField<FNameProperty>(InnerProp->Struct->FindPropertyByName(TEXT("GridName")));
			FNumericProperty* CellSizeProp = CastField<FNumericProperty>(InnerProp->Struct->FindPropertyByName(TEXT("CellSize")));
			FNumericProperty* LoadingRangeProp = CastField<FNumericProperty>(InnerProp->Struct->FindPropertyByName(TEXT("LoadingRange")));

			for (int32 i = 0; i < ArrayHelper.Num(); ++i)
			{
				uint8* StructData = ArrayHelper.GetRawPtr(i);
				FName GridName = NameProp ? NameProp->GetPropertyValue_InContainer(StructData) : NAME_None;

				if (GridName == TEXT("MainGrid") || GridName.IsNone() || i == 0)
				{
					if (NameProp && (GridName.IsNone() || GridName != TEXT("MainGrid")))
					{
						NameProp->SetPropertyValue_InContainer(StructData, TEXT("MainGrid"));
					}
					if (CellSizeProp)
					{
						CellSizeProp->SetIntPropertyValue(CellSizeProp->ContainerPtrToValuePtr<void>(StructData), static_cast<int64>(NewCellSize));
					}
					if (LoadingRangeProp)
					{
						LoadingRangeProp->SetIntPropertyValue(LoadingRangeProp->ContainerPtrToValuePtr<void>(StructData), static_cast<int64>(NewLoadingRange));
					}

					HashObj->MarkPackageDirty();
					World->MarkPackageDirty();
					UE_LOG(LogTemp, Warning, TEXT("[GanapatiWorldLevelHelper] RuntimeSpatialHash MainGrid: CellSize set to %d, LoadingRange set to %.1f"),
						NewCellSize, NewLoadingRange);
					return true;
				}
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[GanapatiWorldLevelHelper] Could not configure grid on %s!"), *HashObj->GetClass()->GetName());
	return false;
}

bool UGanapatiWorldLevelHelper::GetWorldPartitionGridSettings(UWorld* World, int32& OutCellSize, float& OutLoadingRange)
{
	OutCellSize = 0;
	OutLoadingRange = 0.0f;

	if (!World)
	{
		return false;
	}

	UWorldPartition* WP = World->GetWorldPartition();
	if (!WP && World->GetWorldSettings())
	{
		WP = World->GetWorldSettings()->GetWorldPartition();
	}

	if (!WP || !WP->RuntimeHash)
	{
		return false;
	}

	UObject* HashObj = WP->RuntimeHash;

	// --- Case 1: UWorldPartitionRuntimeHashSet ---
	FArrayProperty* RuntimePartitionsProp = CastField<FArrayProperty>(HashObj->GetClass()->FindPropertyByName(TEXT("RuntimePartitions")));
	if (RuntimePartitionsProp)
	{
		FScriptArrayHelper ArrayHelper(RuntimePartitionsProp, RuntimePartitionsProp->ContainerPtrToValuePtr<void>(HashObj));
		FStructProperty* DescStructProp = CastField<FStructProperty>(RuntimePartitionsProp->Inner);

		if (DescStructProp && DescStructProp->Struct)
		{
			FObjectProperty* MainLayerProp = CastField<FObjectProperty>(DescStructProp->Struct->FindPropertyByName(TEXT("MainLayer")));
			for (int32 i = 0; i < ArrayHelper.Num(); ++i)
			{
				uint8* StructData = ArrayHelper.GetRawPtr(i);
				UObject* MainLayer = MainLayerProp ? MainLayerProp->GetObjectPropertyValue_InContainer(StructData) : nullptr;
				if (MainLayer)
				{
					FNumericProperty* CellSizeProp = CastField<FNumericProperty>(MainLayer->GetClass()->FindPropertyByName(TEXT("CellSize")));
					FNumericProperty* LoadingRangeProp = CastField<FNumericProperty>(MainLayer->GetClass()->FindPropertyByName(TEXT("LoadingRange")));

					if (CellSizeProp)
					{
						OutCellSize = static_cast<int32>(CellSizeProp->GetSignedIntPropertyValue(CellSizeProp->ContainerPtrToValuePtr<void>(MainLayer)));
					}
					if (LoadingRangeProp)
					{
						OutLoadingRange = static_cast<float>(LoadingRangeProp->GetSignedIntPropertyValue(LoadingRangeProp->ContainerPtrToValuePtr<void>(MainLayer)));
					}
					return true;
				}
			}
		}
	}

	// --- Case 2: UWorldPartitionRuntimeSpatialHash ---
	FArrayProperty* GridsProp = CastField<FArrayProperty>(HashObj->GetClass()->FindPropertyByName(TEXT("Grids")));
	if (GridsProp)
	{
		FScriptArrayHelper ArrayHelper(GridsProp, GridsProp->ContainerPtrToValuePtr<void>(HashObj));
		FStructProperty* InnerProp = CastField<FStructProperty>(GridsProp->Inner);
		if (InnerProp && InnerProp->Struct)
		{
			FNameProperty* NameProp = CastField<FNameProperty>(InnerProp->Struct->FindPropertyByName(TEXT("GridName")));
			FNumericProperty* CellSizeProp = CastField<FNumericProperty>(InnerProp->Struct->FindPropertyByName(TEXT("CellSize")));
			FNumericProperty* LoadingRangeProp = CastField<FNumericProperty>(InnerProp->Struct->FindPropertyByName(TEXT("LoadingRange")));

			for (int32 i = 0; i < ArrayHelper.Num(); ++i)
			{
				uint8* StructData = ArrayHelper.GetRawPtr(i);
				FName GridName = NameProp ? NameProp->GetPropertyValue_InContainer(StructData) : NAME_None;
				if (GridName == TEXT("MainGrid") || i == 0)
				{
					if (CellSizeProp)
					{
						OutCellSize = static_cast<int32>(CellSizeProp->GetSignedIntPropertyValue(CellSizeProp->ContainerPtrToValuePtr<void>(StructData)));
					}
					if (LoadingRangeProp)
					{
						OutLoadingRange = static_cast<float>(LoadingRangeProp->GetSignedIntPropertyValue(LoadingRangeProp->ContainerPtrToValuePtr<void>(StructData)));
					}
					return true;
				}
			}
		}
	}

	return false;
}

bool UGanapatiWorldLevelHelper::IsPartitionedWorld(UWorld* World)
{
	if (!World)
	{
		return false;
	}

	if (World->IsPartitionedWorld())
	{
		return true;
	}

	if (World->GetWorldSettings() && World->GetWorldSettings()->GetWorldPartition())
	{
		return true;
	}

	return false;
}
