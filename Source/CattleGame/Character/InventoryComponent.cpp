#include "InventoryComponent.h"
#include "CattleCharacter.h"
#include "CattleGame/Weapons/WeaponBase.h"
#include "CattleGame/AbilitySystem/CattleAbilitySystemComponent.h"
#include "CattleGame/AbilitySystem/CattleGameplayTags.h"
#include "Net/UnrealNetwork.h"
#include "CattleGame/CattleGame.h"
#include "EnhancedInputComponent.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.TickInterval = 0.1f;
	SetIsReplicated(true);

	// Initialize weapon slots array with 6 empty slots
	WeaponSlots.SetNum((int32)EWeaponSlot::MaxSlots);
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	// Get owner character reference
	OwnerCharacter = Cast<ACattleCharacter>(GetOwner());

	AActor *Owner = GetOwner();
	if (!Owner || Owner->GetLocalRole() != ROLE_Authority)
	{
		return;
	}

	// Initialize default weapons if classes are set
	if (DefaultRevolverClass || DefaultLassoClass || DefaultDynamiteClass || DefaultTrumpetClass)
	{
		InitializeDefaultWeapons();
	}
}

bool UInventoryComponent::EquipWeapon(int32 SlotIndex)
{
	if (!IsSlotValid(SlotIndex))
	{
		UE_LOG(LogGASDebug, Error, TEXT("EquipWeapon: BLOCKED - Invalid slot index %d (valid range: 0-%d)"),
			   SlotIndex, (int32)EWeaponSlot::MaxSlots - 1);
		return false;
	}

	UE_LOG(LogGASDebug, Warning, TEXT("EquipWeapon: Attempting to equip slot %d, CurrentEquippedSlot: %d"),
		   SlotIndex, CurrentEquippedSlot);

	// If on client, call server RPC
	AActor *Owner = GetOwner();
	if (!Owner || Owner->GetLocalRole() != ROLE_Authority)
	{
		ServerEquipWeapon(SlotIndex);
		return true;
	}

	// Get weapon from slot (may be nullptr for empty slots)
	AWeaponBase *WeaponToEquip = WeaponSlots[SlotIndex];

	// Already equipped (including if both are nullptr)
	if (WeaponToEquip == EquippedWeapon)
	{
		return false;
	}

	// Unequip current weapon
	if (EquippedWeapon)
	{
		// Revoke weapon abilities before unequipping
		RevokeWeaponAbilities(EquippedWeapon);
		EquippedWeapon->UnequipWeapon();
	}

	// Equip new weapon (or set to nullptr for empty slot = unarmed state)
	EquippedWeapon = WeaponToEquip;
	CurrentEquippedSlot = SlotIndex;

	if (WeaponToEquip)
	{
		UE_LOG(LogGASDebug, Warning, TEXT("EquipWeapon [%s]: Equipping weapon %p (name: %s) to slot %d (Owner=%s)"),
			   GetOwner() && GetOwner()->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"),
			   EquippedWeapon, *EquippedWeapon->GetName(), SlotIndex,
			   *GetNameSafe(GetOwner()));
		EquippedWeapon->EquipWeapon();

		// Grant weapon abilities after equipping
		GrantWeaponAbilities(WeaponToEquip);

		OnWeaponEquipped.Broadcast();
	}
	else
	{
		UE_LOG(LogGASDebug, Warning, TEXT("EquipWeapon: Equipping empty slot %d (unarmed state)"), SlotIndex);
		OnWeaponUnequipped.Broadcast();
	}

	return true;
}

void UInventoryComponent::UnequipWeapon()
{
	AActor *Owner = GetOwner();
	if (!Owner || Owner->GetLocalRole() != ROLE_Authority)
	{
		return;
	}

	if (EquippedWeapon)
	{
		// Revoke weapon abilities before unequipping
		RevokeWeaponAbilities(EquippedWeapon);

		EquippedWeapon->UnequipWeapon();
		EquippedWeapon = nullptr;
		CurrentEquippedSlot = -1;
		OnWeaponUnequipped.Broadcast();
	}
}

bool UInventoryComponent::AddWeaponToSlot(AWeaponBase *Weapon, int32 SlotIndex)
{
	if (!Weapon || !IsSlotValid(SlotIndex))
	{
		return false;
	}

	// Check if slot is already occupied
	if (WeaponSlots[SlotIndex] != nullptr)
	{
		return false;
	}

	// Add weapon to slot
	WeaponSlots[SlotIndex] = Weapon;
	UE_LOG(LogGASDebug, Warning, TEXT("AddWeaponToSlot: Adding weapon %p (name: %s) to slot %d, OwnerCharacter: %p"),
		   Weapon, *Weapon->GetName(), SlotIndex, OwnerCharacter);
	Weapon->SetOwnerCharacter(OwnerCharacter);
	// Note: Mesh attachment is now handled in blueprints

	// Auto-equip if no weapon is currently equipped
	if (CurrentEquippedSlot == -1)
	{
		EquipWeapon(SlotIndex);
	}

	OnWeaponAdded.Broadcast();

	return true;
}

int32 UInventoryComponent::AddWeaponToFirstAvailableSlot(AWeaponBase *Weapon)
{
	if (!Weapon)
	{
		return -1;
	}

	// Try to add to pickup slots (3-5)
	for (int32 i = 3; i < (int32)EWeaponSlot::MaxSlots; i++)
	{
		if (WeaponSlots[i] == nullptr)
		{
			if (AddWeaponToSlot(Weapon, i))
			{
				return i;
			}
		}
	}

	return -1; // No space available
}

AWeaponBase *UInventoryComponent::RemoveWeaponFromSlot(int32 SlotIndex)
{
	if (!IsSlotValid(SlotIndex))
	{
		return nullptr;
	}

	AWeaponBase *RemovedWeapon = WeaponSlots[SlotIndex];

	if (RemovedWeapon)
	{
		// If it's the equipped weapon, unequip it first
		if (RemovedWeapon == EquippedWeapon)
		{
			UnequipWeapon();
		}

		// Remove from slot
		WeaponSlots[SlotIndex] = nullptr;
		RemovedWeapon->SetOwnerCharacter(nullptr);
		// Note: Mesh detachment is now handled in blueprints

		OnWeaponRemoved.Broadcast();
	}

	return RemovedWeapon;
}

void UInventoryComponent::DropEquippedWeapon()
{
	AActor *Owner = GetOwner();
	if (!Owner || Owner->GetLocalRole() != ROLE_Authority)
	{
		ServerDropWeapon();
		return;
	}

	if (CurrentEquippedSlot >= 0)
	{
		RemoveWeaponFromSlot(CurrentEquippedSlot);
	}
}

AWeaponBase *UInventoryComponent::GetWeaponInSlot(int32 SlotIndex) const
{
	if (!IsSlotValid(SlotIndex))
	{
		return nullptr;
	}

	return WeaponSlots[SlotIndex];
}

bool UInventoryComponent::IsSlotEmpty(int32 SlotIndex) const
{
	if (!IsSlotValid(SlotIndex))
	{
		return true;
	}

	return WeaponSlots[SlotIndex] == nullptr;
}

int32 UInventoryComponent::GetWeaponCount() const
{
	int32 Count = 0;
	for (const AWeaponBase *Weapon : WeaponSlots)
	{
		if (Weapon)
		{
			Count++;
		}
	}
	return Count;
}

void UInventoryComponent::CycleToNextWeapon()
{
	int32 NextSlot = FindNextWeaponSlot();
	if (NextSlot >= 0)
	{
		EquipWeapon(NextSlot);
	}
}

void UInventoryComponent::CycleToPreviousWeapon()
{
	int32 PrevSlot = FindPreviousWeaponSlot();
	if (PrevSlot >= 0)
	{
		EquipWeapon(PrevSlot);
	}
}

void UInventoryComponent::GetAllWeapons(TArray<AWeaponBase *> &OutWeapons) const
{
	OutWeapons = WeaponSlots;
}

void UInventoryComponent::InitializeDefaultWeapons()
{
	AActor *Owner = GetOwner();
	if (!Owner || Owner->GetLocalRole() != ROLE_Authority || !OwnerCharacter)
	{
		UE_LOG(LogGASDebug, Error, TEXT("InitializeDefaultWeapons: BLOCKED - No owner or authority"));
		return;
	}

	UE_LOG(LogGASDebug, Warning, TEXT("InitializeDefaultWeapons: Starting weapon initialization"));

	// Spawn Revolver in slot 0
	if (DefaultRevolverClass)
	{
		UE_LOG(LogGASDebug, Warning, TEXT("InitializeDefaultWeapons: Spawning Revolver"));

		AWeaponBase *Revolver = SpawnWeapon(DefaultRevolverClass);
		if (Revolver)
		{
			UE_LOG(LogGASDebug, Warning, TEXT("InitializeDefaultWeapons: Adding Revolver to slot 0"));
			AddWeaponToSlot(Revolver, (int32)EWeaponSlot::Revolver);
		}
		else
		{
			UE_LOG(LogGASDebug, Error, TEXT("InitializeDefaultWeapons: Failed to spawn Revolver!"));
		}
	}
	else
	{
		UE_LOG(LogGASDebug, Error, TEXT("InitializeDefaultWeapons: DefaultRevolverClass is NULL!"));
	}

	// Spawn Lasso in slot 1
	if (DefaultLassoClass)
	{
		AWeaponBase *Lasso = SpawnWeapon(DefaultLassoClass);
		if (Lasso)
		{
			AddWeaponToSlot(Lasso, (int32)EWeaponSlot::Lasso);
		}
	}

	// Spawn Dynamite in slot 2
	if (DefaultDynamiteClass)
	{
		AWeaponBase *Dynamite = SpawnWeapon(DefaultDynamiteClass);
		if (Dynamite)
		{
			AddWeaponToSlot(Dynamite, (int32)EWeaponSlot::Dynamite);
		}
	}

	// Spawn Trumpet in slot 3
	if (DefaultTrumpetClass)
	{
		AWeaponBase *Trumpet = SpawnWeapon(DefaultTrumpetClass);
		if (Trumpet)
		{
			AddWeaponToSlot(Trumpet, (int32)EWeaponSlot::Trumpet);
		}
	}

	// Equip revolver by default
	EquipWeapon((int32)EWeaponSlot::Revolver);
}

void UInventoryComponent::SetDefaultWeaponClasses(TSubclassOf<AWeaponBase> RevolverClass, TSubclassOf<AWeaponBase> LassoClass, TSubclassOf<AWeaponBase> DynamiteClass, TSubclassOf<AWeaponBase> TrumpetClass)
{
	DefaultRevolverClass = RevolverClass;
	DefaultLassoClass = LassoClass;
	DefaultDynamiteClass = DynamiteClass;
	DefaultTrumpetClass = TrumpetClass;
}

AWeaponBase *UInventoryComponent::SpawnWeapon(TSubclassOf<AWeaponBase> WeaponClass)
{
	if (!WeaponClass || !OwnerCharacter)
	{
		return nullptr;
	}

	AWeaponBase *NewWeapon = GetWorld()->SpawnActor<AWeaponBase>(WeaponClass);
	if (NewWeapon)
	{
		NewWeapon->SetOwner(OwnerCharacter);
	}

	return NewWeapon;
}

void UInventoryComponent::ServerEquipWeapon_Implementation(int32 SlotIndex)
{
	EquipWeapon(SlotIndex);
}

void UInventoryComponent::ServerDropWeapon_Implementation()
{
	DropEquippedWeapon();
}

void UInventoryComponent::OnRep_WeaponSlots()
{
	// Called on clients when the WeaponSlots array is replicated
	// This ensures that newly replicated weapons trigger their blueprint events on the client

	// If we have an equipped weapon from replication, make sure to call its EquipWeapon event
	if (EquippedWeapon)
	{
		EquippedWeapon->EquipWeapon();
	}

	// Broadcast the weapon added event to trigger any UI updates
	OnWeaponAdded.Broadcast();
}

void UInventoryComponent::OnRep_EquippedWeapon()
{
	// Called on clients when the EquippedWeapon pointer is replicated
	// This ensures the weapon's blueprint EquipWeapon event fires on the client

	// If there's an equipped weapon, call EquipWeapon to fire the blueprint event
	if (EquippedWeapon)
	{
		UE_LOG(LogGASDebug, Log, TEXT("OnRep_EquippedWeapon: Calling EquipWeapon on replicated weapon %s"), *EquippedWeapon->GetName());
		EquippedWeapon->EquipWeapon();
		OnWeaponEquipped.Broadcast();
	}
	else
	{
		// No equipped weapon (empty slot)
		OnWeaponUnequipped.Broadcast();
	}
}

int32 UInventoryComponent::FindNextWeaponSlot() const
{
	if (CurrentEquippedSlot == -1)
	{
		// No weapon equipped, find first available
		for (int32 i = 0; i < (int32)EWeaponSlot::MaxSlots; i++)
		{
			if (WeaponSlots[i] != nullptr)
			{
				return i;
			}
		}
		return -1;
	}

	// Find next non-empty slot after current
	for (int32 i = CurrentEquippedSlot + 1; i < (int32)EWeaponSlot::MaxSlots; i++)
	{
		if (WeaponSlots[i] != nullptr)
		{
			return i;
		}
	}

	// Wrap around to beginning
	for (int32 i = 0; i < CurrentEquippedSlot; i++)
	{
		if (WeaponSlots[i] != nullptr)
		{
			return i;
		}
	}

	return -1;
}

int32 UInventoryComponent::FindPreviousWeaponSlot() const
{
	if (CurrentEquippedSlot == -1)
	{
		// No weapon equipped, find last available
		for (int32 i = (int32)EWeaponSlot::MaxSlots - 1; i >= 0; i--)
		{
			if (WeaponSlots[i] != nullptr)
			{
				return i;
			}
		}
		return -1;
	}

	// Find previous non-empty slot before current
	for (int32 i = CurrentEquippedSlot - 1; i >= 0; i--)
	{
		if (WeaponSlots[i] != nullptr)
		{
			return i;
		}
	}

	// Wrap around to end
	for (int32 i = (int32)EWeaponSlot::MaxSlots - 1; i > CurrentEquippedSlot; i--)
	{
		if (WeaponSlots[i] != nullptr)
		{
			return i;
		}
	}

	return -1;
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UInventoryComponent, WeaponSlots);
	DOREPLIFETIME(UInventoryComponent, EquippedWeapon);
	DOREPLIFETIME(UInventoryComponent, CurrentEquippedSlot);
}

void UInventoryComponent::GrantWeaponAbilities(AWeaponBase *Weapon)
{
	if (!Weapon || !OwnerCharacter)
	{
		return;
	}

	UCattleAbilitySystemComponent *ASC = Cast<UCattleAbilitySystemComponent>(OwnerCharacter->GetAbilitySystemComponent());
	if (!ASC)
	{
		UE_LOG(LogGASDebug, Error, TEXT("GrantWeaponAbilities: No ASC found on owner character"));
		return;
	}

	// Clear any existing handles
	Weapon->GrantedAbilityHandles.Empty();

	// Generate InputIDs starting from a high offset to avoid conflicts with character abilities
	// Character abilities use 0-20ish, weapon abilities use 100+
	static int32 NextWeaponInputID = 100;

	for (const FWeaponAbilityInfo &AbilityInfo : Weapon->GetWeaponAbilities())
	{
		if (!AbilityInfo.IsValid())
		{
			continue;
		}

		// Generate unique InputID for this ability
		int32 InputID = NextWeaponInputID++;

		// Create ability spec with input binding
		FGameplayAbilitySpec AbilitySpec(AbilityInfo.AbilityClass, 1, InputID);

		// Grant the ability
		FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(AbilitySpec);
		Weapon->GrantedAbilityHandles.Add(Handle);

		UE_LOG(LogGASDebug, Log, TEXT("GrantWeaponAbilities: Granted %s with InputID %d (Handle: %s)"),
			   *GetNameSafe(AbilityInfo.AbilityClass),
			   InputID,
			   *Handle.ToString());

		// Bind input action if specified
		if (AbilityInfo.InputAction)
		{
			BindWeaponAbilityInput(AbilityInfo.InputAction, InputID);
		}
	}
}

void UInventoryComponent::RevokeWeaponAbilities(AWeaponBase *Weapon)
{
	if (!Weapon || !OwnerCharacter)
	{
		return;
	}

	UCattleAbilitySystemComponent *ASC = Cast<UCattleAbilitySystemComponent>(OwnerCharacter->GetAbilitySystemComponent());
	if (!ASC)
	{
		return;
	}

	// Revoke all abilities granted by this weapon
	for (const FGameplayAbilitySpecHandle &Handle : Weapon->GrantedAbilityHandles)
	{
		if (Handle.IsValid())
		{
			ASC->ClearAbility(Handle);
			UE_LOG(LogGASDebug, Log, TEXT("RevokeWeaponAbilities: Cleared ability (Handle: %s)"), *Handle.ToString());
		}
	}

	// Unbind weapon ability inputs
	UnbindWeaponAbilityInputs();

	// Clear the handles
	Weapon->GrantedAbilityHandles.Empty();
}

void UInventoryComponent::BindWeaponAbilityInput(UInputAction *InputAction, int32 InputID)
{
	if (!InputAction || !OwnerCharacter)
	{
		return;
	}

	APlayerController *PC = Cast<APlayerController>(OwnerCharacter->GetController());
	if (!PC)
	{
		return;
	}

	UEnhancedInputComponent *EnhancedInputComponent = Cast<UEnhancedInputComponent>(PC->InputComponent);
	if (!EnhancedInputComponent)
	{
		return;
	}

	// Store handle for cleanup
	FInputBindingHandle PressedHandle = EnhancedInputComponent->BindAction(InputAction, ETriggerEvent::Started,
																		   OwnerCharacter, &ACattleCharacter::OnAbilityInputPressed, InputID);
	FInputBindingHandle ReleasedHandle = EnhancedInputComponent->BindAction(InputAction, ETriggerEvent::Completed,
																			OwnerCharacter, &ACattleCharacter::OnAbilityInputReleased, InputID);

	WeaponInputBindingHandles.Add(PressedHandle.GetHandle());
	WeaponInputBindingHandles.Add(ReleasedHandle.GetHandle());

	UE_LOG(LogGASDebug, Log, TEXT("BindWeaponAbilityInput: Bound %s to InputID %d"),
		   *InputAction->GetName(), InputID);
}

void UInventoryComponent::UnbindWeaponAbilityInputs()
{
	if (!OwnerCharacter)
	{
		return;
	}

	APlayerController *PC = Cast<APlayerController>(OwnerCharacter->GetController());
	if (!PC)
	{
		return;
	}

	UEnhancedInputComponent *EnhancedInputComponent = Cast<UEnhancedInputComponent>(PC->InputComponent);
	if (!EnhancedInputComponent)
	{
		return;
	}

	// Remove all weapon input bindings
	for (uint32 Handle : WeaponInputBindingHandles)
	{
		EnhancedInputComponent->RemoveBindingByHandle(Handle);
	}
	WeaponInputBindingHandles.Empty();

	UE_LOG(LogGASDebug, Log, TEXT("UnbindWeaponAbilityInputs: Cleared all weapon input bindings"));
}
