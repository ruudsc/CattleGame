#include "CattleVolume.h"
#include "Components/CattleVolumeLogicComponent.h"
#include "Components/BrushComponent.h"
#include "Engine/CollisionProfile.h"

ACattleVolume::ACattleVolume()
{
	// Default to no physics collision, but overlap events enabled
    GetBrushComponent()->SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName); // Start blocked or define custom? Trigger is typical.
    // Actually, volumes usually are trigger.
    GetBrushComponent()->SetCollisionProfileName(TEXT("Trigger"));
}

void ACattleVolume::BeginPlay()
{
	Super::BeginPlay();
	
	// Cache logic components
	TArray<UCattleVolumeLogicComponent*> FoundComponents;
	GetComponents<UCattleVolumeLogicComponent>(FoundComponents);
	LogicComponents.Empty(FoundComponents.Num());
	for (UCattleVolumeLogicComponent* Comp : FoundComponents)
	{
		LogicComponents.Add(Comp);
	}
}

void ACattleVolume::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();
	// Also ensure we cache if components are added later/in editor
	TArray<UCattleVolumeLogicComponent*> FoundComponents;
	GetComponents<UCattleVolumeLogicComponent>(FoundComponents);
	LogicComponents.Empty(FoundComponents.Num());
	for (UCattleVolumeLogicComponent* Comp : FoundComponents)
	{
		LogicComponents.Add(Comp);
	}
}

TArray<UCattleVolumeLogicComponent*> ACattleVolume::GetLogicComponents() const
{
	// Return raw pointers from cached TObjectPtrs
	TArray<UCattleVolumeLogicComponent*> Result;
	Result.Reserve(LogicComponents.Num());
	for (const auto& Comp : LogicComponents)
	{
		if (Comp)
		{
			Result.Add(Comp);
		}
	}
	return Result;
}
