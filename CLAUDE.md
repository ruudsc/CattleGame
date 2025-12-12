# CattleGame Development Notes

## GAS System - Lessons Learned

### Blueprint Events vs C++ Implementations

**The Pattern:**
When you have a `UFUNCTION(BlueprintImplementableEvent)`, Unreal creates TWO functions:
1. **Base function** - Triggers the blueprint event (e.g., `OnWeaponEquipped()`)
2. **Implementation function** - For C++ code (e.g., `OnWeaponEquipped_Implementation()`)

**The Problem:**
If you only call the base function, the blueprint event fires but the C++ implementation **never gets called**. This is because blueprint events can be implemented entirely in Blueprint, so the C++ implementation is optional.

**The Solution:**
When you need C++ logic to ALWAYS run (even if blueprints override it), you must:

1. **Override the base function** with `virtual void FunctionName() override`
2. **Call Super::FunctionName()** to fire the blueprint event
3. **Manually call the Implementation function** to ensure C++ logic runs

**Example (Weapon Fire):**
```cpp
// In Revolver.h
virtual void Fire() override;

// In Revolver.cpp
void ARevolver::Fire()
{
    // Fire the blueprint event
    Super::Fire();

    // Explicitly call C++ implementation to ensure ammo is consumed
    ACattleCharacter *CurrentOwner = OwnerCharacter ? OwnerCharacter : Cast<ACattleCharacter>(GetOwner());
    USkeletalMeshComponent *ActiveMesh = CurrentOwner ? CurrentOwner->GetActiveCharacterMesh() : nullptr;
    OnWeaponFired_Implementation(CurrentOwner, ActiveMesh);
}
```

**When to Use This Pattern:**
- Weapon firing - Ammo must always be consumed
- Equipping weapons - State must always be updated
- Any critical game logic - When C++ code must run regardless of blueprint implementations

---

### Virtual Functions and Override Specifier

**Problem:** "Method with override specifier did not override any base class methods"

**Solution:** Always mark weapon functions as `virtual` in the base class:

```cpp
// WeaponBase.h
UFUNCTION(BlueprintCallable, Category = "Weapon")
virtual void EquipWeapon();

UFUNCTION(BlueprintCallable, Category = "Weapon")
virtual void Fire();

UFUNCTION(BlueprintCallable, Category = "Weapon")
virtual void Reload();
```

This allows child classes to override them:

```cpp
// Revolver.h
virtual void EquipWeapon() override;
virtual void Fire() override;
```

---

### Ability System - Character Owner Caching

**Problem:** "GetWeapon_Implementation: CachedCharacterOwner is NULL"

**Root Cause:** The ability cached the character owner on `ActivateAbility()`, but when the ability was reused or called again, the cache became stale.

**Solution - Multi-Level Fallback:**

```cpp
AWeaponBase* UGameplayAbility_Weapon::GetWeapon_Implementation() const
{
    // Try cached character owner first
    ACattleCharacter* CharacterOwner = CachedCharacterOwner;

    // If cached owner is NULL, try to get it from the ability's actor info
    if (!CharacterOwner && CurrentActorInfo)
    {
        CharacterOwner = Cast<ACattleCharacter>(CurrentActorInfo->OwnerActor.Get());
    }

    if (!CharacterOwner)
    {
        UE_LOG(LogGASDebug, Error, TEXT("GetWeapon_Implementation: CharacterOwner could not be resolved"));
        return nullptr;
    }

    // Now safely use CharacterOwner...
    if (UInventoryComponent *Inventory = CharacterOwner->GetInventoryComponent())
    {
        return Inventory->GetEquippedWeapon();
    }

    return nullptr;
}
```

**Why This Works:**
- `CurrentActorInfo` is always available during ability execution
- It contains the current actor (owner) information
- It's more reliable than relying on cached values alone

---

### Ammo Not Being Consumed - Weapon Firing Issue

**Problem:** Shots were being fired but ammo never decreased from 6/6

**Investigation Flow:**
1. CanActivateAbility() ✓ returned TRUE
2. ActivateAbility() ✓ was called
3. FireWeapon() ✓ was called
4. Weapon->Fire() ✓ was called
5. **OnWeaponFired_Implementation() ✗ NEVER CALLED**

**Root Cause:**
`Fire()` from `WeaponBase` only triggered the blueprint event. Without explicitly calling the implementation function, the C++ ammo-consumption code never ran.

**Solution:**
Override `Fire()` in Revolver to manually call the implementation:

```cpp
void ARevolver::Fire()
{
    Super::Fire();  // Fire the blueprint event

    // Manually call implementation to ensure ammo is consumed
    ACattleCharacter *CurrentOwner = OwnerCharacter ? OwnerCharacter : Cast<ACattleCharacter>(GetOwner());
    USkeletalMeshComponent *ActiveMesh = CurrentOwner ? CurrentOwner->GetActiveCharacterMesh() : nullptr;
    OnWeaponFired_Implementation(CurrentOwner, ActiveMesh);
}
```

---

### Reload Check - CanReload Logic

**Expected Behavior:** You can only reload when ammo is not full (< MaxAmmo)

**Implementation:**
```cpp
bool ARevolver::CanReload() const
{
    if (bIsReloading)
    {
        return false;  // Already reloading
    }

    // Only reload if ammo is not full
    if (CurrentAmmo < MaxAmmo)
    {
        return true;
    }

    return false;
}
```

**Debug Logs Show:**
```
Reload Ability: Revolver->CanReload() = 0, ammo: 6/6, bIsReloading: FALSE
```

This is CORRECT - you have full ammo so reload is blocked. Fire some shots first to reduce ammo, then reload will be available.

---

## Weapon State Management

**Critical State Variables:**
```cpp
UPROPERTY(Replicated)
bool bIsEquipped = false;      // Is weapon currently equipped?

UPROPERTY(Replicated)
bool bIsReloading = false;     // Is weapon in reload animation?

UPROPERTY(Replicated)
int32 CurrentAmmo = 6;         // Current ammo in magazine

float LastFireTime = -9999.0f; // For fire rate limiting (non-replicated)
```

**State Transition Example:**
```
Start: CurrentAmmo=6, bIsEquipped=TRUE, bIsReloading=FALSE
Fire 1: CurrentAmmo=5
Fire 2: CurrentAmmo=4
Fire 3: CurrentAmmo=3
Reload: bIsReloading=TRUE
Wait...
Reload Complete: CurrentAmmo=6, bIsReloading=FALSE
```

---

## Logging Strategy for GAS Debugging

**Always Log:**
- When CanActivateAbility() blocks (with reason)
- State BEFORE and AFTER changes
- When functions are called
- State being checked (ammo, flags, timers)

**Example from Fire Ability:**
```cpp
void UGameplayAbility_WeaponFire::ActivateAbility(...)
{
    UE_LOG(LogGASDebug, Warning, TEXT("Fire Ability: ActivateAbility called"));
    Super::ActivateAbility(...);

    UE_LOG(LogGASDebug, Warning, TEXT("Fire Ability: Calling FireWeapon()"));
    FireWeapon();
    UE_LOG(LogGASDebug, Warning, TEXT("Fire Ability: FireWeapon() completed"));
}
```

This creates a clear audit trail showing exactly where the flow breaks.

---

## The Three-Layer GAS Pattern

```
┌─────────────────────────────────────────────────────────────┐
│ Layer 1: Ability System (GameplayAbility_WeaponFire.cpp)   │
│ - CanActivateAbility() - Check prerequisites               │
│ - ActivateAbility() - Cache owners, call ability logic     │
│ - FireWeapon() - Get weapon, call weapon->Fire()           │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│ Layer 2: Weapon Interface (Revolver.cpp)                   │
│ - Fire() override - Call base event + implementation       │
│ - Calls OnWeaponFired_Implementation()                      │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│ Layer 3: Weapon Logic (OnWeaponFired_Implementation)       │
│ - Consume ammo                                              │
│ - Perform line trace                                        │
│ - Apply damage                                              │
└─────────────────────────────────────────────────────────────┘
```

**Key Principle:** Each layer must explicitly trigger the next layer. Don't rely on implicit behavior.

---

## Quick Debugging Checklist

When weapon abilities don't work:

- [ ] Is `CanActivateAbility()` returning TRUE? (Check logs)
- [ ] Is `ActivateAbility()` being called? (Check logs)
- [ ] Is the actual action function being called? (Check logs in FireWeapon, StartReload, etc.)
- [ ] Are the _Implementation functions being called? (Override the base function and call explicitly)
- [ ] Are state variables replicated? (Check DOREPLIFETIME)
- [ ] Are state variables being updated correctly? (Check BEFORE/AFTER logs)
- [ ] Is the character owner available when needed? (Use fallback mechanisms)
- [ ] Did you add logging to identify the break point? (Essential!)

---

## Future Weapons

When adding new weapons (Lasso, Dynamite, etc.), remember:

1. **Override critical functions** - Fire(), EquipWeapon(), Reload()
2. **Call Super::FunctionName()** first to fire blueprint events
3. **Manually call _Implementation()** to ensure C++ logic runs
4. **Implement CanFire() and CanReload()** with proper state checks
5. **Replicate weapon state** - bIsEquipped, bIsReloading, ammo counts
6. **Add logging** at state transitions for debugging

---

## GAS Implementation - Major Lessons Learned

### Input System Architecture: Tag-Based vs InputID-Based Activation

**Initial Approach (Tag-Based):**
```cpp
// Character had individual callback functions
void Fire() { ActivateAbilityByTag(Input_Weapon_Fire); }
void Reload() { ActivateAbilityByTag(Input_Weapon_Reload); }
void Move() { AddMovementInput(...); }
```

**Problems:**
1. Three overlapping input systems (Enhanced Input Actions, EAbilityInputID enum, Input gameplay tags)
2. Abilities were granted/removed per weapon in InventoryComponent
3. Mixing direct character callbacks with GAS activation
4. No clear pattern for continuous input (Move, Look)

**Final Approach (InputID-Based + Data Asset):**
```cpp
// PlayerGameplayAbilitiesDataAsset.h
USTRUCT(BlueprintType)
struct FGameplayInputAbilityInfo
{
    TSubclassOf<UGameplayAbility> GameplayAbilityClass;
    TObjectPtr<UInputAction> InputAction;
    int32 InputID; // Auto-generated
};

// CattleCharacter.cpp
void ACattleCharacter::InitAbilitySystem()
{
    // Grant ALL abilities once at initialization
    for (const FGameplayInputAbilityInfo& AbilityInfo : DataAsset->GetInputAbilities())
    {
        AbilitySystemComponent->GiveAbility(
            FGameplayAbilitySpec(AbilityInfo.GameplayAbilityClass, 1, AbilityInfo.InputID));
    }
}

void ACattleCharacter::OnAbilityInputPressed(int32 InputID)
{
    AbilitySystemComponent->AbilityLocalInputPressed(InputID);
}
```

**Benefits:**
1. Single source of truth: Data Asset maps Input Actions → Abilities → InputIDs
2. Auto-generated InputIDs eliminate manual enum maintenance
3. Abilities granted once at initialization, persistent across weapon switches
4. Cleaner character code: No weapon-specific callbacks
5. Data-driven: Add new abilities without code changes

**Key Insight:** InputID-based activation is simpler and more maintainable than tag-based activation for input handling. Tags are better for ability state management (State_Weapon_Firing, State_Weapon_Reloading).

---

### Continuous Input Handling (Triggered Events)

**Problem:** Move and Look require frame-by-frame input updates (ETriggerEvent::Triggered), but GAS abilities only get Started/Completed events by default.

**Initial Attempts:**
- Using Started/Completed with持续 activation ❌ (abilities end immediately)
- Keeping abilities active indefinitely ❌ (prevents other abilities)

**Solution - GameplayAbility_TriggeredInput Base Class:**
```cpp
// GameplayAbility_TriggeredInput.h
class UGameplayAbility_TriggeredInput : public UGameplayAbility
{
protected:
    virtual void ActivateAbility(...) override;
    virtual void EndAbility(...) override;

    // Override this in child classes
    virtual void OnTriggeredInputAction_Implementation(const FInputActionValue& Value);

private:
    FDelegateHandle TriggeredDelegateHandle;
};

// GameplayAbility_TriggeredInput.cpp
void UGameplayAbility_TriggeredInput::ActivateAbility(...)
{
    Super::ActivateAbility(...);

    // Dynamically bind to Triggered events during ability lifetime
    if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(Character->InputComponent))
    {
        TriggeredDelegateHandle = EIC->BindAction(
            InputAction, ETriggerEvent::Triggered,
            this, &UGameplayAbility_TriggeredInput::OnTriggeredInputAction).GetHandle();
    }
}

void UGameplayAbility_TriggeredInput::EndAbility(...)
{
    // Clean up triggered binding
    if (UEnhancedInputComponent* EIC = ...)
    {
        EIC->RemoveBindingByHandle(TriggeredDelegateHandle);
    }

    Super::EndAbility(...);
}
```

**Child Classes:**
```cpp
// GameplayAbility_Move.cpp
void UGameplayAbility_Move::OnTriggeredInputAction_Implementation(const FInputActionValue& Value)
{
    const FVector2D MovementVector = Value.Get<FVector2D>();
    Character->AddMovementInput(Character->GetActorForwardVector(), MovementVector.Y);
    Character->AddMovementInput(Character->GetActorRightVector(), MovementVector.X);
}
```

**Key Lessons:**
1. Started/Completed events are for discrete actions (Fire, Reload, Jump)
2. Triggered events are for continuous input (Move, Look, Scroll)
3. Abilities can dynamically bind to input during their lifetime
4. Always clean up dynamic bindings in EndAbility()
5. Base classes can handle boilerplate (binding/unbinding), children implement logic

---

### Ability System Architecture: Two-Tier Design

**Overview:**
The ability system uses a two-tier architecture with clear separation between character and weapon abilities:

| Category | Source | When Granted | InputID Range | Examples |
|----------|--------|--------------|---------------|----------|
| **Character Abilities** | `CattleCharacter::CharacterAbilities` | Once at `InitAbilitySystem()` | 1000-1999 | Move, Look, Jump, Crouch, EquipSlot |
| **Weapon Abilities** | `WeaponBase::WeaponAbilities` | On weapon equip | 100-999 | Fire, Reload, LassoFire, DynamiteThrow |

**Character Abilities (1000-1999):**
- Defined directly on `ACattleCharacter` via `TArray<FCharacterAbilityInfo> CharacterAbilities`
- Granted once in `InitAbilitySystem()`, never removed
- Core capabilities: movement, camera, jumping, crouching, weapon slot switching
- InputIDs auto-generated in editor via `PostEditChangeProperty`

**Weapon Abilities (100-999):**
- Defined on each weapon via `TArray<FWeaponAbilityInfo> WeaponAbilities`
- Granted by `InventoryComponent::GrantWeaponAbilities()` on equip
- Revoked by `InventoryComponent::RevokeWeaponAbilities()` on unequip
- Examples: GA_WeaponFire, GA_WeaponReload, GA_LassoFire, GA_DynamiteThrow

**Why This Separation?**
1. **Clear ownership** - Character owns movement, weapons own combat actions
2. **No InputID conflicts** - Separate ranges (1000+ vs 100+)
3. **Dynamic weapon binding** - Weapon abilities swap with equipped weapon
4. **Extensible** - Add new character abilities without touching weapons

---

### Ability Persistence vs Per-Weapon Granting

**Anti-Pattern (Initial Implementation):**
```cpp
// InventoryComponent::EquipWeapon()
void UInventoryComponent::EquipWeapon(int32 SlotIndex)
{
    // Remove old weapon abilities
    if (EquippedWeapon)
    {
        ASC->ClearAbility(GrantedFireAbilityHandle);
        ASC->ClearAbility(GrantedReloadAbilityHandle);
    }

    // Grant new weapon abilities
    GrantedFireAbilityHandle = ASC->GiveAbility(FireAbilityClass);
    GrantedReloadAbilityHandle = ASC->GiveAbility(ReloadAbilityClass);
}
```

**Problems:**
1. Abilities granted/removed on every weapon switch
2. Tracking ability handles across weapon switches
3. Potential for ability handles to become stale
4. Abilities lost if weapon is removed
5. Unnecessary complexity in InventoryComponent

**Correct Pattern:**
```cpp
// CattleCharacter::InitAbilitySystem()
void ACattleCharacter::InitAbilitySystem()
{
    // Grant character abilities ONCE at initialization (1000+ range)
    // Move, Look, Jump, Crouch, EquipSlot[1-6]
    for (const FCharacterAbilityInfo& AbilityInfo : CharacterAbilities)
    {
        AbilitySystemComponent->GiveAbility(
            FGameplayAbilitySpec(AbilityInfo.GameplayAbilityClass, 1, AbilityInfo.InputID));
    }
}

// Weapon abilities granted separately via InventoryComponent (100+ range)
// InventoryComponent::GrantWeaponAbilities() on equip
// InventoryComponent::RevokeWeaponAbilities() on unequip

// GameplayAbility_WeaponFire::CanActivateAbility()
bool UGameplayAbility_WeaponFire::CanActivateAbility(...) const
{
    // Ability checks weapon availability at activation time
    AWeaponBase* Weapon = Inventory->GetEquippedWeapon();
    if (!Weapon)
    {
        return false; // No weapon equipped
    }

    return Weapon->CanFire();
}
```

**Key Insights:**
1. **Abilities are capabilities, not weapon properties** - A character always has the capability to fire/reload, they just need a weapon to do it
2. **CanActivateAbility() is the gatekeeper** - Block abilities when weapon is unavailable or in wrong state
3. **Simpler state management** - No ability handle tracking needed
4. **Cleaner separation** - InventoryComponent only manages weapons, Character manages abilities
5. **Better for multiplayer** - Abilities replicate once, not on every weapon switch

---

### Weapon State Queries: When and Where

**Problem:** Abilities need to check weapon state (ammo, reload status) but GetEquippedWeapon() might return nullptr.

**Layered Checking Pattern:**
```cpp
// Layer 1: CanActivateAbility (const function, called frequently)
bool UGameplayAbility_WeaponFire::CanActivateAbility(...) const
{
    // Get character from ActorInfo (NOT CachedCharacterOwner)
    ACattleCharacter* Character = ActorInfo ? Cast<ACattleCharacter>(ActorInfo->OwnerActor.Get()) : nullptr;
    if (!Character) return false;

    UInventoryComponent* Inventory = Character->GetInventoryComponent();
    if (!Inventory) return false;

    // Check weapon exists
    AWeaponBase* Weapon = Inventory->GetEquippedWeapon();
    if (!Weapon) return false;

    // Check weapon-specific state
    if (ARevolver* Revolver = Cast<ARevolver>(Weapon))
    {
        return Revolver->CanFire();
    }

    return true;
}

// Layer 2: ActivateAbility (caches character owner)
void UGameplayAbility_WeaponFire::ActivateAbility(...)
{
    Super::ActivateAbility(...);
    FireWeapon();
}

// Layer 3: FireWeapon (performs action)
void UGameplayAbility_WeaponFire::FireWeapon()
{
    AWeaponBase* Weapon = GetWeapon_Implementation();
    if (!Weapon)
    {
        EndAbility(..., true, true); // Cancel
        return;
    }

    Weapon->Fire();
    EndAbility(..., true, false); // Success
}
```

**Key Principles:**
1. **CanActivateAbility() must be const** - Use ActorInfo, not cached values
2. **Always null-check at every layer** - Even if CanActivateAbility passed, state can change
3. **Fail gracefully** - End ability with bCancelled=true if state is invalid
4. **Weapon-specific logic in weapon class** - CanFire(), CanReload() belong in weapon, not ability
5. **Multi-level fallback** - Try cached owner, then ActorInfo, then fail

---

### Input Action Binding Strategy

**Complete Migration Pattern:**
```cpp
// CattleCharacter::SetupPlayerInputComponent()
void ACattleCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        if (PlayerGameplayAbilitiesDataAsset)
        {
            const TSet<FGameplayInputAbilityInfo>& InputAbilities =
                PlayerGameplayAbilitiesDataAsset->GetInputAbilities();

            for (const FGameplayInputAbilityInfo& AbilityInfo : InputAbilities)
            {
                if (AbilityInfo.IsValid())
                {
                    // Bind Started event
                    EIC->BindAction(AbilityInfo.InputAction, ETriggerEvent::Started,
                        this, &ACattleCharacter::OnAbilityInputPressed, AbilityInfo.InputID);

                    // Bind Completed event
                    EIC->BindAction(AbilityInfo.InputAction, ETriggerEvent::Completed,
                        this, &ACattleCharacter::OnAbilityInputReleased, AbilityInfo.InputID);
                }
            }
        }
    }
}
```

**What Gets Removed:**
```cpp
// DELETE these properties
UPROPERTY(EditAnywhere, Category = "Input")
UInputAction* FireAction; // ❌

UPROPERTY(EditAnywhere, Category = "Input")
UInputAction* MoveAction; // ❌

// DELETE these callback functions
void Fire() { ... } // ❌
void Move(const FInputActionValue& Value) { ... } // ❌
```

**Result:**
- **Before:** 13+ input properties, 13+ callback functions
- **After:** 1 Data Asset property, 2 forwarding functions (OnAbilityInputPressed/Released)

**Key Benefits:**
1. Character code shrinks dramatically
2. Adding new abilities = edit Data Asset only (no code changes)
3. Input Actions automatically bound from Data Asset
4. No risk of forgetting to unbind callbacks
5. Consistent pattern for all abilities

---

### Scroll Wheel Spam Prevention

**Problem:** Mouse scroll wheel generates rapid events. Without throttling, weapon switching becomes uncontrollable.

**Solution - Cooldown in TriggeredInput Ability:**
```cpp
// GameplayAbility_SwitchWeapon.h
class UGameplayAbility_SwitchWeapon : public UGameplayAbility_TriggeredInput
{
private:
    float LastSwitchTime;

    UPROPERTY(EditAnywhere, Category = "Weapon Switch")
    float SwitchCooldown = 0.2f; // 200ms
};

// GameplayAbility_SwitchWeapon.cpp
void UGameplayAbility_SwitchWeapon::OnTriggeredInputAction_Implementation(const FInputActionValue& Value)
{
    const float AxisValue = Value.Get<float>();

    // Ignore noise
    if (FMath::Abs(AxisValue) < 0.1f) return;

    // Check cooldown
    const float CurrentTime = GetWorld()->GetTimeSeconds();
    if (CurrentTime - LastSwitchTime < SwitchCooldown) return;

    LastSwitchTime = CurrentTime;

    // Perform switch
    if (AxisValue > 0.0f)
        Inventory->CycleToNextWeapon();
    else if (AxisValue < 0.0f)
        Inventory->CycleToPreviousWeapon();
}
```

**Key Lessons:**
1. **Axis input needs noise filtering** - Ignore values < threshold (0.1f)
2. **Time-based cooldowns prevent spam** - 200ms is good for scroll wheel
3. **Store timing state in ability** - LastSwitchTime persists across triggers
4. **World time for cooldowns** - Use GetWorld()->GetTimeSeconds(), not delta time
5. **Per-ability cooldown tuning** - Different abilities need different cooldown values

---

### Multiple Input Methods for Same Action

**Use Case:** Weapon switching via scroll wheel (cycle) OR number keys (direct selection)

**Pattern - Two Separate Abilities:**
```cpp
// Method 1: Scroll Wheel - GameplayAbility_SwitchWeapon
// - Inherits from TriggeredInput (continuous axis input)
// - Calls CycleToNextWeapon() / CycleToPreviousWeapon()
// - Includes cooldown to prevent spam
// - Bound to Triggered event

// Method 2: Number Keys - GameplayAbility_EquipWeaponSlot
// - Inherits from GameplayAbility (discrete button press)
// - Each instance configured with SlotIndexToEquip (0-5)
// - Calls EquipWeapon(SlotIndexToEquip) directly
// - Bound to Started event
// - 6 instances in Data Asset (one per key)
```

**Data Asset Configuration:**
```
Ability 1: SwitchWeapon    → MouseWheel  → InputID=7  (Triggered)
Ability 2: EquipSlot0      → Key1        → InputID=8  (Started)
Ability 3: EquipSlot1      → Key2        → InputID=9  (Started)
Ability 4: EquipSlot2      → Key3        → InputID=10 (Started)
Ability 5: EquipSlot3      → Key4        → InputID=11 (Started)
Ability 6: EquipSlot4      → Key5        → InputID=12 (Started)
Ability 7: EquipSlot5      → Key6        → InputID=13 (Started)
```

**Key Insights:**
1. **Different input types = different ability types** - Axis vs Button
2. **Multiple ability instances is fine** - 6 EquipWeaponSlot instances with different SlotIndexToEquip
3. **InventoryComponent provides both APIs** - CycleToNextWeapon() and EquipWeapon(Index)
4. **EditDefaultsOnly properties configure instances** - SlotIndexToEquip set in Data Asset
5. **User can choose their preferred method** - Scroll for convenience, numbers for precision

---

### Empty Slot Handling (Unarmed State)

**Design Decision:** Allow equipping empty slots to create "holstered" or "unarmed" state.

**Implementation:**
```cpp
// InventoryComponent::EquipWeapon()
bool UInventoryComponent::EquipWeapon(int32 SlotIndex)
{
    // Get weapon from slot (may be nullptr for empty slots)
    AWeaponBase* WeaponToEquip = WeaponSlots[SlotIndex];

    // Already equipped (including if both are nullptr)
    if (WeaponToEquip == EquippedWeapon) return false;

    // Unequip current weapon
    if (EquippedWeapon)
    {
        EquippedWeapon->UnequipWeapon();
    }

    // Equip new weapon (or set to nullptr for empty slot)
    EquippedWeapon = WeaponToEquip;
    CurrentEquippedSlot = SlotIndex;

    if (WeaponToEquip)
    {
        EquippedWeapon->EquipWeapon();
        OnWeaponEquipped.Broadcast();
    }
    else
    {
        // Empty slot = unarmed state
        OnWeaponUnequipped.Broadcast();
    }

    return true;
}
```

**Ability System Integration:**
```cpp
// Fire/Reload abilities automatically block when nullptr
bool UGameplayAbility_WeaponFire::CanActivateAbility(...) const
{
    AWeaponBase* Weapon = Inventory->GetEquippedWeapon();
    if (!Weapon) return false; // Blocks firing when unarmed

    return Weapon->CanFire();
}
```

**Cycling Behavior:**
```cpp
// Scroll wheel skips empty slots (only cycles through weapons)
int32 UInventoryComponent::FindNextWeaponSlot() const
{
    for (int32 i = CurrentEquippedSlot + 1; i < MaxSlots; i++)
    {
        if (WeaponSlots[i] != nullptr) // Skip empty
        {
            return i;
        }
    }
    return -1;
}

// Number keys select any slot (including empty ones)
// Ability: EquipWeaponSlot → EquipWeapon(SlotIndex) → Allows nullptr
```

**Key Principles:**
1. **CurrentEquippedSlot tracks slot index, not weapon validity** - Can be 0-5 even if slot is empty
2. **EquippedWeapon = nullptr is a valid state** - Represents "unarmed"
3. **Different input methods, different behaviors** - Scroll skips empty, keys select any
4. **Event differentiation** - OnWeaponEquipped vs OnWeaponUnequipped based on weapon presence
5. **Abilities naturally block** - nullptr checks in CanActivateAbility() prevent unarmed shooting

---

## Inventory System - Empty Slot Equipping

**Feature:** Players can equip empty weapon slots to holster their weapon and show an "unarmed" or "idle hands" state.

**How It Works:**
When an empty slot is equipped, the `InventoryComponent` sets `EquippedWeapon` to `nullptr` and broadcasts `OnWeaponUnequipped` instead of `OnWeaponEquipped`.

**Implementation:**
```cpp
// In InventoryComponent::EquipWeapon()
// Get weapon from slot (may be nullptr for empty slots)
AWeaponBase* WeaponToEquip = WeaponSlots[SlotIndex];

// Already equipped (including if both are nullptr)
if (WeaponToEquip == EquippedWeapon)
{
    return false;
}

// Unequip current weapon
if (EquippedWeapon)
{
    EquippedWeapon->UnequipWeapon();
}

// Equip new weapon (or set to nullptr for empty slot = unarmed state)
EquippedWeapon = WeaponToEquip;
CurrentEquippedSlot = SlotIndex;

if (WeaponToEquip)
{
    EquippedWeapon->EquipWeapon();
    OnWeaponEquipped.Broadcast();
}
else
{
    // Empty slot equipped - unarmed state
    OnWeaponUnequipped.Broadcast();
}
```

**Ability System Interaction:**
The Fire and Reload abilities already properly handle nullptr weapons in their `CanActivateAbility()` checks:
- Fire ability: Returns false if `GetEquippedWeapon()` is nullptr
- Reload ability: Returns false if `GetEquippedWeapon()` is nullptr

**Weapon Cycling:**
The scroll wheel cycling (CycleToNextWeapon/CycleToPreviousWeapon) automatically skips empty slots by checking for non-null weapons only.

**Direct Slot Selection:**
Number keys (1-6) can select any slot, including empty ones. This allows players to:
- Holster their weapon by pressing a number key for an empty slot
- Quickly return to an unarmed state for social interactions or stealth

**Blueprint Integration:**
Listen to `OnWeaponUnequipped` to show idle hand animations when an empty slot is equipped.

---

## Files Modified in This Session

- `WeaponBase.h` - Made Fire/EquipWeapon/Reload virtual
- `Revolver.h` - Added Fire() override
- `Revolver.cpp` - Implemented Fire() override, added firing logs
- `GameplayAbility_Weapon.cpp` - Added fallback for character owner
- `GameplayAbility_WeaponFire.cpp` - Added comprehensive logging
- `GameplayAbility_WeaponReload.cpp` - Added reload logging
- `InventoryComponent.cpp` - Added weapon equipping logs, added empty slot equipping support

---

# Session Update (2025-11-13): GAS + Networking Lessons

## Continuous Input Abilities (Move/Look) and Local Control

**Issue observed:** Clients could not move/look; logs spammed: `TriggeredInput: EnhancedInputComponent is null on BP_Player_C_1 during ActivateAbility.`

**Root cause:** Our continuous-input base (`UGameplayAbility_TriggeredInput`) tried to bind Enhanced Input on server/remote proxies. Those contexts don't have a valid `EnhancedInputComponent`.

**Fix pattern:** Only bind Triggered events on locally controlled pawns. Treat mirrored server/remote activations as "success" without binding to avoid cancel spam.

```cpp
void UGameplayAbility_TriggeredInput::ActivateAbility(...) {
    if (const ACattleCharacter* PlayerCharacter = Cast<ACattleCharacter>(GetAvatarActorFromActorInfo()))
    {
        if (!PlayerCharacter->IsLocallyControlled())
        {
            // Skip binding on server/remote; still commit so mirrored ability stays valid
            CommitAbility(Handle, ActorInfo, ActivationInfo);
            return;
        }
        // Bind Triggered on owning client only...
    }
}
```

**NetExecutionPolicy guidance:**
- Move/Look/Jump: `LocalPredicted`
- Actions that must be authoritative (e.g., ammo reset on reload): `ServerOnly`
- Cosmetic prediction remains in weapon/ability code paths.

## Ability-to-Input mapping: robust class matching

**Issue:** Data Asset listed a parent class while the actual granted ability was a BP subclass (or vice versa). Strict equality failed → no binding.

**Fix:** Match with `IsChildOf` in both directions.

```cpp
const UClass* ThisClass = GetClass();
const UClass* ConfiguredClass = AbilityInfo.GameplayAbilityClass;
const bool bMatch = ThisClass && ConfiguredClass &&
    (ThisClass->IsChildOf(ConfiguredClass) || ConfiguredClass->IsChildOf(ThisClass));
```

**Actionable checklist:**
- Ensure `PlayerGameplayAbilitiesDataAsset` includes entries for Move (Axis2D) and Look (Axis2D).
- Verify the Input Mapping Context maps IA_Move/IA_Look appropriately.

## Owner/Weapon resolution: avoid stale caches

**Problem:** `GetWeapon_Implementation: CharacterOwner could not be resolved` could occur when relying solely on cached owner.

**Fix:** Add resolver helpers that prefer cache during activation but fall back to `ActorInfo` during queries (e.g., `CanActivateAbility`).

```cpp
ACattleCharacter* ResolveCharacterOwner(const FGameplayAbilityActorInfo* ActorInfo) const;
AWeaponBase* ResolveWeapon(const FGameplayAbilityActorInfo* ActorInfo) const;
```

Use these in both `CanActivateAbility` and runtime weapon fetches.

## Client-predicted hitscan fire

**Pattern:**
- Client computes trace start/dir from camera and immediately plays cosmetics.
- Sends start/dir via server RPC.
- Server validates `CanFire()`, consumes ammo, runs authoritative trace/damage, and replicates ammo.

This preserves responsive feel while keeping ammo and damage authoritative. We also clamped the cooldown log to avoid `-0.00` display:

```cpp
const float DisplayTime = FMath::Max(0.0f, TimeSinceLastFire);
UE_LOG(Log..., TEXT("Fire rate cooldown (%.2f/%.2f)"), DisplayTime, FireCooldown);
```

## Diagnostics worth keeping

- Add warnings when no matching InputAction is found for a TriggeredInput ability.
- Log when the EnhancedInputComponent or Data Asset is missing (only once per activation, and only on locally controlled pawns to limit noise).
- Weapon/ability logs should include authority (SERVER/CLIENT), pointers, and key state.
