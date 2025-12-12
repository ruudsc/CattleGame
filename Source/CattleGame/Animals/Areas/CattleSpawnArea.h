// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CattleSpawnArea.generated.h"

class ACattleAnimal;
class USplineComponent;
class UBoxComponent;
class UBillboardComponent;

/**
 * FCattleSpawnItem
 *
 * Configuration for a single type of cattle to spawn in the area.
 */
USTRUCT(BlueprintType)
struct CATTLEGAME_API FCattleSpawnItem
{
    GENERATED_BODY()

    /** Blueprint class of the cattle animal to spawn (must be subclass of ACattleAnimal) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
    TSubclassOf<ACattleAnimal> ActorBlueprint;

    /** Number of this animal type to spawn */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn", meta = (ClampMin = "1"))
    int32 SpawnCount = 1;

    FCattleSpawnItem()
        : ActorBlueprint(nullptr), SpawnCount(1)
    {
    }
};

/**
 * ACattleSpawnArea
 *
 * Level-placed actor that defines a spawn area for cattle animals.
 * Animals are distributed evenly within the area on BeginPlay.
 *
 * Supports both box and spline-based area shapes.
 * Search "CattleSpawn" in the actor panel to find this actor.
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Cattle Spawn Area"))
class CATTLEGAME_API ACattleSpawnArea : public AActor
{
    GENERATED_BODY()

public:
    ACattleSpawnArea();

    // ===== Spawn Configuration =====

    /** Array of spawn items defining what cattle to spawn and how many */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cattle Spawn")
    TArray<FCattleSpawnItem> SpawnItems;

    /** Whether to spawn animals on BeginPlay */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cattle Spawn")
    bool bSpawnOnBeginPlay = true;

    /** Whether to use the spline component for area shape (false = use box) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cattle Spawn|Shape")
    bool bUseSplineShape = false;

    /** Height range for spawn location randomization */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cattle Spawn|Shape")
    float SpawnHeightOffset = 50.0f;

    /** Minimum distance between spawned animals */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cattle Spawn|Distribution", meta = (ClampMin = "0.0"))
    float MinSpawnDistance = 200.0f;

    /** Maximum attempts to find a valid spawn location per animal */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cattle Spawn|Distribution", meta = (ClampMin = "1"))
    int32 MaxSpawnAttempts = 50;

    // ===== Spawn Functions =====

    /** Spawn all configured animals in the area */
    UFUNCTION(BlueprintCallable, Category = "Cattle Spawn")
    void SpawnAllAnimals();

    /** Spawn a specific number of animals from a spawn item */
    UFUNCTION(BlueprintCallable, Category = "Cattle Spawn")
    TArray<ACattleAnimal *> SpawnAnimalsOfType(const FCattleSpawnItem &SpawnItem);

    /** Get a random spawn location within the area */
    UFUNCTION(BlueprintCallable, Category = "Cattle Spawn")
    FVector GetRandomSpawnLocation() const;

    /** Check if a location is valid for spawning (inside area) */
    UFUNCTION(BlueprintCallable, Category = "Cattle Spawn")
    bool IsValidSpawnLocation(const FVector &Location) const;

    /** Get all spawned animals from this area (C++ only due to TWeakObjectPtr) */
    const TArray<TWeakObjectPtr<ACattleAnimal>> &GetSpawnedAnimals() const { return SpawnedAnimals; }

    /** Get all spawned animals as regular array (Blueprint accessible) */
    UFUNCTION(BlueprintCallable, Category = "Cattle Spawn")
    TArray<ACattleAnimal *> GetSpawnedAnimalsArray() const;

    /** Get the total number of animals to spawn */
    UFUNCTION(BlueprintCallable, Category = "Cattle Spawn")
    int32 GetTotalSpawnCount() const;

    // ===== Debug =====

    /** Draw debug visualization for the spawn area */
    UFUNCTION(BlueprintCallable, Category = "Cattle Spawn|Debug")
    void DrawDebugSpawnArea(float Duration = 0.0f) const;

    /** Color used for debug visualization */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cattle Spawn|Debug")
    FColor DebugColor = FColor::Yellow;

    /** Whether to show debug in editor */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cattle Spawn|Debug")
    bool bShowDebugInEditor = true;

protected:
    virtual void BeginPlay() override;
    virtual void OnConstruction(const FTransform &Transform) override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent) override;
#endif

    // ===== Components =====

    /** Root scene component */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USceneComponent> SceneRoot;

    /** Spline component for freeform area shapes */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USplineComponent> SplineComponent;

    /** Box component for simple rectangular areas */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UBoxComponent> BoxComponent;

#if WITH_EDITORONLY_DATA
    /** Billboard for editor visibility */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UBillboardComponent> BillboardComponent;
#endif

    // ===== Spawned Animals =====

    /** Weak references to all animals spawned by this area */
    UPROPERTY(Transient)
    TArray<TWeakObjectPtr<ACattleAnimal>> SpawnedAnimals;

    /** Locations already used for spawning (to maintain minimum distance) */
    TArray<FVector> UsedSpawnLocations;

    // ===== Helper Functions =====

    /** Check if point is inside spline area */
    bool IsInsideSplineArea(const FVector &Location) const;

    /** Check if point is inside box area */
    bool IsInsideBoxArea(const FVector &Location) const;

    /** Get a random point inside the box */
    FVector GetRandomPointInBox() const;

    /** Get a random point inside the spline area */
    FVector GetRandomPointInSpline() const;

    /** Check if location is far enough from all used locations */
    bool IsLocationFarEnoughFromOthers(const FVector &Location) const;

    /** Generate evenly distributed points within the area */
    TArray<FVector> GenerateEvenlyDistributedPoints(int32 Count) const;

    /** Update component visibility based on shape mode */
    void UpdateShapeVisibility();
};
