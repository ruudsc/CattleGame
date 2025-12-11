#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayEffect.h"
#include "ICattleFlowSource.generated.h"

class UCurveFloat;

/**
 * Interface for any actor that can influence cattle movement flow.
 * Implemented by both volume-based actors (overlap triggers) and spline-based actors (proximity).
 */
UINTERFACE(MinimalAPI, Blueprintable)
class UCattleFlowSource : public UInterface
{
    GENERATED_BODY()
};

class CATTLEGAME_API ICattleFlowSource
{
    GENERATED_BODY()

public:
    /**
     * Get the desired flow direction at a specific world location.
     * @param Location World location to sample.
     * @param OutWeight Output weight/strength of this flow source at the given location (0-1 based on falloff).
     * @return Normalized direction vector.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Cattle Flow")
    FVector GetFlowDirection(const FVector &Location, float &OutWeight) const;
    virtual FVector GetFlowDirection_Implementation(const FVector &Location, float &OutWeight) const
    {
        OutWeight = 1.0f;
        return FVector::ZeroVector;
    }

    /**
     * Get the Gameplay Effect class to apply when cattle enters this flow source's influence.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Cattle Flow")
    TSubclassOf<UGameplayEffect> GetApplyEffectClass() const;
    virtual TSubclassOf<UGameplayEffect> GetApplyEffectClass_Implementation() const { return nullptr; }

    /**
     * Get the influence radius for proximity-based sources (splines).
     * For volume-based sources, this may return 0 (uses collision shape instead).
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Cattle Flow")
    float GetInfluenceRadius() const;
    virtual float GetInfluenceRadius_Implementation() const { return 0.0f; }

    /**
     * Get the falloff curve for influence weight based on distance.
     * X axis = normalized distance (0 = at source, 1 = at influence radius edge)
     * Y axis = weight multiplier (typically 1 at center, 0 at edge)
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Cattle Flow")
    UCurveFloat *GetFalloffCurve() const;
    virtual UCurveFloat *GetFalloffCurve_Implementation() const { return nullptr; }

    /**
     * Get the priority of this flow source. Higher priority sources take precedence.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Cattle Flow")
    int32 GetFlowPriority() const;
    virtual int32 GetFlowPriority_Implementation() const { return 0; }

    /**
     * Whether this source uses proximity-based registration (requires subsystem polling)
     * vs overlap-based registration (direct collision events).
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Cattle Flow")
    bool IsProximityBased() const;
    virtual bool IsProximityBased_Implementation() const { return false; }

    /**
     * Get the actor implementing this interface.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Cattle Flow")
    AActor *GetFlowSourceActor() const;
    virtual AActor *GetFlowSourceActor_Implementation() const { return nullptr; }
};
