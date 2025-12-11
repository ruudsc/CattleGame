#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffect.h"
#include "CattleVolumeLogicComponent.generated.h"

class ACattleAnimal;
class ACattleVolume;

/**
 * DEPRECATED: Use ACattleFlowActorBase derived actors instead (ACattleAvoidVolume, ACattleGrazeVolume, ACattleGuideSpline).
 *
 * Base Logic Component for Cattle Volumes.
 * Handles:
 * 1. Listening for Overlap events from the Owner (Volume).
 * 2. Applying/Removing GameplayEffects to Cattle.
 * 3. Providing flow vectors for movement.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent, DeprecatedNode, DeprecationMessage = "Use ACattleFlowActorBase derived actors instead"), Abstract)
class CATTLEGAME_API UCattleVolumeLogicComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCattleVolumeLogicComponent();

	virtual void PostLoad() override;

protected:
	virtual void BeginPlay() override;
	virtual void OnRegister() override;

public:
	/**
	 * Gameplay Effect to apply when a Cattle enters the volume.
	 * Automatically removed on exit.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cattle Logic")
	TSubclassOf<UGameplayEffect> ApplyEffectClass;

	/**
	 * Get the desired flow direction at a specific location.
	 * Override this for Guide/Avoid behaviors.
	 * @param Location World location to sample.
	 * @return Normalized direction vector (magnitude can be > 1 for intensity).
	 */
	virtual FVector GetFlowDirection(const FVector &Location) const { return FVector::ZeroVector; }

protected:
	// Overlap Handlers
	UFUNCTION()
	virtual void OnOverlapBegin(AActor *OverlappedActor, AActor *OtherActor);

	UFUNCTION()
	virtual void OnOverlapEnd(AActor *OverlappedActor, AActor *OtherActor);

	// Helpers
public:
	UFUNCTION(BlueprintCallable, Category = "Cattle Logic")
	class ACattleVolume *GetOwningVolume() const;

protected:
private:
	// Track active GE handles per actor to remove them later
	TMap<FObjectKey, FActiveGameplayEffectHandle> ActiveEffectHandles;
};
