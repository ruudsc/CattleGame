#include "GA_LassoThrow.h"
#include "CattleGame/Weapons/Lasso/Lasso.h"
#include "CattleGame/Character/CattleCharacter.h"
#include "CattleGame/Character/InventoryComponent.h"
#include "CattleGame/CattleGame.h"

UGA_LassoThrow::UGA_LassoThrow()
{
	// Instanced per actor for state tracking
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// Let server handle the actual throw
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UGA_LassoThrow::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
									 const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData *TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ALasso *LassoWeapon = GetLassoWeapon();
	if (!LassoWeapon)
	{
		UE_LOG(LogLasso, Error, TEXT("GA_LassoThrow::ActivateAbility - FAILED: No lasso weapon found"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ELassoState State = LassoWeapon->GetLassoState();
	const TCHAR *StateNames[] = {TEXT("Idle"), TEXT("Throwing"), TEXT("Tethered"), TEXT("Retracting")};
	UE_LOG(LogLasso, Log, TEXT("GA_LassoThrow::ActivateAbility - Current state: %s"), StateNames[(int32)State]);

	switch (State)
	{
	case ELassoState::Idle:
		// Throw the lasso
		UE_LOG(LogLasso, Log, TEXT("GA_LassoThrow::ActivateAbility - Initiating THROW"));
		ExecuteThrow();
		// ExecuteThrow calls EndAbility
		break;

	case ELassoState::Tethered:
		// Start pulling - ability stays active until input released
		UE_LOG(LogLasso, Log, TEXT("GA_LassoThrow::ActivateAbility - Initiating PULL (ability stays active)"));
		bIsPullMode = true;
		LassoWeapon->StartPulling();
		// Don't end ability here - it will end when input is released (InputReleased or EndAbility called externally)
		break;

	default:
		// Can't do anything in Throwing or Retracting states
		UE_LOG(LogLasso, Log, TEXT("GA_LassoThrow::ActivateAbility - Cannot act in state %s, ending ability"),
			   StateNames[(int32)State]);
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		break;
	}
}

void UGA_LassoThrow::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
								const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	UE_LOG(LogLasso, Log, TEXT("GA_LassoThrow::EndAbility - Replicate=%d, Cancelled=%d, WasPullMode=%d"),
		   bReplicateEndAbility, bWasCancelled, bIsPullMode);

	// Stop pulling if we were in pull mode
	if (bIsPullMode)
	{
		ALasso *LassoWeapon = GetLassoWeapon();
		if (LassoWeapon)
		{
			UE_LOG(LogLasso, Log, TEXT("GA_LassoThrow::EndAbility - Stopping pull"));
			LassoWeapon->StopPulling();
		}
		bIsPullMode = false;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UGA_LassoThrow::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
										const FGameplayTagContainer *SourceTags, const FGameplayTagContainer *TargetTags,
										OUT FGameplayTagContainer *OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		UE_LOG(LogLasso, Verbose, TEXT("GA_LassoThrow::CanActivateAbility - BLOCKED by parent class"));
		return false;
	}

	// Get lasso weapon
	ACattleCharacter *Character = ResolveCharacterOwner(ActorInfo);
	if (!Character)
	{
		UE_LOG(LogLasso, Verbose, TEXT("GA_LassoThrow::CanActivateAbility - BLOCKED: No character"));
		return false;
	}

	UInventoryComponent *Inventory = Character->GetInventoryComponent();
	if (!Inventory)
	{
		UE_LOG(LogLasso, Verbose, TEXT("GA_LassoThrow::CanActivateAbility - BLOCKED: No inventory"));
		return false;
	}

	ALasso *LassoWeapon = Cast<ALasso>(Inventory->GetEquippedWeapon());
	if (!LassoWeapon)
	{
		UE_LOG(LogLasso, Verbose, TEXT("GA_LassoThrow::CanActivateAbility - BLOCKED: Equipped weapon is not lasso"));
		return false;
	}

	// Can activate if Idle (to throw) or Tethered (to pull)
	ELassoState State = LassoWeapon->GetLassoState();
	bool bCanActivate = (State == ELassoState::Idle && LassoWeapon->CanFire()) || State == ELassoState::Tethered;

	if (!bCanActivate)
	{
		const TCHAR *StateNames[] = {TEXT("Idle"), TEXT("Throwing"), TEXT("Tethered"), TEXT("Retracting")};
		UE_LOG(LogLasso, Log, TEXT("GA_LassoThrow::CanActivateAbility - BLOCKED: State=%s, CanFire=%d"),
			   StateNames[(int32)State], LassoWeapon->CanFire());
	}

	return bCanActivate;
}

ALasso *UGA_LassoThrow::GetLassoWeapon() const
{
	AWeaponBase *Weapon = ResolveWeapon(CurrentActorInfo);
	return Cast<ALasso>(Weapon);
}

void UGA_LassoThrow::ExecuteThrow()
{
	ALasso *LassoWeapon = GetLassoWeapon();
	ACattleCharacter *Character = GetCharacterOwner();

	if (!LassoWeapon || !Character)
	{
		UE_LOG(LogLasso, Error, TEXT("GA_LassoThrow::ExecuteThrow - FAILED: Weapon=%s, Character=%s"),
			   *GetNameSafe(LassoWeapon), *GetNameSafe(Character));
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	// Calculate spawn location and direction
	FVector SpawnLocation = Character->GetActorLocation() + Character->GetActorForwardVector() * 80.0f + FVector(0, 0, 60);

	// Prefer spawning from the actual weapon mesh if available
	if (UStaticMeshComponent *CoilMesh = LassoWeapon->GetHandCoilMesh())
	{
		SpawnLocation = CoilMesh->GetComponentLocation();
	}

	FVector LaunchDirection = Character->GetControlRotation().Vector();

	UE_LOG(LogLasso, Log, TEXT("GA_LassoThrow::ExecuteThrow - Throwing lasso"));
	UE_LOG(LogLasso, Log, TEXT("  SpawnLocation: %s"), *SpawnLocation.ToString());
	UE_LOG(LogLasso, Log, TEXT("  LaunchDirection: %s (from control rotation %s)"),
		   *LaunchDirection.ToString(), *Character->GetControlRotation().ToString());

	// Call server to throw
	LassoWeapon->ServerFire(SpawnLocation, LaunchDirection);

	// End ability immediately after throw - it will be reactivated for pull or next throw
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_LassoThrow::InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
								   const FGameplayAbilityActivationInfo ActivationInfo)
{
	UE_LOG(LogLasso, Log, TEXT("GA_LassoThrow::InputReleased - bIsPullMode=%d"), bIsPullMode);

	// If we were pulling, end the ability now
	if (bIsPullMode)
	{
		UE_LOG(LogLasso, Log, TEXT("GA_LassoThrow::InputReleased - Ending pull ability"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}
