# Weapon Blueprint Setup Guide

This document explains how to set up weapon Blueprints in the CattleGame project. All core weapon logic is implemented in C++, so Blueprints only need minimal configuration for visuals and assets.

## Overview

The weapon system uses a C++ base with Blueprint configuration for:
- **Mesh assets** (static/skeletal meshes)
- **VFX assets** (particle systems, Niagara effects)
- **Audio assets** (sound cues, sound waves)
- **Ability assignments** (which abilities the weapon grants)

## Weapon Classes

| Weapon | C++ Class | Blueprint | Slot ID |
|--------|-----------|-----------|---------|
| Revolver | `ARevolver` | `BP_Revolver` | 0 |
| Lasso | `ALasso` | `BP_Lasso` | 1 |
| Dynamite | `ADynamite` | `BP_Dynamite` | 2 |
| Trumpet | `ATrumpet` | `BP_Trumpet` | 3 |

---

## Creating a Weapon Blueprint

### Step 1: Create the Blueprint

1. Right-click in Content Browser → **Blueprint Class**
2. Search for the parent class (e.g., `Revolver`, `Lasso`, `Dynamite`, `Trumpet`)
3. Name it with `BP_` prefix (e.g., `BP_Revolver`)
4. Save to `/Game/Characters/Player/Weapons/` or appropriate subfolder

### Step 2: Configure the Mesh Component

Each weapon has a `StaticMeshComponent` created in C++:

| Weapon | Component Name | Socket Constant |
|--------|----------------|-----------------|
| Revolver | `RevolverMeshComponent` | `HandGrip_R` |
| Lasso | `LassoMeshComponent` | `HandGrip_R_Lasso` |
| Dynamite | `DynamiteMeshComponent` | `HandGrip_R` |
| Trumpet | `TrumpetMeshComponent` | `HandGrip_R` |

**To set the mesh:**
1. Open the Blueprint
2. Select the mesh component in the Components panel
3. In Details, set **Static Mesh** to your weapon mesh asset
4. Adjust **Relative Transform** if needed for proper hand alignment

### Step 3: Configure Weapon Abilities

In the **Class Defaults**, find the `Weapon Abilities` array and add entries:

#### Revolver Abilities
```
[0] AbilityClass: GA_WeaponFire
    InputAction: IA_Fire
[1] AbilityClass: GA_WeaponReload
    InputAction: IA_Reload
```

#### Lasso Abilities
```
[0] AbilityClass: GA_LassoFire
    InputAction: IA_Fire
[1] AbilityClass: GA_LassoRetract
    InputAction: IA_SecondaryFire
[2] AbilityClass: GA_LassoPullPawnIn
    InputAction: IA_Fire (same as fire, used when tethered)
```

#### Dynamite Abilities
```
[0] AbilityClass: GA_WeaponFire
    InputAction: IA_Fire
```

#### Trumpet Abilities
```
[0] AbilityClass: GA_TrumpetLure
    InputAction: IA_Fire
[1] AbilityClass: GA_TrumpetScare
    InputAction: IA_SecondaryFire
```

### Step 4: Configure Weapon-Specific Properties

#### Revolver Properties
| Property | Default | Description |
|----------|---------|-------------|
| `MaxAmmo` | 6 | Magazine capacity |
| `DamageAmount` | 25.0 | Damage per shot |
| `FireRate` | 2.0 | Shots per second |
| `ReloadTime` | 2.0 | Seconds to reload |
| `WeaponRange` | 5000.0 | Hitscan range (units) |
| `WeaponSpread` | 0.0 | Accuracy spread (degrees) |
| `MuzzleFlashEffect` | None | Particle system for muzzle flash |
| `ImpactEffect` | None | Particle system for bullet impact |
| `FireSound` | None | Sound for firing |
| `ReloadSound` | None | Sound for reloading |
| `EmptySound` | None | Sound when empty |

#### Lasso Properties
| Property | Default | Description |
|----------|---------|-------------|
| `ProjectileClass` | None | **Required**: Set to `BP_LassoProjectile` |
| `MaxRopeLength` | 1000.0 | Auto-retract distance |
| `ThrowSpeed` | 1500.0 | Initial projectile speed |
| `GravityScale` | 0.5 | Arc trajectory (0=straight, 1=full gravity) |
| `AimAssistAngle` | 15.0 | Aim assist cone (degrees) |
| `AimAssistMaxDistance` | 800.0 | Aim assist range |
| `RopeElasticity` | 500.0 | Spring constant for pull |
| `RopeDamping` | 50.0 | Prevents oscillation |
| `ThrowCooldown` | 0.5 | Seconds before next throw |
| `RetractSpeed` | 2000.0 | Rope return velocity |

#### Dynamite Properties
| Property | Default | Description |
|----------|---------|-------------|
| `ProjectileClass` | None | **Required**: Set to `BP_DynamiteProjectile` |
| `CurrentAmmo` | 3 | Starting dynamite sticks |
| `MaxAmmo` | 10 | Maximum capacity |
| `ThrowForce` | 1500.0 | Throw velocity |
| `ExplosionRadius` | 500.0 | Blast radius |
| `ExplosionDamage` | 100.0 | Damage at center |
| `FuseTime` | 5.0 | Seconds until explosion |

#### Trumpet Properties
| Property | Default | Description |
|----------|---------|-------------|
| `LureRadius` | 800.0 | Lure effect radius |
| `LureStrength` | 500.0 | Attraction force |
| `ScareRadius` | 800.0 | Scare effect radius |
| `ScareStrength` | 500.0 | Repulsion force |

---

## Projectile Blueprints

### BP_LassoProjectile

Parent class: `ALassoProjectile`

**Components (created in C++):**
- `CollisionSphere` - Root collision
- `MeshComponent` - Rope loop visual
- `CableComponent` - Rope trail visual
- `ProjectileMovement` - Flight physics

**Setup:**
1. Set `MeshComponent` static mesh to rope loop asset
2. Configure `CableComponent` material and width
3. Adjust collision sphere radius if needed

### BP_DynamiteProjectile

Parent class: `ADynamiteProjectile`

**Components (created in C++):**
- `CollisionSphere` - Root collision with bounce
- `MeshComponent` - Dynamite stick visual
- `ProjectileMovement` - Throw physics with bounce

**Setup:**
1. Set `MeshComponent` static mesh to dynamite asset
2. Optionally set explosion particle effect
3. Optionally set fuse/explosion sounds

---

## Blueprint Events (Optional Overrides)

All weapons expose `BlueprintNativeEvent` functions that can be overridden for custom behavior. The C++ base provides default implementations.

### Common Weapon Events

| Event | When Called | Default Behavior |
|-------|-------------|------------------|
| `OnWeaponEquipped` | Weapon is equipped | Logs equip |
| `OnWeaponUnequipped` | Weapon is unequipped | Logs unequip |
| `OnWeaponFired` | Weapon fires | Logs fire |

### Lasso-Specific Events

| Event | When Called | Default Behavior |
|-------|-------------|------------------|
| `OnLassoThrown` | Projectile launched | Logs throw |
| `OnLassoCaughtTarget` | Target caught | Logs catch |
| `OnLassoReleased` | Target released | Logs release |
| `OnLassoRetractStarted` | Retract begins | Logs retract start |
| `OnLassoRetractComplete` | Retract finishes | Logs retract complete |
| `OnLassoReady` | Cooldown ends | Logs ready |
| `OnLassoReattached` | Lasso returns to hand | **Re-attaches to socket** |
| `OnLassoStateChanged` | Any state transition | Logs state change |
| `OnShowRopeWrapVisual` | Target caught | Placeholder for wrap VFX |
| `OnHideRopeWrapVisual` | Target released | Placeholder for wrap cleanup |
| `OnPullStarted` | Player holds fire | Logs pull start |
| `OnPullStopped` | Player releases fire | Logs pull stop |
| `OnRopeTensionChanged` | While pulling | (No log, frequent call) |
| `OnTargetLost` | Target destroyed | Logs target lost |
| `OnProjectileDestroyed` | Projectile cycle ends | Logs cleanup |

### Trumpet-Specific Events

| Event | When Called | Default Behavior |
|-------|-------------|------------------|
| `OnTrumpetStarted` | Playing begins | Broadcasts delegate |
| `OnTrumpetStopped` | Playing ends | Broadcasts delegate |

---

## Attachment System

Weapons automatically attach to character hand sockets when equipped:

```cpp
// In EquipWeapon()
AttachToActor(CurrentOwner, AttachRules, SocketName);
```

Socket names are defined as static constants:
- `ARevolver::RevolverHandSocketName` → `"HandGrip_R"`
- `ALasso::LassoHandSocketName` → `"HandGrip_R_Lasso"`
- `ADynamite::DynamiteHandSocketName` → `"HandGrip_R"`
- `ATrumpet::TrumpetHandSocketName` → `"HandGrip_R"`

**Important:** Ensure your character skeleton has these sockets defined.

---

## State Machine (Lasso Only)

The lasso has a state machine managed in C++:

```
┌──────────────────────────────────────────────────────┐
│                                                      │
│  ┌─────────┐    Fire    ┌───────────┐               │
│  │  IDLE   │ ─────────► │ IN_FLIGHT │               │
│  └─────────┘            └───────────┘               │
│       ▲                       │                      │
│       │                       │ Hit Target           │
│       │ Cooldown              ▼                      │
│       │                 ┌──────────┐                │
│  ┌────────────┐         │ TETHERED │                │
│  │ RETRACTING │ ◄────── └──────────┘                │
│  └────────────┘  Release/Miss                       │
│                                                      │
└──────────────────────────────────────────────────────┘
```

State queries available:
- `GetLassoState()` → Returns `ELassoState`
- `IsInState(State)` → Boolean check
- `GetTetheredTarget()` → Returns caught actor
- `IsPulling()` → True if player holding fire

---

## Gameplay Tags

The weapon system uses gameplay tags for state tracking:

### Weapon Tags
- `State.Weapon.Firing` - Currently firing
- `State.Weapon.Reloading` - Currently reloading

### Lasso Tags
- `State.Lasso.Active` - Lasso is in use (any non-idle state)
- `State.Lasso.Throwing` - Projectile in flight
- `State.Lasso.Pulling` - Tethered to target
- `State.Lasso.Retracting` - Rope returning

### Trumpet Tags
- `State.Trumpet.Playing` - Trumpet is playing

---

## Troubleshooting

### Weapon not visible when equipped
1. Check mesh component has a valid static mesh assigned
2. Verify socket name exists on character skeleton
3. Check `SetWeaponMeshVisible(true)` is called

### Weapon not attaching to hand
1. Verify `OwnerCharacter` is set before `EquipWeapon()`
2. Check socket name matches skeleton socket exactly
3. Review logs for attachment errors

### Abilities not working
1. Ensure `WeaponAbilities` array is populated in Blueprint
2. Verify ability classes are valid
3. Check input actions are bound in project settings

### Lasso projectile not spawning
1. Ensure `ProjectileClass` is set to valid Blueprint class
2. Check `CanFire()` returns true (Idle state, no cooldown)
3. Verify projectile Blueprint compiles without errors

### Dynamite not exploding
1. Check `FuseTime` is reasonable (default 5 seconds)
2. Verify `ExplosionRadius` and `ExplosionDamage` are set
3. Check server authority (explosions are server-only)

---

## Example: Minimal BP_Revolver Setup

1. Create Blueprint with parent `Revolver`
2. Set `RevolverMeshComponent` → Static Mesh to revolver model
3. Add to `WeaponAbilities`:
   - `GA_WeaponFire` + `IA_Fire`
   - `GA_WeaponReload` + `IA_Reload`
4. Set `MuzzleFlashEffect` to particle system
5. Set `FireSound` to gunshot sound
6. Done! All firing, damage, reload logic is in C++

---

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2025-11-29 | Initial C++ implementation with Blueprint configuration |
