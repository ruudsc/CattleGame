#include "GA_TrumpetLure.h"
#include "CattleGame/Weapons/Trumpet/Trumpet.h"
#include "CattleGame/AbilitySystem/CattleGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "CattleGame/Character/CattleCharacter.h"
#include "CattleGame/Character/InventoryComponent.h"
#include "CattleGame/CattleGame.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"

UGA_TrumpetLure::UGA_TrumpetLure()
{
	// This ability uses Ability_Trumpet_Lure tag
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(CattleGameplayTags::Ability_Trumpet_Lure);
	SetAssetTags(AssetTags);

	// Block activation if Scare is active
	ActivationBlockedTags.AddTag(CattleGameplayTags::Ability_Trumpet_Scare);

	// While this ability is active, add the Lure tag (so Scare can't activate)
	ActivationOwnedTags.AddTag(CattleGameplayTags::Ability_Trumpet_Lure);

	// Cancel this ability if Scare somehow activates
	CancelAbilitiesWithTag.AddTag(CattleGameplayTags::Ability_Trumpet_Scare);

	InputReleaseTask = nullptr;
}

void UGA_TrumpetLure::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
									  const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData *TriggerEventData)
{
	UE_LOG(LogGASDebug, Warning, TEXT("TrumpetLure Ability: ActivateAbility called"));

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// Start waiting for input release
	InputReleaseTask = UAbilityTask_WaitInputRelease::WaitInputRelease(this);
	if (InputReleaseTask)
	{
		InputReleaseTask->OnRelease.AddDynamic(this, &UGA_TrumpetLure::OnInputReleased);
		InputReleaseTask->ReadyForActivation();
	}

	PlayLure();
}

void UGA_TrumpetLure::OnInputReleased(float TimeHeld)
{
	UE_LOG(LogGASDebug, Warning, TEXT("TrumpetLure Ability: Input released after %.2f seconds"), TimeHeld);
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_TrumpetLure::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
								 const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	UE_LOG(LogGASDebug, Warning, TEXT("TrumpetLure Ability: EndAbility called"));

	// Remove GameplayCue for VFX/Audio
	if (UAbilitySystemComponent *ASC = ActorInfo->AbilitySystemComponent.Get())
	{
		ASC->RemoveGameplayCue(CattleGameplayTags::GameplayCue_Trumpet_Lure);
		ASC->RemoveLooseGameplayTag(CattleGameplayTags::State_Trumpet_Playing);
	}

	// Stop playing when fire is released
	ATrumpet *TrumpetWeapon = GetTrumpetWeapon();
	if (TrumpetWeapon && TrumpetWeapon->IsPlaying())
	{
		TrumpetWeapon->StopPlaying();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UGA_TrumpetLure::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
										 const FGameplayTagContainer *SourceTags, const FGameplayTagContainer *TargetTags,
										 OUT FGameplayTagContainer *OptionalRelevantTags) const
{
	// Get character from ActorInfo
	ACattleCharacter *Character = ActorInfo ? Cast<ACattleCharacter>(ActorInfo->OwnerActor.Get()) : nullptr;
	if (!Character)
		return false;

	// Get trumpet weapon
	UInventoryComponent *Inventory = Character->GetInventoryComponent();
	if (!Inventory)
		return false;

	ATrumpet *TrumpetWeapon = Cast<ATrumpet>(Inventory->GetEquippedWeapon());
	if (!TrumpetWeapon)
		return false;

	// Trumpet can always play
	return TrumpetWeapon->CanFire();
}

ATrumpet *UGA_TrumpetLure::GetTrumpetWeapon() const
{
	AWeaponBase *Weapon = GetWeapon_Implementation();
	return Cast<ATrumpet>(Weapon);
}

void UGA_TrumpetLure::PlayLure()
{
	ATrumpet *TrumpetWeapon = GetTrumpetWeapon();
	if (!TrumpetWeapon)
	{
		UE_LOG(LogGASDebug, Error, TEXT("TrumpetLure Ability: PlayLure - No trumpet weapon"));
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	UE_LOG(LogGASDebug, Warning, TEXT("TrumpetLure Ability: PlayLure - Starting lure effect"));

	// Start the trumpet's lure effect on cattle
	TrumpetWeapon->PlayLure();

	// Add playing state tag and trigger GameplayCue for VFX/Audio
	if (UAbilitySystemComponent *ASC = CurrentActorInfo->AbilitySystemComponent.Get())
	{
		ASC->AddLooseGameplayTag(CattleGameplayTags::State_Trumpet_Playing);

		// Add GameplayCue for replicated VFX/Audio (looping effect)
		ASC->AddGameplayCue(CattleGameplayTags::GameplayCue_Trumpet_Lure);
	}

	UE_LOG(LogGASDebug, Log, TEXT("GA_TrumpetLure - Trumpet lure GameplayCue triggered"));
}
