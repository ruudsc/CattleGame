#include "GA_LassoRetract.h"
#include "CattleGame/Weapons/Lasso/Lasso.h"
#include "CattleGame/AbilitySystem/CattleGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "CattleGame/Character/CattleCharacter.h"
#include "CattleGame/Character/InventoryComponent.h"
#include "CattleGame/CattleGame.h"

void UGA_LassoRetract::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
									   const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData *TriggerEventData)
{
	UE_LOG(LogGASDebug, Log, TEXT("LassoRetract/Release Ability: ActivateAbility called"));

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ReleaseLasso();

	// Ability completes immediately - actual retraction is handled by weapon/projectile
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

bool UGA_LassoRetract::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
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

	// Can only release/retract if lasso is tethered or in-flight
	ELassoState State = LassoWeapon->GetLassoState();
	if (State != ELassoState::Tethered && State != ELassoState::InFlight)
	{
		UE_LOG(LogGASDebug, Log, TEXT("LassoRetract/Release Ability: CanActivateAbility = FALSE, state=%d"), (int32)State);
		return false;
	}

	return true;
}

ALasso *UGA_LassoRetract::GetLassoWeapon() const
{
	AWeaponBase *Weapon = GetWeapon_Implementation();
	return Cast<ALasso>(Weapon);
}

void UGA_LassoRetract::ReleaseLasso()
{
	ALasso *LassoWeapon = GetLassoWeapon();
	if (!LassoWeapon)
	{
		UE_LOG(LogGASDebug, Error, TEXT("LassoRetract/Release Ability: ReleaseLasso - No lasso weapon"));
		return;
	}

	ELassoState State = LassoWeapon->GetLassoState();
	UE_LOG(LogGASDebug, Log, TEXT("LassoRetract/Release Ability: ReleaseLasso - Current state=%d"), (int32)State);

	// If tethered, release the tether (this triggers retract internally)
	if (State == ELassoState::Tethered)
	{
		LassoWeapon->ReleaseTether();
	}
	// If in-flight, just start retracting
	else if (State == ELassoState::InFlight)
	{
		LassoWeapon->StartRetract();
	}
}
