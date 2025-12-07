# CattleGame Development Reference

## GAS System Essentials

### Weapon Firing Pattern
Blueprint events (`UFUNCTION(BlueprintImplementableEvent)`) require **explicit C++ implementation calls**:

```cpp
void ARevolver::Fire() {
    Super::Fire();  // Triggers blueprint event
    OnWeaponFired_Implementation(GetOwner(), ActiveMesh);  // Ensures C++ logic runs
}
```
**Why:** Without explicitly calling `_Implementation()`, C++ code (ammo consumption, damage) never executes even if the blueprint event fires.

### Character Owner Resolution
Use **multi-level fallback** to avoid stale caches:

```cpp
ACattleCharacter* CharacterOwner = CachedCharacterOwner;
if (!CharacterOwner && CurrentActorInfo) {
    CharacterOwner = Cast<ACattleCharacter>(CurrentActorInfo->OwnerActor.Get());
}
// Now use CharacterOwner...
```

### Weapon State Variables
Keep these replicated:
- `UPROPERTY(Replicated) bool bIsEquipped`
- `UPROPERTY(Replicated) bool bIsReloading`
- `UPROPERTY(Replicated) int32 CurrentAmmo`

---

## Ability System Architecture

### Two-Tier Ability Design
| Layer | InputID Range | Granted | Examples |
|-------|---------------|---------|----------|
| **Character Abilities** | 1000-1999 | Once at init | Move, Look, Jump, Switch Slots |
| **Weapon Abilities** | 100-999 | On equip/unequip | Fire, Reload, LassoFire |

**Key principle:** Abilities are permanent; weapons are temporary. Check weapon state in `CanActivateAbility()`:

```cpp
bool UGameplayAbility_WeaponFire::CanActivateAbility() const {
    AWeaponBase* Weapon = Inventory->GetEquippedWeapon();
    return Weapon && Weapon->CanFire();
}
```

### Input Binding Pattern
Data Asset drives everything—zero weapon-specific callbacks in character:

```cpp
void ACattleCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) {
    if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
        for (const FGameplayInputAbilityInfo& AbilityInfo : DataAsset->GetInputAbilities()) {
            EIC->BindAction(AbilityInfo.InputAction, ETriggerEvent::Started,
                this, &ACattleCharacter::OnAbilityInputPressed, AbilityInfo.InputID);
        }
    }
}
```

### Continuous Input (Move/Look)
Inherit from `UGameplayAbility_TriggeredInput` and bind Enhanced Input **only on locally controlled pawns**:

```cpp
void UGameplayAbility_TriggeredInput::ActivateAbility(...) {
    if (const ACattleCharacter* PlayerCharacter = Cast<ACattleCharacter>(GetAvatarActorFromActorInfo())) {
        if (!PlayerCharacter->IsLocallyControlled()) {
            CommitAbility(Handle, ActorInfo, ActivationInfo);
            return;  // Skip binding on server/remote
        }
        // Bind Triggered on owning client only...
    }
}
```

---

## Gameplay Cues & VFX

### Gameplay Cues Overview
Gameplay Cues are **replicating cosmetic systems** that separate server logic from client visuals. A single GameplayCue can be executed on all clients (or a subset) without network traffic for each visual effect.

**Key pattern:**
```cpp
// In weapon ability (server logic)
void UGameplayAbility_WeaponFire::FireWeapon() {
    AWeaponBase* Weapon = GetWeapon_Implementation();
    Weapon->Fire();  // Server-only logic (ammo, damage)

    // Execute cue on all clients (replicated via FGameplayEventData)
    FGameplayEventData EventData;
    EventData.Instigator = GetAvatarActorFromActorInfo();
    UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
        EventData.Instigator, FGameplayTag::RequestGameplayTag(FName("Event.Weapon.Fire")), EventData);
}
```

### Base Gameplay Cue Class Pattern
Create a **base C++ class** with reusable blueprint hooks:

```cpp
// GameplayCue_WeaponBase.h
class CATTLEGAME_API AGameplayCue_WeaponBase : public AGameplayCueNotify_Actor {
public:
    virtual void OnExecute_Implementation(const FGameplayCueParameters& Parameters) override;

protected:
    UFUNCTION(BlueprintImplementableEvent, Category = "Weapon")
    void OnWeaponFireVisuals(AActor* InstigatorActor);

    UFUNCTION(BlueprintImplementableEvent, Category = "Weapon")
    void PlayMuzzleFlash();

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void SpawnTracer(FVector Start, FVector End);

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void PlayFireSound(USoundBase* FireSound);
};
```

```cpp
// GameplayCue_WeaponBase.cpp
void AGameplayCue_WeaponBase::OnExecute_Implementation(const FGameplayCueParameters& Parameters) {
    Super::OnExecute_Implementation(Parameters);

    AActor* InstigatorActor = Parameters.Instigator;
    if (!InstigatorActor) return;

    // Call blueprint event for visual customization
    OnWeaponFireVisuals(InstigatorActor);

    // Call reusable C++ helpers
    PlayMuzzleFlash();
    PlayFireSound(nullptr);  // Override in blueprints
}
```

### Blueprint Cue Specialization
Create **weapon-specific blueprints** from the base class:

```
BP_GameplayCue_RevolverFire (inherits AGameplayCue_WeaponBase)
  ├─ Implement OnWeaponFireVisuals() → Revolver muzzle position
  ├─ Implement PlayMuzzleFlash() → Revolver flash VFX
  └─ Implement PlayFireSound() → Revolver gunshot sound

BP_GameplayCue_LassoFire (inherits AGameplayCue_WeaponBase)
  ├─ Implement OnWeaponFireVisuals() → Lasso swing animation
  ├─ Implement PlayMuzzleFlash() → No flash (not needed)
  └─ Implement PlayFireSound() → Rope whoosh sound
```

### Triggering from Abilities
Tag-based execution is simpler than class references:

```cpp
// In any ability
void ExecuteWeaponCue(const FString& WeaponType) {
    FGameplayEventData EventData;
    EventData.Instigator = GetAvatarActorFromActorInfo();

    // Tag format: "Event.Weapon.Fire.Revolver"
    FString TagName = FString::Printf(TEXT("Event.Weapon.Fire.%s"), *WeaponType);
    FGameplayTag CueTag = FGameplayTag::RequestGameplayTag(FName(*TagName));

    UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
        EventData.Instigator, CueTag, EventData);
}
```

---

## Base Class to Blueprint Pattern

### Architecture Pattern
Use **C++ base classes** for game logic, **Blueprint children** for content variation:

```
C++ Base Class (Logic)
    ↓
    ├─ Defines virtual functions for customization
    ├─ Implements core behavior
    └─ Provides BlueprintImplementableEvent hooks

Blueprint Child (Content)
    ↓
    ├─ Overrides virtual functions
    ├─ Implements blueprint events for visuals
    └─ Configures properties (VFX, sounds, etc.)
```

### Weapon Base Class Example
```cpp
// WeaponBase.h
class CATTLEGAME_API AWeaponBase : public AActor {
public:
    virtual void Fire();
    virtual void EquipWeapon();
    virtual void UnequipWeapon();
    virtual bool CanFire() const;

    UFUNCTION(BlueprintImplementableEvent)
    void OnWeaponFired(AActor* Owner, USkeletalMeshComponent* Mesh);

    UFUNCTION(BlueprintImplementableEvent)
    void OnEquipped();

    UFUNCTION(BlueprintImplementableEvent)
    void OnUnequipped();

protected:
    UPROPERTY(BlueprintReadOnly, Replicated)
    int32 CurrentAmmo = 6;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    int32 MaxAmmo = 6;

    float FireCooldown = 0.1f;
};
```

### Blueprint Specialization Pattern
```cpp
// Revolver.h (Blueprint child in C++)
class CATTLEGAME_API ARevolver : public AWeaponBase {
public:
    virtual void Fire() override;  // Custom firing logic
    virtual bool CanFire() const override;

private:
    void ConsumeAmmo();
    void ValidateOwner();
};

// BP_Revolver (Content Blueprint)
// - Inherits from Revolver (or AWeaponBase)
// - Implements OnWeaponFired event for sound/vfx
// - Sets Properties: MaxAmmo=6, FireCooldown=0.15f
// - Configures mesh, animations, etc.
```

### Virtual Function Rules for Blueprint Inheritance
1. **Mark functions `virtual` in base class** - Allows overrides
2. **Use `BlueprintImplementableEvent`** - For pure blueprint implementation
3. **Use `BlueprintNativeEvent`** - For C++ fallback + blueprint override
4. **Call `Super::Function()`** - Always chain upward
5. **Call `_Implementation()` explicitly** - If C++ logic must run

```cpp
// WeaponBase.h - Best pattern for hybrid C++/Blueprint
UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
void Fire();

// WeaponBase.cpp
void AWeaponBase::Fire_Implementation() {
    // Default C++ behavior
    if (!CanFire()) return;
    ConsumeAmmo();
    PlayFireAnimation();
}

// Revolver.h
virtual void Fire() override;

// Revolver.cpp
void ARevolver::Fire() {
    Super::Fire();  // Calls AWeaponBase::Fire_Implementation()
    // Revolver-specific logic here
}
```

### Blueprint Configuration Pattern
Properties should be **EditDefaultsOnly** so blueprints can configure without changing at runtime:

```cpp
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
float FireRate = 0.1f;

UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio")
USoundBase* FireSound;

UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VFX")
UParticleSystem* MuzzleFlashVFX;

UPROPERTY(BlueprintReadWrite, Replicated, Category = "State")
int32 CurrentAmmo;  // Runtime state
```

### Common Mistakes with Base Classes
1. **Not marking functions `virtual`** → Children can't override
2. **Forgetting `Super::Function()` call** → Parent logic skipped
3. **Using `BlueprintImplementableEvent` only** → No C++ fallback possible
4. **Making everything `BlueprintReadWrite`** → Content can break game logic
5. **Not using `_Implementation()` suffix** → `BlueprintNativeEvent` behavior unclear

---

## Weapon State & Equipping

### Empty Slot Handling
Players can equip empty slots to holster weapons:

```cpp
bool UInventoryComponent::EquipWeapon(int32 SlotIndex) {
    AWeaponBase* WeaponToEquip = WeaponSlots[SlotIndex];  // May be nullptr

    if (WeaponToEquip == EquippedWeapon) return false;

    if (EquippedWeapon) EquippedWeapon->UnequipWeapon();

    EquippedWeapon = WeaponToEquip;
    CurrentEquippedSlot = SlotIndex;

    if (WeaponToEquip) {
        EquippedWeapon->EquipWeapon();
        OnWeaponEquipped.Broadcast();
    } else {
        OnWeaponUnequipped.Broadcast();  // Unarmed state
    }
    return true;
}
```

**Weapon cycling:** Scroll wheel skips empty slots; number keys select any slot.

---

## Networking & Authority

### Client-Predicted Hitscan Pattern
```
Client: Compute trace from camera → play cosmetics immediately
Server: Receive trace → validate CanFire() → consume ammo → run authoritative trace/damage
```
This preserves responsiveness while keeping ammo/damage authoritative.

### NetExecutionPolicy
- **Move/Look/Jump:** `LocalPredicted`
- **Ammo updates/damage:** `ServerOnly`
- **Cosmetics:** Client-side only

---

## Debugging Checklist

When weapon abilities fail:
- [ ] Is `CanActivateAbility()` returning TRUE? (Check logs)
- [ ] Is `ActivateAbility()` called? (Check logs)
- [ ] Are `_Implementation()` functions called? (Override + explicit call)
- [ ] Are variables replicated? (Check `DOREPLIFETIME`)
- [ ] Is character owner resolved? (Use fallback pattern)
- [ ] Only locally controlled pawns bind Enhanced Input?

**Key log pattern:**
```cpp
UE_LOG(LogGASDebug, Warning, TEXT("Fire: CanActivateAbility=%d, HasWeapon=%s, CanFire=%d"),
    CanActivateAbility(), Weapon ? TEXT("true") : TEXT("false"), Weapon->CanFire());
```

---

## Adding New Weapons

1. Override `Fire()`, `EquipWeapon()`, `Reload()`
2. Call `Super::FunctionName()` first to fire blueprint events
3. Manually call `_Implementation()` to ensure C++ logic runs
4. Implement `CanFire()` and `CanReload()` with state checks
5. Replicate weapon state (bIsEquipped, bIsReloading, ammo)
6. Add logging at state transitions

---

## Blueprint Serializer Plugin

Export/import Blueprints as JSON for AI-assisted modifications.

**Editor usage:**
- Right-click Blueprint → **Blueprint Serializer → Export to JSON**
- Modify JSON (AI-assisted)
- Right-click Blueprint → **Blueprint Serializer → Import from JSON**
- Review merge in three-way editor

**Python CLI:**
```powershell
python -m blueprint_serializer.export_blueprint "/Game/Characters/Player/BP_Player" -o out.json
python -m blueprint_serializer.import_blueprint out.json /Game/Characters/Player -n BP_Player_Modified
```

**Schema:** Version 2.0.0 with `FMemberReference` for proper function/variable/event resolution.

---

## Common Pitfalls

1. **Forgetting manual `_Implementation()` call** → C++ logic never runs
2. **Caching character owner** → Cache becomes stale on ability reuse
3. **Binding Enhanced Input on server/remote** → Crashes or spam errors
4. **Class mismatch in Data Asset** → Use `IsChildOf()` for robust matching
5. **Abilities granted/revoked per weapon** → Overcomplicated; use capability pattern instead
