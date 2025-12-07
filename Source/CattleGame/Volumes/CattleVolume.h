#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Volume.h"
#include "CattleVolume.generated.h"

class UCattleVolumeLogicComponent;

/**
 * Base volume class for Cattle AI areas.
 * Inherits from AVolume to support Brush Shaping / BSP logic.
 * The actual behavior is defined by UCattleVolumeLogicComponent subobjects.
 */
UCLASS()
class CATTLEGAME_API ACattleVolume : public AVolume
{
	GENERATED_BODY()
	
public:
	ACattleVolume();

protected:
	virtual void BeginPlay() override;

	virtual void PostRegisterAllComponents() override;

public:
	// Helper to find logic components on this volume
	TArray<UCattleVolumeLogicComponent*> GetLogicComponents() const;

private:
	/** Cached list of logic components for runtime efficiency */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UCattleVolumeLogicComponent>> LogicComponents;
};
