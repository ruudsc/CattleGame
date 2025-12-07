#include "CattleVolumeLogicComponent.h"
#include "CattleGame/Volumes/CattleVolume.h"
#include "CattleGame/Animals/CattleAnimal.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"

UCattleVolumeLogicComponent::UCattleVolumeLogicComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCattleVolumeLogicComponent::PostLoad()
{
	Super::PostLoad();
}

void UCattleVolumeLogicComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UCattleVolumeLogicComponent::OnRegister()
{
	Super::OnRegister();

	// Automatically bind to owner overlap events if we are on an actor
	if (AActor* Owner = GetOwner())
	{
		Owner->OnActorBeginOverlap.AddDynamic(this, &UCattleVolumeLogicComponent::OnOverlapBegin);
		Owner->OnActorEndOverlap.AddDynamic(this, &UCattleVolumeLogicComponent::OnOverlapEnd);
	}
}

ACattleVolume* UCattleVolumeLogicComponent::GetOwningVolume() const
{
	return Cast<ACattleVolume>(GetOwner());
}

void UCattleVolumeLogicComponent::OnOverlapBegin(AActor* OverlappedActor, AActor* OtherActor)
{
	if (!OtherActor || OtherActor == OverlappedActor)
	{
		return;
	}

	// 1. Check if it's a Cattle Animal
	ACattleAnimal* Cattle = Cast<ACattleAnimal>(OtherActor);
	if (!Cattle)
	{
		return;
	}

	// 2. Register Logic with Cattle (For polling flow direction)
	Cattle->AddActiveVolumeLogic(this);

	// 3. Apply Gameplay Effect
	if (ApplyEffectClass)
	{
		UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor);
		if (ASC)
		{
			FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
			EffectContext.AddSourceObject(this);

			FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(ApplyEffectClass, 1.0f, EffectContext);
			if (SpecHandle.IsValid())
			{
				FActiveGameplayEffectHandle ActiveHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
				ActiveEffectHandles.Add(OtherActor, ActiveHandle);
			}
		}
	}
}

void UCattleVolumeLogicComponent::OnOverlapEnd(AActor* OverlappedActor, AActor* OtherActor)
{
	if (!OtherActor)
	{
		return;
	}

	// 1. Unregister Logic
	if (ACattleAnimal* Cattle = Cast<ACattleAnimal>(OtherActor))
	{
		Cattle->RemoveActiveVolumeLogic(this);
	}

	// 2. Remove Gameplay Effect
	if (ActiveEffectHandles.Contains(OtherActor))
	{
		UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor);
		if (ASC)
		{
			ASC->RemoveActiveGameplayEffect(ActiveEffectHandles[OtherActor], 1); // Stacks = 1
		}
		ActiveEffectHandles.Remove(OtherActor);
	}
}
