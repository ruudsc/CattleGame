// Copyright Epic Games, Inc. All Rights Reserved.

#include "CattleAbilitySystemComponent.h"
#include "CattleAttributeSet.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/Actor.h"

UCattleAbilitySystemComponent::UCattleAbilitySystemComponent()
	: AttributeSet(nullptr)
{
	// Enable replication
	SetIsReplicated(true);

	// Use minimal replication for better performance in multiplayer
	ReplicationMode = EGameplayEffectReplicationMode::Minimal;

	// Set the default ability batch size for better performance
	// DefaultAbilityBatchSize = 1; // This is not a valid property, removed
}

void UCattleAbilitySystemComponent::BeginPlay()
{
	Super::BeginPlay();

	// Initialize ability system globals if not already initialized
	UAbilitySystemGlobals::Get().InitGlobalData();
}

void UCattleAbilitySystemComponent::InitializeAbilitySystem(AActor *InOwnerActor, AActor *InAvatarActor)
{
	// This should only be called on the server or owning client
	if (InOwnerActor && !InOwnerActor->HasAuthority())
	{
		return;
	}

	// Set the owner actor info
	SetOwnerActor(InOwnerActor);

	// Create and initialize the AttributeSet
	if (!AttributeSet)
	{
		AttributeSet = NewObject<UCattleAttributeSet>(InOwnerActor);
		AddAttributeSetSubobject(AttributeSet);
	}

	// Initialize the ability actor info
	InitAbilityActorInfo(InOwnerActor, InAvatarActor);

	// Bind to attribute changes
	if (AttributeSet)
	{
		// Bind to health changes
		GetGameplayAttributeValueChangeDelegate(AttributeSet->GetHealthAttribute()).AddUObject(this, &UCattleAbilitySystemComponent::OnHealthChangedCallback);
	}
}

FGameplayAbilitySpecHandle UCattleAbilitySystemComponent::GrantAbility(TSubclassOf<UGameplayAbility> AbilityClass, int32 Level, int32 InputID)
{
	if (!AbilityClass)
	{
		return FGameplayAbilitySpecHandle();
	}

	// Create and grant the ability
	FGameplayAbilitySpec AbilitySpec(AbilityClass, Level, InputID);
	return GiveAbility(AbilitySpec);
}

void UCattleAbilitySystemComponent::RemoveAbility(FGameplayAbilitySpecHandle AbilitySpecHandle)
{
	if (!AbilitySpecHandle.IsValid())
	{
		return;
	}

	// Remove the ability from the spec list
	ClearAbility(AbilitySpecHandle);
}

bool UCattleAbilitySystemComponent::ActivateAbilityByTag(const FGameplayTag &AbilityTag)
{
	if (!AbilityTag.IsValid())
	{
		return false;
	}

	// Find abilities matching the tag
	TArray<FGameplayAbilitySpec *> MatchingAbilities;
	GetActivatableGameplayAbilitySpecsByAllMatchingTags(AbilityTag.GetSingleTagContainer(), MatchingAbilities);

	if (MatchingAbilities.IsEmpty())
	{
		return false;
	}

	// Activate the first matching ability
	return TryActivateAbility(MatchingAbilities[0]->Handle);
}

void UCattleAbilitySystemComponent::OnHealthChangedCallback(const FOnAttributeChangeData &Data)
{
	if (AttributeSet)
	{
		// Call the blueprint event with health changes
		OnHealthChanged(Data.NewValue, AttributeSet->GetMaxHealth());

		// Check if character died
		if (Data.OldValue > 0.0f && Data.NewValue <= 0.0f)
		{
			OnDeath();
		}
	}
}
