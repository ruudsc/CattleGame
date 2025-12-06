# GameplayCue Setup Guide

This document explains how to create GameplayCue Notify Blueprints in the Unreal Editor to handle VFX and Audio for the weapon abilities.

## Overview

GameplayCues are the GAS (Gameplay Ability System) way to handle cosmetic effects like VFX and audio in a replicated manner. The C++ code triggers GameplayCues via tags, and Blueprint assets respond to those tags to spawn Niagara effects, play sounds, etc.

### GameplayCue Tags Defined in Code

| Tag | Source | Type | Description |
|-----|--------|------|-------------|
| `GameplayCue.Trumpet.Lure` | GA_TrumpetLure | Looping | Active while holding LMB with trumpet |
| `GameplayCue.Trumpet.Scare` | GA_TrumpetScare | Looping | Active while holding RMB with trumpet |
| `GameplayCue.Lasso.Throw` | GA_LassoFire | One-shot | Triggered when lasso is thrown |
| `GameplayCue.Dynamite.Throw` | GA_DynamiteThrow | One-shot | Triggered when dynamite is thrown |
| `GameplayCue.Dynamite.Explode` | DynamiteProjectile | One-shot | Triggered when dynamite explodes |

## Creating GameplayCue Blueprints

### Step 1: Create the GameplayCue Notify Asset

1. In Content Browser, navigate to `Content/Core/Abilities/GameplayCues/` (create this folder if it doesn't exist)
2. Right-click → **Blueprint Class**
3. Search for the appropriate parent class:
   - **For looping effects (Trumpet):** `GameplayCueNotify_Actor`
   - **For one-shot effects (Lasso, Dynamite):** `GameplayCueNotify_Burst` or `GameplayCueNotify_Static`

### Step 2: Name the Blueprint

Use a consistent naming convention:
- `GC_TrumpetLure`
- `GC_TrumpetScare`
- `GC_LassoThrow`
- `GC_DynamiteThrow`
- `GC_DynamiteExplode`

### Step 3: Set the GameplayCue Tag

1. Open the Blueprint
2. In the **Class Defaults** panel, find `Gameplay Cue Tag`
3. Set it to the matching tag:
   - `GameplayCue.Trumpet.Lure`
   - `GameplayCue.Trumpet.Scare`
   - `GameplayCue.Lasso.Throw`
   - `GameplayCue.Dynamite.Throw`

---

## Looping Effects (GameplayCueNotify_Actor)

Use this for Trumpet abilities where effects should play continuously while the ability is active.

### Key Events to Override

| Event | When Called | Use For |
|-------|-------------|---------|
| `OnActive` | When GameplayCue is added | Spawn Niagara system, start looping audio |
| `WhileActive` | Every tick while active | Update effect parameters (optional) |
| `OnRemove` | When GameplayCue is removed | Stop/destroy Niagara, stop audio |

### Example: GC_TrumpetLure Setup

```
Event OnActive:
├── Spawn Niagara System Attached
│   ├── System Template: NS_TrumpetLure (your Niagara asset)
│   ├── Attach to: Target Actor → Get Mesh → "bell" socket (or weapon mesh)
│   └── Store reference in variable "ActiveNiagaraSystem"
│
└── Spawn Sound Attached
    ├── Sound: TrumpetLure_Loop (looping sound cue)
    ├── Attach to: Target Actor
    └── Store reference in variable "ActiveAudioComponent"

Event OnRemove:
├── If ActiveNiagaraSystem is valid:
│   └── Deactivate System (or Destroy Component)
│
└── If ActiveAudioComponent is valid:
    └── Stop (with fade out)
```

### Important Settings for Looping

In the Niagara System:
- Set loop behavior to **Infinite** or use a very long duration
- The system will be manually deactivated on `OnRemove`

For Audio:
- Use a **Sound Cue** with looping enabled, OR
- Use `Spawn Sound Attached` and keep a reference to stop it later

---

## One-Shot Effects (GameplayCueNotify_Burst)

Use this for Lasso and Dynamite where effects should play once and complete on their own.

### Key Events to Override

| Event | When Called | Use For |
|-------|-------------|---------|
| `OnBurst` | When GameplayCue is executed | Spawn one-shot Niagara, play sound |

### Example: GC_LassoThrow Setup

```
Event OnBurst:
├── Spawn Niagara System at Location
│   ├── System Template: NS_LassoThrow
│   ├── Location: Parameters → Effect Context → Get Origin
│   └── Auto Destroy: True
│
└── Play Sound at Location
    ├── Sound: LassoThrow_SFX
    └── Location: Parameters → Effect Context → Get Origin
```

### Example: GC_DynamiteThrow Setup

```
Event OnBurst:
├── Spawn Niagara System Attached
│   ├── System Template: NS_DynamiteThrow
│   ├── Attach to: Instigator (the character)
│   └── Socket: "HandGrip_R" or weapon socket
│
└── Play Sound at Location
    ├── Sound: DynamiteThrow_SFX
    └── Location: Effect Context Origin
```

---

## Accessing Context Data

GameplayCues receive a `FGameplayCueParameters` struct with useful data:

### In Blueprint

- **Get Instigator** - The character that triggered the cue
- **Get Effect Context** - Contains:
  - `Get Origin` - World location where effect was triggered
  - `Get Instigator` - Same as above
  - `Get Source Object` - The ability or effect that caused this
- **Get Target Actor** - The actor the cue is attached to (for Actor-based cues)

### Getting the Weapon

To attach effects to the weapon mesh:
```
Get Instigator → Cast to CattleCharacter → Get Inventory Component → Get Equipped Weapon → Get Weapon Mesh
```

---

## Folder Structure Recommendation

```
Content/
└── Core/
    └── Abilities/
        └── GameplayCues/
            ├── GC_TrumpetLure.uasset
            ├── GC_TrumpetScare.uasset
            ├── GC_LassoThrow.uasset
            └── GC_DynamiteThrow.uasset
```

---

## Testing

1. **PIE (Play In Editor):** Effects should trigger when using abilities
2. **Multiplayer Test:** Use "Number of Players: 2" to verify replication
   - Effects should appear on both clients when one player uses an ability
3. **Debug:** Enable `AbilitySystem.DebugGameplayCues 1` in console to see cue events

---

## Troubleshooting

### GameplayCue Not Firing

1. Verify the tag in Blueprint matches exactly: `GameplayCue.Trumpet.Lure`
2. Check that the GameplayCue asset is in a scanned path (Content folder)
3. Ensure the Ability System Component exists on the character

### Effects Not Replicating

1. GameplayCues are automatically replicated by the Ability System
2. Verify the ASC's replication mode is set correctly
3. Check network relevancy settings

### Audio Not Stopping (Looping Effects)

1. Ensure you're storing the AudioComponent reference in `OnActive`
2. Call `Stop()` on the component in `OnRemove`
3. Consider using `Fade Out` for smoother transitions

---

## Summary

| Weapon | GameplayCue Type | Blueprint Parent | Tag |
|--------|------------------|------------------|-----|
| Trumpet Lure | Looping | GameplayCueNotify_Actor | GameplayCue.Trumpet.Lure |
| Trumpet Scare | Looping | GameplayCueNotify_Actor | GameplayCue.Trumpet.Scare |
| Lasso | One-shot | GameplayCueNotify_Burst | GameplayCue.Lasso.Throw |
| Dynamite Throw | One-shot | GameplayCueNotify_Burst | GameplayCue.Dynamite.Throw |
| Dynamite Explode | One-shot | GameplayCueNotify_Burst | GameplayCue.Dynamite.Explode |

---

## Example: GC_DynamiteExplode Setup

This is a good example of a standalone GameplayCue triggered directly from a projectile (not via an AbilitySystemComponent).

```
Event OnBurst:
├── Spawn Niagara System at Location
│   ├── System Template: NS_Explosion
│   ├── Location: Parameters → Location (passed from C++)
│   └── Auto Destroy: True
│
├── Play Sound at Location
│   ├── Sound: Explosion_SFX
│   └── Location: Parameters → Location
│
└── (Optional) Camera Shake
    ├── Shake Class: CS_Explosion
    └── Location: Parameters → Location
```

**Note:** The DynamiteProjectile triggers this cue via `UGameplayCueManager::HandleGameplayCue()` directly, passing the explosion location in `CueParams.Location`. This bypasses the need for an ASC on the projectile.
