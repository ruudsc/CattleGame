# Lasso Blueprint Integration Guide

This document describes how to set up the Lasso weapon Blueprints (`BP_Lasso` and `BP_LassoProjectile`) to integrate with the C++ lasso system.

---

## Overview

The lasso system consists of two main actors:

| Blueprint | Parent Class | Purpose |
|-----------|--------------|---------|
| `BP_Lasso` | `ALasso` | The weapon held by the player |
| `BP_LassoProjectile` | `ALassoProjectile` | The flying rope loop that catches targets |

---

## State Machine

The lasso operates on a simple state machine:

```
┌─────────┐   Primary    ┌───────────┐   Hit Target   ┌──────────┐
│  IDLE   │────Fire─────►│ IN_FLIGHT │───────────────►│ TETHERED │
└─────────┘              └───────────┘                └──────────┘
     ▲                         │                           │
     │                         │ Miss/Max Range            │
     │                         ▼                           │
     │                   ┌───────────┐                     │
     └───────────────────│RETRACTING │◄────Secondary Fire──┘
         (+ 0.5s         └───────────┘
          cooldown)
```

---

## BP_Lasso Setup

### Required Configuration

In the Blueprint Class Defaults, set:

| Property | Category | Recommended Value | Description |
|----------|----------|-------------------|-------------|
| `Projectile Class` | Lasso\|Projectile | `BP_LassoProjectile` | The projectile Blueprint to spawn |
| `Max Rope Length` | Lasso\|Config | `1000.0` | Auto-retract distance (units) |
| `Throw Speed` | Lasso\|Config | `1500.0` | Initial projectile velocity |
| `Gravity Scale` | Lasso\|Config | `0.5` | Arc trajectory (0=straight, 1=full gravity) |
| `Aim Assist Angle` | Lasso\|Config | `15.0` | Cone angle for target assist (degrees) |
| `Aim Assist Max Distance` | Lasso\|Config | `800.0` | Max range for aim assist |
| `Rope Elasticity` | Lasso\|Config | `500.0` | Spring constant for pull physics |
| `Rope Damping` | Lasso\|Config | `50.0` | Prevents oscillation |
| `Throw Cooldown` | Lasso\|Config | `0.5` | Delay after retract before next throw |
| `Retract Speed` | Lasso\|Config | `2000.0` | Rope return velocity |

### Blueprint Events to Implement

Override these events in your `BP_Lasso` Blueprint to add visual/audio feedback:

#### Core Events

| Event | When Called | Suggested Implementation |
|-------|-------------|--------------------------|
| `OnLassoThrown` | Projectile launches | Play throw animation, throw SFX |
| `OnLassoCaughtTarget(CaughtTarget)` | Target caught | Play catch SFX, screen shake |
| `OnLassoReleased` | Tether manually released | Play release SFX |
| `OnLassoRetractStarted` | Rope starts returning | Play retract SFX (looping) |
| `OnLassoRetractComplete` | Rope reaches player | Stop retract SFX |
| `OnLassoReady` | Cooldown ends | Play ready SFX/indicator |

#### Visual Events

| Event | When Called | Suggested Implementation |
|-------|-------------|--------------------------|
| `OnShowRopeWrapVisual(Target)` | Target gets wrapped | Spawn wrap mesh/decal on target |
| `OnHideRopeWrapVisual(Target)` | Target released | Destroy wrap mesh/decal |
| `OnLassoReattached` | Weapon returns to hand | Re-attach weapon mesh to `HandGrip_R_Lasso` socket |

#### Pull/Tension Events

| Event | When Called | Suggested Implementation |
|-------|-------------|--------------------------|
| `OnPullStarted` | Player holds pull input | Start tension VFX, play strain SFX |
| `OnPullStopped` | Player releases pull | Stop tension VFX/SFX |
| `OnRopeTensionChanged(Stretch, Force)` | Every tick while pulling | Scale cable thickness, particle intensity based on `Stretch` |

#### State Events

| Event | When Called | Suggested Implementation |
|-------|-------------|--------------------------|
| `OnLassoStateChanged(OldState, NewState)` | Any state transition | Complex state-based logic |
| `OnTargetLost(LostTarget)` | Target destroyed/invalid | Play break-free SFX |
| `OnProjectileDestroyed` | Projectile cycle complete | Clean up projectile VFX |

### Socket Re-attachment Example

In `OnLassoReattached`:

```
Event OnLassoReattached
    │
    ├─► Get Owning Character
    │
    ├─► Get Character Mesh Component
    │
    ├─► Attach Weapon Mesh to Socket
    │       Socket Name: "HandGrip_R_Lasso"
    │       Location Rule: Snap to Target
    │       Rotation Rule: Snap to Target
    │       Scale Rule: Keep World
    │
    └─► (Optional) Play attach animation/SFX
```

**C++ Socket Constant:** `ALasso::LassoHandSocketName` = `"HandGrip_R_Lasso"`

---

## BP_LassoProjectile Setup

### Required Configuration

In the Blueprint Class Defaults, configure the visual components:

| Component | Configuration |
|-----------|---------------|
| `Mesh Component` | Assign a static mesh for the rope loop (lasso head) |
| `Cable Component` | Configure rope visual (see below) |
| `Collision Sphere` | Set collision preset for detecting targets |

### Cable Component Settings

| Property | Recommended Value | Description |
|----------|-------------------|-------------|
| `Cable Length` | `0.0` | Auto-calculated from attachment points |
| `Num Segments` | `20` | Rope smoothness |
| `Solver Iterations` | `4` | Physics accuracy |
| `Cable Width` | `2.0` | Rope thickness |
| `Num Sides` | `6` | Rope roundness |
| `Tile Material` | `1.0` | UV tiling |
| `Cable Material` | (Your rope material) | Visual appearance |

### Cable Attachment

The cable automatically attaches:
- **Start**: Player's hand socket (set in C++)  
- **End**: Projectile mesh (this Blueprint)

The C++ code handles cable attachment. You just need to configure the visual properties.

### Collision Settings

Configure `CollisionSphere` to detect valid targets:

```
Collision Preset: Custom
Object Type: WorldDynamic
    
Collision Responses:
  - Pawn: Overlap (for catching animals/players)
  - WorldStatic: Block (for environment collision → auto-retract)
  - WorldDynamic: Ignore (or Block if catching physics objects)
```

---

## Ability Setup

The lasso uses the Gameplay Ability System. Ensure these abilities are granted:

| Ability | Input | Purpose |
|---------|-------|---------|
| `GA_LassoFire` | Primary Fire (LMB) | Throw lasso OR pull while tethered |
| `GA_LassoRetract` | Secondary Fire (RMB) | Release tethered target |

These should be configured in the weapon's `WeaponAbilities` array (inherited from `AWeaponBase`).

---

## State Queries (BlueprintCallable)

Use these functions in Blueprint to query lasso state:

| Function | Returns | Description |
|----------|---------|-------------|
| `GetLassoState()` | `ELassoState` | Current state enum |
| `IsInState(State)` | `bool` | Check specific state |
| `GetTetheredTarget()` | `AActor*` | Currently caught actor |
| `IsPulling()` | `bool` | Is pull input held? |
| `GetCooldownRemaining()` | `float` | Seconds until ready |
| `GetProjectile()` | `ALassoProjectile*` | Access projectile instance |

---

## Troubleshooting

### Lasso doesn't throw
- Check `CurrentState` is `Idle`
- Check `CooldownRemaining` is `0`
- Verify `ProjectileClass` is set

### Rope not visible
- Check Cable Component material is assigned
- Check Cable Component `Attach Start To` and `Attach End To`
- Verify projectile is spawning (check logs for `LassoProjectile::Launch`)

### Target not caught
- Check collision settings on `CollisionSphere`
- Verify target is a `Pawn`
- Check `bHasCaughtTarget` flag (prevents multi-catch)

### Pull not working
- Check state is `Tethered`
- Verify `GA_LassoFire` ability allows activation when tethered
- Check `SetPulling()` is being called

### Weapon not re-attaching
- Implement `OnLassoReattached` event in Blueprint
- Use socket name `HandGrip_R_Lasso`
- Check character skeleton has this socket

---

## Debug Logging

Enable verbose logging by checking Output Log for `LogGASDebug`:

```
LogGASDebug: Warning: Lasso::SetState - 0 -> 1        (Idle → InFlight)
LogGASDebug: Warning: Lasso::OnProjectileHitTarget    (Caught target)
LogGASDebug: Warning: Lasso::SetPulling - Pull started
LogGASDebug: Warning: Lasso::TickRetracting - Distance to owner: 150.0
```

---

## Example Event Graph (BP_Lasso)

```
┌─────────────────────────────────────────────────────────────┐
│ Event OnLassoThrown                                         │
│   └─► Play Sound 2D (Throw Whoosh SFX)                      │
│   └─► Play Animation Montage (Throw Anim)                   │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│ Event OnLassoCaughtTarget (CaughtTarget)                    │
│   └─► Play Sound at Location (Catch SFX)                    │
│   └─► Camera Shake (Light)                                  │
│   └─► Spawn Emitter at Location (Catch VFX)                 │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│ Event OnShowRopeWrapVisual (Target)                         │
│   └─► Spawn Actor (BP_RopeWrap)                             │
│   └─► Attach to Component (Target Root)                     │
│   └─► Store Reference (for cleanup)                         │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│ Event OnLassoReattached                                     │
│   └─► Get Owning Pawn → Get Mesh                            │
│   └─► Get Weapon Mesh                                       │
│   └─► Attach to Component (Socket: HandGrip_R_Lasso)        │
│   └─► Set Visibility (true)                                 │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│ Event OnRopeTensionChanged (Stretch, Force)                 │
│   └─► Map Range Clamped (Stretch: 0-500 → Scale: 1.0-2.0)   │
│   └─► Set Cable Width (Base * Scale)                        │
│   └─► (Optional) Spawn Strain Particles if Force > 300     │
└─────────────────────────────────────────────────────────────┘
```
