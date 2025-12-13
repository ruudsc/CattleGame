#include "GA_LassoRelease.h"
#include "CattleGame/Weapons/Lasso/Lasso.h"
#include "CattleGame/Character/CattleCharacter.h"
#include "CattleGame/Character/InventoryComponent.h"
#include "CattleGame/CattleGame.h"

UGA_LassoRelease::UGA_LassoRelease()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UGA_LassoRelease::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
									   const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData *TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ALasso *LassoWeapon = GetLassoWeapon();
	if (LassoWeapon && LassoWeapon->GetLassoState() == ELassoState::Tethered)
	{
		UE_LOG(LogLasso, Log, TEXT("GA_LassoRelease::ActivateAbility - Releasing tether on %s"),
			   *GetNameSafe(LassoWeapon->GetTetheredTarget()));
		LassoWeapon->ReleaseTether();
	}
	else
	{
		const TCHAR *StateNames[] = {TEXT("Idle"), TEXT("Throwing"), TEXT("Tethered"), TEXT("Retracting")};
		UE_LOG(LogLasso, Log, TEXT("GA_LassoRelease::ActivateAbility - Not tethered (State=%s), nothing to release"),
			   LassoWeapon ? StateNames[(int32)LassoWeapon->GetLassoState()] : TEXT("no weapon"));
	}

	// Instant ability - end immediately
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

bool UGA_LassoRelease::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
										  const FGameplayTagContainer *SourceTags, const FGameplayTagContainer *TargetTags,
										  OUT FGameplayTagContainer *OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		UE_LOG(LogLasso, Verbose, TEXT("GA_LassoRelease::CanActivateAbility - BLOCKED by parent class"));
		return false;
	}

	ACattleCharacter *Character = ResolveCharacterOwner(ActorInfo);
	if (!Character)
	{
		UE_LOG(LogLasso, Verbose, TEXT("GA_LassoRelease::CanActivateAbility - BLOCKED: No character"));
		return false;
	}

	UInventoryComponent *Inventory = Character->GetInventoryComponent();
	if (!Inventory)
	{
		UE_LOG(LogLasso, Verbose, TEXT("GA_LassoRelease::CanActivateAbility - BLOCKED: No inventory"));
		return false;
	}

	ALasso *LassoWeapon = Cast<ALasso>(Inventory->GetEquippedWeapon());
	if (!LassoWeapon)
	{
		UE_LOG(LogLasso, Verbose, TEXT("GA_LassoRelease::CanActivateAbility - BLOCKED: Equipped weapon is not lasso"));
		return false;
	}

	// Only activatable when tethered
	bool bCanActivate = LassoWeapon->GetLassoState() == ELassoState::Tethered;
	if (!bCanActivate)
	{
		const TCHAR *StateNames[] = {TEXT("Idle"), TEXT("Throwing"), TEXT("Tethered"), TEXT("Retracting")};
		UE_LOG(LogLasso, Verbose, TEXT("GA_LassoRelease::CanActivateAbility - BLOCKED: Not tethered (State=%s)"),
			   StateNames[(int32)LassoWeapon->GetLassoState()]);
	}
	return bCanActivate;
}

ALasso *UGA_LassoRelease::GetLassoWeapon() const
{
	AWeaponBase *Weapon = ResolveWeapon(CurrentActorInfo);
	return Cast<ALasso>(Weapon);
}
