#include "GA_TrumpetScare.h"
#include "CattleGame/Weapons/Trumpet/Trumpet.h"
#include "CattleGame/AbilitySystem/CattleGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "CattleGame/Character/CattleCharacter.h"
#include "CattleGame/Character/InventoryComponent.h"
#include "CattleGame/CattleGame.h"

void UGA_TrumpetScare::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
										const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData *TriggerEventData)
{
	UE_LOG(LogGASDebug, Warning, TEXT("TrumpetScare Ability: ActivateAbility called"));

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	PlayScare();
}

void UGA_TrumpetScare::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
								   const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	UE_LOG(LogGASDebug, Warning, TEXT("TrumpetScare Ability: EndAbility called"));

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

	UE_LOG(LogGASDebug, Warning, TEXT("TrumpetScare Ability: PlayScare - Calling Trumpet->PlayScare()"));

	// Add playing state tag
	if (UAbilitySystemComponent *ASC = CurrentActorInfo->AbilitySystemComponent.Get())
	{
		ASC->AddLooseGameplayTag(CattleGameplayTags::State_Trumpet_Playing);
	}

	// Play the scare sound
	TrumpetWeapon->PlayScare();
}
