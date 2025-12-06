# Weapon System Refactor - Blueprint Migration Guide

## Overview

The weapon system has been refactored to use a pure **ability-driven architecture**. Weapons are now **data containers**, and all behavior (equip, fire, reload) is handled by **Gameplay Abilities**.

---

## Breaking Changes

### 1. WeaponBase C++ Methods Removed

The following methods have been **removed** from `AWeaponBase`:

- `Fire()`
- `Reload()`
- `EquipWeapon()`
- `UnequipWeapon()`

### 2. Blueprint Events Removed

The following Blueprint events have been **removed** from `AWeaponBase`:

- `OnWeaponEquipped`
- `OnWeaponUnequipped`
- `OnWeaponFired`
- `OnReloadStarted`
- `OnReloadCompleted`

---

## Migration Steps

### Step 1: Migrate Equip/Unequip Logic to GA_WeaponEquip

**Old Blueprint (DEPRECATED):**
```
Event OnWeaponEquipped(Character, Mesh)
  ├─ Attach Weapon Mesh to Socket
  ├─ Play Equip Animation
  └─ Spawn Equip VFX
```

**New Blueprint:**
1. Create a Blueprint child of `GA_WeaponEquip` (e.g., `GA_WeaponEquip_Revolver`)
2. Override the `OnEquipWeapon` event:
   ```
   Event OnEquipWeapon(Weapon, Character, Mesh)
     ├─ [C++ handles attachment automatically]
     ├─ Play Equip Animation (montage on Character)
     └─ Spawn Equip VFX (at Weapon location)
   ```
3. Override the `OnUnequipWeapon` event if needed:
   ```
   Event OnUnequipWeapon(Weapon, Character, Mesh)
     ├─ [C++ handles detachment automatically]
     ├─ Play Holster Animation
     └─ Spawn Holster VFX
   ```

**Note:** The C++ implementation of `GA_WeaponEquip` already handles:
- Weapon attachment to the active mesh
- Socket binding (default: "WeaponSocket")
- Mesh visibility (show on equip, hide on unequip)

You only need to add **cosmetic effects** (animations, VFX, SFX) in Blueprint.

---

### Step 2: Migrate Fire Logic to GA_WeaponFire

**Old Blueprint (DEPRECATED):**
```
Event OnWeaponFired(Character, Mesh)
  ├─ Play Firing Animation
  ├─ Spawn Muzzle Flash VFX
  ├─ Play Fire Sound
  └─ Perform Hitscan/Projectile Spawn
```

**New Blueprint:**
1. Your weapon should already have a `GA_WeaponFire` ability in the `WeaponAbilities` array
2. Create a Blueprint child of `GA_WeaponFire` (e.g., `GA_WeaponFire_Revolver`)
3. Override the firing logic in the ability:
   ```
   Event ActivateAbility
     ├─ Get Weapon (from parent GA_Weapon)
     ├─ Check CanFire() on Weapon
     ├─ Play Firing Animation (montage on Character)
     ├─ Spawn Muzzle Flash VFX
     ├─ Play Fire Sound
     ├─ Perform Hitscan Trace or Spawn Projectile
     ├─ Apply Damage (if hit)
     ├─ Decrement Ammo
     └─ End Ability
   ```

**Important:** All fire logic (trace, damage, ammo) must now be in the **GA_WeaponFire ability**, not in the weapon Blueprint.

---

### Step 3: Migrate Reload Logic to GA_WeaponReload

**Old Blueprint (DEPRECATED):**
```
Event OnReloadStarted(Character, Mesh)
  ├─ Play Reload Animation
  ├─ Play Reload Sound
  └─ Wait for animation completion

Event OnReloadCompleted(Character, Mesh)
  └─ Refill Ammo
```

**New Blueprint:**
1. Create a Blueprint child of `GA_WeaponReload` (e.g., `GA_WeaponReload_Revolver`)
2. Override the reload logic:
   ```
   Event ActivateAbility
     ├─ Get Weapon (from parent GA_Weapon)
     ├─ Check if reload is needed (CurrentAmmo < MaxAmmo)
     ├─ Play Reload Animation (montage on Character)
     ├─ Play Reload Sound
     ├─ Wait for Montage Completion
     ├─ Refill Ammo on Weapon
     └─ End Ability
   ```

**Important:** Ammo state (`CurrentAmmo`, `MaxAmmo`, etc.) remains stored on the **Weapon** as UPROPERTYs. The ability queries and modifies these values.

---

### Step 4: Update Weapon Blueprints

For each weapon Blueprint (e.g., `BP_Revolver`):

1. **Remove** any overrides of:
   - `OnWeaponEquipped`
   - `OnWeaponUnequipped`
   - `OnWeaponFired`
   - `OnReloadStarted`
   - `OnReloadCompleted`

2. **Add Gameplay Abilities** to the `WeaponAbilities` array:
   - Add `GA_WeaponFire_Revolver` (Blueprint child of GA_WeaponFire)
   - Add `GA_WeaponReload_Revolver` (Blueprint child of GA_WeaponReload)
   - Bind each ability to its corresponding Input Action (e.g., `IA_Fire`, `IA_Reload`)

3. **Keep weapon data** (UPROPERTYs):
   - `CurrentAmmo`
   - `MaxAmmo`
   - `bIsReloading`
   - `Damage`
   - `FireRate`
   - etc.

---

### Step 5: Remove HitscanWeaponBase References (Optional)

`HitscanWeaponBase` has been **removed**. If you were using it:

1. Change weapon parent class from `HitscanWeaponBase` to `WeaponBase`
2. Move any client prediction logic into your `GA_WeaponFire_BP`:
   ```
   Event ActivateAbility
     ├─ If (Is Locally Controlled)
     │   └─ Play Predicted VFX/SFX (immediate feedback)
     ├─ Server RPC: Fire Weapon
     │   └─ Perform authoritative trace/damage
     └─ End Ability
   ```

---

## New Architecture Summary

| **Old** | **New** |
|---------|---------|
| `AWeaponBase::Fire()` → Blueprint Event | `GA_WeaponFire::ActivateAbility` |
| `AWeaponBase::Reload()` → Blueprint Event | `GA_WeaponReload::ActivateAbility` |
| `AWeaponBase::EquipWeapon()` → Blueprint Event | `GA_WeaponEquip::OnEquipWeapon` |
| `AWeaponBase::UnequipWeapon()` → Blueprint Event | `GA_WeaponEquip::OnUnequipWeapon` |
| Weapon stores ammo | **Still true** - keep ammo UPROPERTYs on weapon |
| Weapon handles cosmetics | **Now in abilities** - abilities query weapon and trigger cosmetics |

---

## Benefits of New Architecture

1. **Clear separation of concerns**:
   - Weapons = data containers
   - Abilities = behavior logic

2. **Better GAS integration**:
   - All actions go through the Ability System
   - Easier to add cooldowns, costs, and tags

3. **Less duplication**:
   - No more repeated `OwnerCharacter ? OwnerCharacter : Cast<>(GetOwner())`
   - Single `GetOwnerCharacter()` helper

4. **Easier testing**:
   - Abilities can be tested independently
   - Weapons don't have behavior to test

---

## Troubleshooting

### "My weapon doesn't equip/fire anymore"
- Check that `WeaponAbilities` array is populated in your weapon Blueprint
- Verify abilities are correctly bound to Input Actions
- Check that `GA_WeaponEquip` is being granted to the character's ASC

### "Animations don't play"
- Move animation montage playback into the ability Blueprint (`GA_WeaponFire_BP`, etc.)
- Use `Play Montage and Wait` in the ability graph

### "VFX/SFX missing"
- Move cosmetic spawning (VFX, SFX) into ability Blueprints
- Query weapon location/sockets from the `Weapon` parameter in ability events

---

## Questions?

If you encounter issues during migration, check:
1. `GA_WeaponEquip.cpp` - Default equip/unequip implementation
2. `GA_Weapon.h` - Base weapon ability class
3. `InventoryComponent.cpp` - How abilities are granted/triggered

For reference implementations, see:
- `GA_WeaponFire` (Revolver fire ability)
- `GA_WeaponReload` (Revolver reload ability)
