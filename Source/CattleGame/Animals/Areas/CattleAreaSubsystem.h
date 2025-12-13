// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CattleAreaSubsystem.generated.h"

class ACattleAreaBase;
class ACattleFlowGuide;
class ACattleAnimal;

/**
 * ECattleAreaType
 *
 * Types of cattle behavior areas, ordered by priority (higher = more important)
 */
UENUM(BlueprintType)
enum class ECattleAreaType : uint8
{
    None = 0,
    Graze = 10,     // Lowest priority - default grazing behavior
    FlowGuide = 20, // Guide animals toward a direction
    Avoid = 30,     // Animals should avoid this area
    Panic = 40      // Highest priority - animals flee from this area
};

/**
 * FCattleAreaInfluence
 *
 * Struct containing area influence data for an animal
 */
USTRUCT(BlueprintType)
struct CATTLEGAME_API FCattleAreaInfluence
{
    GENERATED_BODY()

    /** The type of area influence */
    UPROPERTY(BlueprintReadOnly, Category = "Cattle Area")
    ECattleAreaType AreaType = ECattleAreaType::None;

    /** The area actor providing this influence */
    UPROPERTY(BlueprintReadOnly, Category = "Cattle Area")
    TWeakObjectPtr<ACattleAreaBase> AreaActor;

    /** Direction the area wants the animal to move */
    UPROPERTY(BlueprintReadOnly, Category = "Cattle Area")
    FVector InfluenceDirection = FVector::ZeroVector;

    /** Speed modifier from this area */
    UPROPERTY(BlueprintReadOnly, Category = "Cattle Area")
    float SpeedModifier = 1.0f;

    /** Strength of the influence (0-1) based on distance from center/edge */
    UPROPERTY(BlueprintReadOnly, Category = "Cattle Area")
    float Strength = 0.0f;

    /** Priority of this area (for overlapping areas) */
    UPROPERTY(BlueprintReadOnly, Category = "Cattle Area")
    int32 Priority = 0;

    bool IsValid() const { return AreaType != ECattleAreaType::None && AreaActor.IsValid(); }
};

/**
 * UCattleAreaSubsystem
 *
 * World subsystem that manages all cattle behavior areas and provides
 * efficient spatial queries for animals to determine their current influences.
 */
UCLASS()
class CATTLEGAME_API UCattleAreaSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // ===== Subsystem Lifecycle =====

    virtual void Initialize(FSubsystemCollectionBase &Collection) override;
    virtual void Deinitialize() override;
    virtual bool ShouldCreateSubsystem(UObject *Outer) const override;

    // ===== Area Registration =====

    /** Register an area with the subsystem */
    UFUNCTION(BlueprintCallable, Category = "Cattle Area")
    void RegisterArea(ACattleAreaBase *Area);

    /** Unregister an area from the subsystem */
    UFUNCTION(BlueprintCallable, Category = "Cattle Area")
    void UnregisterArea(ACattleAreaBase *Area);

    /** Register a flow guide with the subsystem */
    UFUNCTION(BlueprintCallable, Category = "Cattle Area")
    void RegisterFlowGuide(ACattleFlowGuide *FlowGuide);

    /** Unregister a flow guide from the subsystem */
    UFUNCTION(BlueprintCallable, Category = "Cattle Area")
    void UnregisterFlowGuide(ACattleFlowGuide *FlowGuide);

    // ===== Area Queries =====

    /** Get all areas containing a world location */
    UFUNCTION(BlueprintCallable, Category = "Cattle Area")
    TArray<FCattleAreaInfluence> GetAreasAtLocation(const FVector &Location) const;

    /** Get the highest priority area influence at a location */
    UFUNCTION(BlueprintCallable, Category = "Cattle Area")
    FCattleAreaInfluence GetPrimaryAreaAtLocation(const FVector &Location) const;

    /** Get flow direction at a world location (samples all flow guides) */
    UFUNCTION(BlueprintCallable, Category = "Cattle Area")
    FVector GetFlowDirectionAtLocation(const FVector &Location) const;

    /** Check if a location is inside any area of the specified type */
    UFUNCTION(BlueprintCallable, Category = "Cattle Area")
    bool IsLocationInAreaType(const FVector &Location, ECattleAreaType AreaType) const;

    /** Get all registered areas of a specific type */
    UFUNCTION(BlueprintCallable, Category = "Cattle Area")
    TArray<ACattleAreaBase *> GetAreasOfType(ECattleAreaType AreaType) const;

    // ===== Debug =====

    /** Draw debug visualization for all areas */
    UFUNCTION(BlueprintCallable, Category = "Cattle Area|Debug")
    void DrawDebugAreas(float Duration = 0.0f) const;

protected:
    /** All registered behavior areas */
    UPROPERTY()
    TArray<TWeakObjectPtr<ACattleAreaBase>> RegisteredAreas;

    /** All registered flow guides */
    UPROPERTY()
    TArray<TWeakObjectPtr<ACattleFlowGuide>> RegisteredFlowGuides;

    /** Clean up invalid (destroyed) area references */
    void CleanupInvalidReferences();
};
