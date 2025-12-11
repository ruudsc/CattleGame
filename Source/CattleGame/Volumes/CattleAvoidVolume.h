#pragma once

#include "CoreMinimal.h"
#include "CattleFlowActorBase.h"
#include "CattleAvoidVolume.generated.h"

class UBoxComponent;
class ACattleAnimal;

/**
 * Volume that repels cattle away from its center.
 * Use this to create no-go zones or redirect cattle around obstacles.
 *
 * Place in the level and scale the box trigger to define the avoidance area.
 */
UCLASS(Blueprintable)
class CATTLEGAME_API ACattleAvoidVolume : public ACattleFlowActorBase
{
    GENERATED_BODY()

public:
    ACattleAvoidVolume();

protected:
    virtual void BeginPlay() override;

    // Flow calculation - pushes cattle away from center
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

public:
    /** Box trigger component defining the avoidance zone */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UBoxComponent> TriggerBox;

    /** Strength of the repulsion force (multiplier on flow direction) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cattle Flow|Avoid", meta = (ClampMin = "0.0"))
    float RepulsionStrength = 1.0f;

protected:
    /** Track active GE handles per cattle actor */
    TMap<TWeakObjectPtr<AActor>, FActiveGameplayEffectHandle> ActiveEffectHandles;
};
