#pragma once

#include "CoreMinimal.h"
#include "CattleFlowActorBase.h"
#include "CattleGrazeVolume.generated.h"

class UBoxComponent;
class ACattleAnimal;

/**
 * Volume that marks a grazing area for cattle.
 * Does not provide flow direction - cattle can move freely within.
 * Primarily used for applying Gameplay Effects (e.g., GE_Cattle_GrazingState).
 *
 * Place in the level and scale the box trigger to define the grazing zone.
 */
UCLASS(Blueprintable)
class CATTLEGAME_API ACattleGrazeVolume : public ACattleFlowActorBase
{
    GENERATED_BODY()

public:
    ACattleGrazeVolume();

protected:
    virtual void BeginPlay() override;

    // Flow calculation - no flow, cattle move freely in grazing areas
    virtual FVector CalculateFlowDirectionInternal(const FVector &Location) const override;

    // Overlap handlers for GAS effect application
    UFUNCTION()
    void OnVolumeBeginOverlap(UPrimitiveComponent *OverlappedComponent, AActor *OtherActor,
                              UPrimitiveComponent *OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult &SweepResult);

    UFUNCTION()
    void OnVolumeEndOverlap(UPrimitiveComponent *OverlappedComponent, AActor *OtherActor,
                            UPrimitiveComponent *OtherComp, int32 OtherBodyIndex);

    /** Handle cattle entering the volume */
    void HandleCattleEnter(ACattleAnimal *Cattle);

    /** Handle cattle exiting the volume */
    void HandleCattleExit(ACattleAnimal *Cattle);

    // Debug visualization override
    virtual void DrawDebugVisualization() const override;

public:
    /** Box trigger component defining the grazing zone */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UBoxComponent> TriggerBox;

protected:
    /** Track active GE handles per cattle actor */
    TMap<TWeakObjectPtr<AActor>, FActiveGameplayEffectHandle> ActiveEffectHandles;
};
