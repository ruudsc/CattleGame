#include "GA_TrumpetScare.h"
#include "CattleGame/Weapons/Trumpet/Trumpet.h"
#include "CattleGame/AbilitySystem/CattleGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "CattleGame/Character/CattleCharacter.h"
#include "CattleGame/Character/InventoryComponent.h"
#include "CattleGame/CattleGame.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"

UGA_TrumpetScare::UGA_TrumpetScare()
{
	// This ability uses Ability_Trumpet_Scare tag
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(CattleGameplayTags::Ability_Trumpet_Scare);
	SetAssetTags(AssetTags);

	// Block activation if Lure is active
	ActivationBlockedTags.AddTag(CattleGameplayTags::Ability_Trumpet_Lure);

	// While this ability is active, add the Scare tag (so Lure can't activate)
	ActivationOwnedTags.AddTag(CattleGameplayTags::Ability_Trumpet_Scare);

	// Cancel this ability if Lure somehow activates
	CancelAbilitiesWithTag.AddTag(CattleGameplayTags::Ability_Trumpet_Lure);

	InputReleaseTask = nullptr;
}

void UGA_TrumpetScare::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
									   const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData *TriggerEventData)
{
	UE_LOG(LogGASDebug, Warning, TEXT("TrumpetScare Ability: ActivateAbility called"));

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// Start waiting for input release
	InputReleaseTask = UAbilityTask_WaitInputRelease::WaitInputRelease(this);
	if (InputReleaseTask)
	{
		InputReleaseTask->OnRelease.AddDynamic(this, &UGA_TrumpetScare::OnInputReleased);
		InputReleaseTask->ReadyForActivation();
	}

	PlayScare();
}

void UGA_TrumpetScare::OnInputReleased(float TimeHeld)
{
	UE_LOG(LogGASDebug, Warning, TEXT("TrumpetScare Ability: Input released after %.2f seconds"), TimeHeld);
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_TrumpetScare::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
								  const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	UE_LOG(LogGASDebug, Warning, TEXT("TrumpetScare Ability: EndAbility called"));

	// Remove GameplayCue for VFX/Audio
	if (UAbilitySystemComponent *ASC = ActorInfo->AbilitySystemComponent.Get())
	{
		ASC->RemoveGameplayCue(CattleGameplayTags::GameplayCue_Trumpet_Scare);
		ASC->RemoveLooseGameplayTag(CattleGameplayTags::State_Trumpet_Playing);
	}

	// Stop playing when secondary fire is released
	ATrumpet *TrumpetWeapon = GetTrumpetWeapon();
	if (TrumpetWeapon && TrumpetWeapon->IsPlaying())
	{
		TrumpetWeapon->StopPlaying();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UGA_TrumpetScare::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
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

ATrumpet *UGA_TrumpetScare::GetTrumpetWeapon() const
{
	AWeaponBase *Weapon = GetWeapon_Implementation();
	return Cast<ATrumpet>(Weapon);
}

void UGA_TrumpetScare::PlayScare()
{
	ATrumpet *TrumpetWeapon = GetTrumpetWeapon();
	if (!TrumpetWeapon)
	{
		UE_LOG(LogGASDebug, Error, TEXT("TrumpetScare Ability: PlayScare - No trumpet weapon"));
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	UE_LOG(LogGASDebug, Warning, TEXT("TrumpetScare Ability: PlayScare - Starting scare effect"));

	// Add playing state tag and trigger GameplayCue for VFX/Audio
	if (UAbilitySystemComponent *ASC = CurrentActorInfo->AbilitySystemComponent.Get())
	{
		ASC->AddLooseGameplayTag(CattleGameplayTags::State_Trumpet_Playing);

		// Add GameplayCue for replicated VFX/Audio (looping effect)
		ASC->AddGameplayCue(CattleGameplayTags::GameplayCue_Trumpet_Scare);
	}

	// Play the scare sound (legacy - will be handled by GameplayCue)
	TrumpetWeapon->PlayScare();
}
