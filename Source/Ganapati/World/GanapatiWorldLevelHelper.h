// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GanapatiWorldLevelHelper.generated.h"

class UWorld;

/**
 * Helper library for World Partition configuration and validation in Ganapati open world.
 */
UCLASS()
class GANAPATI_API UGanapatiWorldLevelHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Configures the MainGrid runtime spatial hash parameters on a World Partition level.
	 * @param World The world to configure.
	 * @param NewCellSize Main grid cell size in cm (e.g. 25600 for 256m).
	 * @param NewLoadingRange Main grid loading range in cm (e.g. 51200 for 512m).
	 * @return True if configuration succeeded.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ganapati|WorldPartition")
	static bool ConfigureWorldPartitionGrid(UWorld* World, int32 NewCellSize = 25600, float NewLoadingRange = 51200.0f);

	/**
	 * Retrieves the current MainGrid runtime spatial hash parameters from a World Partition level.
	 * @param World The world to inspect.
	 * @param OutCellSize Output cell size in cm.
	 * @param OutLoadingRange Output loading range in cm.
	 * @return True if retrieval succeeded.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ganapati|WorldPartition")
	static bool GetWorldPartitionGridSettings(UWorld* World, int32& OutCellSize, float& OutLoadingRange);

	/**
	 * Checks if the given world is a World Partition enabled world.
	 * @param World The world to inspect.
	 * @return True if partitioned.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ganapati|WorldPartition")
	static bool IsPartitionedWorld(UWorld* World);
};
