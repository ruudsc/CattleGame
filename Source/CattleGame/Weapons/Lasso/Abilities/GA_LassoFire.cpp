#include "GA_LassoFire.h"
#include "CattleGame/Weapons/Lasso/Lasso.h"
#include "CattleGame/AbilitySystem/CattleGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "CattleGame/Character/CattleCharacter.h"
#include "CattleGame/Character/InventoryComponent.h"
#include "CattleGame/CattleGame.h"

void UGA_LassoFire::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
									const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData *TriggerEventData)
{
	UE_LOG(LogGASDebug, Warning, TEXT("LassoFire Ability: ActivateAbility called"));

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ALasso *LassoWeapon = GetLassoWeapon();
	if (!LassoWeapon)
	{
		UE_LOG(LogGASDebug, Error, TEXT("LassoFire Ability: No lasso weapon!"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ELassoState CurrentState = LassoWeapon->GetLassoState();
	UE_LOG(LogGASDebug, Warning, TEXT("LassoFire Ability: Current lasso state = %d"), (int32)CurrentState);

	if (CurrentState == ELassoState::Idle)
	{
		// Fire the lasso
		FireLasso();
	}
	else if (CurrentState == ELassoState::Tethered)
	{
		// Start pulling - elastic force applied in Lasso::TickTethered
		UE_LOG(LogGASDebug, Warning, TEXT("LassoFire Ability: Starting pull on tethered target"));
		LassoWeapon->SetPulling(true);
	}
	else
	{
		// In flight or retracting - can't do anything
		UE_LOG(LogGASDebug, Warning, TEXT("LassoFire Ability: Can't fire/pull in state %d"), (int32)CurrentState);
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}

void UGA_LassoFire::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
							   const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	UE_LOG(LogGASDebug, Warning, TEXT("LassoFire Ability: EndAbility called, bWasCancelled: %d"), bWasCancelled);

	// Stop pulling when button is released
	ALasso *LassoWeapon = GetLassoWeapon();
	if (LassoWeapon && LassoWeapon->GetLassoState() == ELassoState::Tethered)
	{
		UE_LOG(LogGASDebug, Warning, TEXT("LassoFire Ability: Stopping pull"));
		LassoWeapon->SetPulling(false);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UGA_LassoFire::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
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

	// Can activate if:
	// - Idle (to throw)
	// - Tethered (to pull)
	ELassoState State = LassoWeapon->GetLassoState();
	if (State == ELassoState::Idle || State == ELassoState::Tethered)
	{
		return true;
	}

	UE_LOG(LogGASDebug, Warning, TEXT("LassoFire Ability: CanActivateAbility = FALSE, State = %d"), (int32)State);
	return false;
}

ALasso *UGA_LassoFire::GetLassoWeapon() const
{
	AWeaponBase *Weapon = GetWeapon_Implementation();
	return Cast<ALasso>(Weapon);
}

void UGA_LassoFire::FireLasso()
{
	ALasso *LassoWeapon = GetLassoWeapon();
	if (!LassoWeapon)
	{
		UE_LOG(LogGASDebug, Error, TEXT("LassoFire Ability: FireLasso - No lasso weapon"));
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	ACattleCharacter *Character = Cast<ACattleCharacter>(CurrentActorInfo->OwnerActor.Get());
	if (!Character)
	{
		UE_LOG(LogGASDebug, Error, TEXT("LassoFire Ability: FireLasso - No character"));
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	UE_LOG(LogGASDebug, Warning, TEXT("LassoFire Ability: FireLasso - Throwing lasso"));

	// Add firing state tag and execute GameplayCue for VFX/Audio (one-shot)
	if (UAbilitySystemComponent *ASC = CurrentActorInfo->AbilitySystemComponent.Get())
	{
		ASC->AddLooseGameplayTag(CattleGameplayTags::State_Lasso_Active);

		// Execute GameplayCue for replicated VFX/Audio (one-shot burst effect)
		ASC->ExecuteGameplayCue(CattleGameplayTags::GameplayCue_Lasso_Throw);
	}

	// Get spawn location and direction from character
	FVector SpawnLocation = Character->GetActorLocation() + Character->GetActorForwardVector() * 100.0f + FVector(0, 0, 50);
	FVector LaunchDirection = Character->GetControlRotation().Vector();

	// Call server to throw the lasso
	LassoWeapon->ServerFire(SpawnLocation, LaunchDirection);
}
