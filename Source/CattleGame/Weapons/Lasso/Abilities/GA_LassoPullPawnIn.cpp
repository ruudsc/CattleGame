#include "GA_LassoPullPawnIn.h"
#include "CattleGame/Weapons/Lasso/Lasso.h"
#include "CattleGame/AbilitySystem/CattleGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "CattleGame/Character/CattleCharacter.h"
#include "CattleGame/Character/InventoryComponent.h"
#include "CattleGame/CattleGame.h"

void UGA_LassoPullPawnIn::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
										  const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData *TriggerEventData)
{
	UE_LOG(LogGASDebug, Log, TEXT("LassoPullPawnIn Ability: ActivateAbility called"));

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// Enable pulling - the elastic tether force is applied in Lasso::TickTethered
	ALasso *LassoWeapon = GetLassoWeapon();
	if (LassoWeapon)
	{
		LassoWeapon->SetPulling(true);
	}
}

void UGA_LassoPullPawnIn::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
									 const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	UE_LOG(LogGASDebug, Log, TEXT("LassoPullPawnIn Ability: EndAbility called, bWasCancelled: %d"), bWasCancelled);

	// Disable pulling when button is released
	ALasso *LassoWeapon = GetLassoWeapon();
	if (LassoWeapon)
	{
		LassoWeapon->SetPulling(false);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UGA_LassoPullPawnIn::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
											 const FGameplayTagContainer *SourceTags, const FGameplayTagContainer *TargetTags,
											 OUT FGameplayTagContainer *OptionalRelevantTags) const
{
	// Get character from ActorInfo
	ACattleCharacter *Character = ActorInfo ? Cast<ACattleCharacter>(ActorInfo->OwnerActor.Get()) : nullptr;
	if (!Character)
		return false;

	// Get lasso weapon
	UInventoryComponent *Inventory = Character->GetInventoryComponent();
	if (!Inventory)
		return false;

	ALasso *LassoWeapon = Cast<ALasso>(Inventory->GetEquippedWeapon());
	if (!LassoWeapon)
		return false;

	// Can ONLY pull if lasso is tethered to a target
	if (LassoWeapon->GetLassoState() != ELassoState::Tethered)
	{
		return false;
	}

	return true;
}

ALasso *UGA_LassoPullPawnIn::GetLassoWeapon() const
{
	AWeaponBase *Weapon = GetWeapon_Implementation();
	return Cast<ALasso>(Weapon);
}
