#include "GA_TrumpetLure.h"
#include "CattleGame/Weapons/Trumpet/Trumpet.h"
#include "CattleGame/AbilitySystem/CattleGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "CattleGame/Character/CattleCharacter.h"
#include "CattleGame/Character/InventoryComponent.h"
#include "CattleGame/CattleGame.h"

void UGA_TrumpetLure::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
									   const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData *TriggerEventData)
{
	UE_LOG(LogGASDebug, Warning, TEXT("TrumpetLure Ability: ActivateAbility called"));

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	PlayLure();
}

void UGA_TrumpetLure::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
								  const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	UE_LOG(LogGASDebug, Warning, TEXT("TrumpetLure Ability: EndAbility called"));

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

	UE_LOG(LogGASDebug, Warning, TEXT("TrumpetLure Ability: PlayLure - Calling Trumpet->Fire()"));

	// Add playing state tag
	if (UAbilitySystemComponent *ASC = CurrentActorInfo->AbilitySystemComponent.Get())
	{
		ASC->AddLooseGameplayTag(CattleGameplayTags::State_Trumpet_Playing);
	}

	// Play the trumpet (lure)
	TrumpetWeapon->Fire();
}
