#pragma once

#include "CoreMinimal.h"
#include "CattleFlowActorBase.h"
#include "CattleGuideSpline.generated.h"

class USplineComponent;
class ACattleAnimal;

/**
 * Spline-based guide that directs cattle along a path.
 * Uses proximity-based registration via UCattleFlowSubsystem - no overlap trigger required.
 *
 * Place in the level and edit the spline to define the cattle path.
 * Cattle within InfluenceRadius of the spline will be guided along it.
 */
UCLASS(Blueprintable)
class CATTLEGAME_API ACattleGuideSpline : public ACattleFlowActorBase
{
    GENERATED_BODY()

public:
    ACattleGuideSpline();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaTime) override;

    // Override to return true - splines use proximity registration
    virtual bool IsProximityBased_Implementation() const override { return true; }

    // Flow calculation - guides along spline tangent
    virtual FVector CalculateFlowDirectionInternal(const FVector &Location) const override;

    // Override GetFlowDirection to calculate distance from spline, not actor center
    virtual FVector GetFlowDirection_Implementation(const FVector &Location, float &OutWeight) const override;

    // Debug visualization override
    virtual void DrawDebugVisualization() const override;

    // GAS effect management for proximity-based cattle
    void UpdateProximityCattle();
    void HandleCattleEnterProximity(ACattleAnimal *Cattle);
    void HandleCattleExitProximity(ACattleAnimal *Cattle);

public:
    /** Spline component defining the cattle path */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USplineComponent> GuideSpline;

    /**
     * If true, also pulls cattle towards the spline (not just along it).
     * Useful for keeping cattle centered on the path.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cattle Flow|Guide")
    bool bPullTowardsSpline = true;

    /**
     * Strength of the pull towards the spline center (if bPullTowardsSpline is true).
     * 0 = no pull, 1 = equal pull and flow along spline.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cattle Flow|Guide", meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bPullTowardsSpline"))
    float PullStrength = 0.3f;

    /**
     * How often to check for cattle entering/exiting proximity (in seconds).
     * Lower values = more responsive but more expensive.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cattle Flow|Guide", meta = (ClampMin = "0.05"))
    float ProximityCheckInterval = 0.25f;

protected:
    /** Cattle currently within influence proximity (for GAS effect tracking) */
    UPROPERTY(Transient)
    TSet<TWeakObjectPtr<ACattleAnimal>> CattleInProximity;

    /** Track active GE handles per cattle actor */
    TMap<TWeakObjectPtr<AActor>, FActiveGameplayEffectHandle> ActiveEffectHandles;

    /** Timer for proximity checks */
    float ProximityCheckTimer = 0.0f;

public:
    /**
     * Get the closest point on the spline to a world location.
     * @param WorldLocation Location to query.
     * @param OutClosestPoint Output: closest point on spline.
     * @param OutDistance Output: distance from WorldLocation to closest point.
     * @return The input key (parameter) at the closest point.
     */
    UFUNCTION(BlueprintCallable, Category = "Cattle Flow|Guide")
    float GetClosestPointOnSpline(const FVector &WorldLocation, FVector &OutClosestPoint, float &OutDistance) const;
};
