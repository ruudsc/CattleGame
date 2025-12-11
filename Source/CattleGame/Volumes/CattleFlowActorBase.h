#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ICattleFlowSource.h"
#include "CattleFlowActorBase.generated.h"

class UCurveFloat;
class UGameplayEffect;

/**
 * Base class for all Cattle Flow Source actors.
 * Provides common functionality for influence radius, falloff curves,
 * gameplay effects, and debug visualization.
 *
 * Derived classes implement specific flow behaviors (avoid, graze, guide).
 */
UCLASS(Abstract, Blueprintable)
class CATTLEGAME_API ACattleFlowActorBase : public AActor, public ICattleFlowSource
{
    GENERATED_BODY()

public:
    ACattleFlowActorBase();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaTime) override;

public:
    // ===== ICattleFlowSource Interface =====

    virtual FVector GetFlowDirection_Implementation(const FVector &Location, float &OutWeight) const override;
    virtual TSubclassOf<UGameplayEffect> GetApplyEffectClass_Implementation() const override;
    virtual float GetInfluenceRadius_Implementation() const override;
    virtual UCurveFloat *GetFalloffCurve_Implementation() const override;
    virtual int32 GetFlowPriority_Implementation() const override;
    virtual bool IsProximityBased_Implementation() const override;
    virtual AActor *GetFlowSourceActor_Implementation() const override;

    // ===== Configuration =====

    /** Gameplay Effect to apply when cattle enters this flow source's influence */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cattle Flow|Effects")
    TSubclassOf<UGameplayEffect> ApplyEffectClass;

    /**
     * Radius of influence for proximity-based sources.
     * For volume-based sources, this supplements the collision shape.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cattle Flow|Influence", meta = (ClampMin = "0.0"))
    float InfluenceRadius = 500.0f;

    /**
     * Falloff curve for influence weight based on distance.
     * X = normalized distance (0 = at source center, 1 = at influence radius)
     * Y = weight multiplier (typically 1.0 at center, 0.0 at edge)
     * If null, uses linear falloff.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cattle Flow|Influence")
    TObjectPtr<UCurveFloat> FalloffCurve;

    /** Priority for this flow source. Higher values take precedence when blending. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cattle Flow|Influence")
    int32 Priority = 0;

    /** Whether to show debug visualization (flow direction arrows, influence radius) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cattle Flow|Debug")
    bool bShowDebugFlow = false;

    /** Color for debug visualization */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cattle Flow|Debug")
    FColor DebugColor = FColor::Cyan;

protected:
    /**
     * Override in derived classes to calculate the actual flow direction.
     * Base implementation returns zero vector.
     * @param Location World location to sample.
     * @return Normalized flow direction.
     */
    virtual FVector CalculateFlowDirectionInternal(const FVector &Location) const;

    /**
     * Calculate the weight/strength at a given distance using the falloff curve.
     * @param Distance Distance from the source.
     * @return Weight value (0-1).
     */
    float CalculateFalloffWeight(float Distance) const;

    /**
     * Draw debug visualization. Called in Tick when bShowDebugFlow is true.
     */
    virtual void DrawDebugVisualization() const;

    /** Root scene component */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USceneComponent> RootSceneComponent;
};
