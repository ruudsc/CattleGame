// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CattleAreaSubsystem.h"
#include "CattleAreaBase.generated.h"

class USplineComponent;
class UBoxComponent;

/**
 * ACattleAreaBase
 *
 * Base class for all cattle behavior area actors.
 * Supports both spline-based (freeform) and box-based (simple) area definitions.
 *
 * Searchable in editor as "Cattle" prefix for easy discovery.
 */
UCLASS(Abstract, Blueprintable, BlueprintType)
class CATTLEGAME_API ACattleAreaBase : public AActor
{
    GENERATED_BODY()

public:
    ACattleAreaBase();

    // ===== Area Configuration =====

    /** The type of behavior this area provides */
    UFUNCTION(BlueprintCallable, Category = "Cattle Area")
    virtual ECattleAreaType GetAreaType() const PURE_VIRTUAL(ACattleAreaBase::GetAreaType, return ECattleAreaType::None;);

    /** Priority when overlapping with other areas (higher = more important) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cattle Area")
    int32 Priority = 0;

    /** Whether to use the spline component for area shape (false = use box) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cattle Area|Shape")
    bool bUseSplineShape = false;

    /** Height of the area volume */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cattle Area|Shape", meta = (ClampMin = "100.0"))
    float AreaHeight = 500.0f;

    /** Falloff distance at edges where influence fades */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cattle Area|Shape", meta = (ClampMin = "0.0"))
    float EdgeFalloff = 200.0f;

    // ===== Area Queries =====

    /** Check if a world location is inside this area */
    UFUNCTION(BlueprintCallable, Category = "Cattle Area")
    virtual bool IsLocationInArea(const FVector &Location) const;

    /** Get the influence this area has at a specific location */
    UFUNCTION(BlueprintCallable, Category = "Cattle Area")
    virtual FCattleAreaInfluence GetInfluenceAtLocation(const FVector &Location) const;

    /** Get the movement direction this area wants to impart at a location */
    UFUNCTION(BlueprintCallable, Category = "Cattle Area")
    virtual FVector GetInfluenceDirection(const FVector &Location) const;

    /** Get the speed modifier this area applies */
    UFUNCTION(BlueprintCallable, Category = "Cattle Area")
    virtual float GetSpeedModifier() const { return 1.0f; }

    // ===== Debug =====

    /** Draw debug visualization for this area */
    UFUNCTION(BlueprintCallable, Category = "Cattle Area|Debug")
    virtual void DrawDebugArea(float Duration = 0.0f) const;

    /** Color used for debug visualization */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cattle Area|Debug")
    FColor DebugColor = FColor::White;

    /** Whether to show debug in editor */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cattle Area|Debug")
    bool bShowDebugInEditor = true;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

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

    // ===== Helper Functions =====

    /** Get distance from location to area boundary (negative = inside) */
    float GetDistanceToBoundary(const FVector &Location) const;

    /** Calculate influence strength based on distance to boundary */
    float CalculateInfluenceStrength(float DistanceToBoundary) const;

    /** Check if point is inside spline area */
    bool IsInsideSplineArea(const FVector &Location) const;

    /** Get the area subsystem */
    UCattleAreaSubsystem *GetAreaSubsystem() const;

    /** Register with area subsystem */
    void RegisterWithSubsystem();

    /** Unregister from area subsystem */
    void UnregisterFromSubsystem();
};
