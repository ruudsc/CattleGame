# Lasso System – Implementation Plan (UE 5.6 + GAS)

## Overview
This document outlines the full implementation plan for a **Lasso System** in **CattleGame** (Unreal Engine 5.6) using **Gameplay Ability System (GAS)**, **Enhanced Input**, and modular weapon architecture.

### **Important Notes**
- **Legacy code removal**: All existing lasso-related code will be removed and replaced with this new implementation.
- **Breaking changes are acceptable**: This is a complete refactor to establish a proper GAS-based foundation.
- **Project-specific integration**: This system integrates with CattleGame's existing weapon and inventory systems.

---

## 1. System Goals
- Allow the player to throw a lasso projectile.
- If the projectile hits a target, establish a tether.
- While holding the fire input, maintain pulling force.
- On release, deactivate the pull and clean up the lasso.
- Support multiplayer, prediction, and replication.
- Replace all legacy lasso implementations with GAS-based system.

---

## 2. Legacy Code Removal
### **Files/Classes to Remove**
- Any existing lasso Blueprint actors not using GAS
- Old input bindings for lasso actions
- Non-GAS lasso components or systems
- Legacy tether/rope implementations

### **Migration Strategy**
1. Identify all existing lasso-related assets in Content browser
2. Back up current implementation (if needed for reference)
3. Remove old code before implementing new system
4. Update any dependent systems to use new GAS abilities

---

## 3. Required Components
### **3.1 Actor Classes**
- **ALassoProjectile** – handles collision, physics, and hit detection.
- **ALassoTether** (optional) – draws spline or cable.
- **ALasso** (weapon actor) – owns projectile class, animations, and editor configuration.

### **3.2 Gameplay Ability Classes**
- **UGameplayAbility_LassoFire** – activates on input *Started*, maintains pull *while held*, and stops *OnCompleted*.
- **UGameplayAbility_LassoPullEffect** (optional) – applies continuous pull.

### **3.3 Gameplay Effects**
- **GE_LassoCooldown** – optional cooldown effect
- No stamina/energy consumption required

### **3.4 Components**
- **UInventoryComponent** – gives abilities when weapon equipped (use existing CattleGame inventory).
- **UAbilitySystemComponent** – enables attribute and ability management (already present in CattleGame character).

---

## 4. Lasso Ability State Flow
### **Input Flow (Enhanced Input) → GAS**
| Input Event | Ability Task |
|-------------|--------------|
| **Started** | Spawn projectile → Begin tracing hit detection |
| **Triggered (held)** | Maintain pull force → Update lasso tension |
| **Completed / Canceled** | Destroy tether → Release target |

### **Ability Lifetime**
```
ActivateAbility
  ├─ Spawn projectile
  ├─ Wait for event: OnHit
  ├─ OnHit → Create tether → Start continuous pull
  ├─ Wait for input release
  ├─ EndAbility → cleanup
```

---

## 5. Implementation Steps
### **5.1 Remove Legacy Code**
1. Search Content browser for existing lasso assets
2. Delete or archive old lasso Blueprints
3. Remove old input mappings from project settings
4. Clean up any non-GAS lasso components

### **5.2 Create Lasso Projectile (ALassoProjectile)**
- Location: `Source/CattleGame/Weapons/Lasso/`
- Sphere collision set to **Query & Physics**.
- Projectile movement component (gravity enabled).
- OnHit delegate → calls back into the ability.
- Replicated movement.
- Tags identifying valid actors (e.g., `Target.Lassoable`).

### **5.3 Ability: GameplayAbility_LassoFire**
- Location: `Source/CattleGame/AbilitySystem/Abilities/`

#### Activation policy
```cpp
ActivationPolicy = EGameplayAbilityActivationPolicy::OnInputTriggered;
```
Or use **OnInputStarted** with Enhanced Input triggers.

#### Ability Tasks
- `UAbilityTask_SpawnActor`
- `UAbilityTask_WaitGameplayEvent` (OnHit)
- `UAbilityTask_Repeat` to apply pull force each frame
- `UAbilityTask_WaitInputRelease`

#### Pull Force Logic
- Get player's forward vector.
- Calculate direction to target.
- Apply force via:
  - `AddForce()` (physics), or
  - Move component logic if character-type.

### **5.4 Visual Tether (Cable or Spline)**
- Use **UCableComponent** on weapon.
- On projectile hit: attach cable end to hit component.
- On release: disable/hide cable.

---

## 6. Networking / Replication Notes
### **Projectile**
- Server spawns projectile.
- Client uses **prediction** for visuals.

### **Hit Events**
- Use **Gameplay Events** to notify ability.

### **Tether / Pull**
- Pull logic runs on the **server**.
- Visual cable runs on **client** only.

### Prediction Windows
If using **projectile prediction**, allow small correction window.

---

## 7. Weapon Integration (CattleGame Inventory System)
### Steps when equipping weapon
1. Update existing weapon base class to support lasso
2. Weapon gives ability `GameplayAbility_LassoFire` via CattleGame's inventory component
3. Ability binds to input action `IA_LassoFire` (create new Enhanced Input action)
4. Weapon provides data:
   - Projectile class
   - Pull strength
   - Max distance
   - Cable visuals

### **Breaking Changes**
- Old weapon equip logic may need updates to support GAS ability granting
- Existing input bindings for lasso must be removed and recreated

---

## 8. Editor Setup (CattleGame Content)
### **Content Browser Organization**
- Audio: `Content/Audio/Weapons/Lasso/`
  - Import `LassoThrow.wav`, `LassoPull.wav`
  - Create **MetaSound** assets for looping pull audio
- VFX: `Content/VFX/Weapons/Lasso/`
  - Projectile trail particle system
  - Hit impact VFX
- Blueprints: `Content/Blueprints/Weapons/Lasso/`
  - BP_Lasso (inherits from ALasso C++ class)
  - BP_LassoProjectile (inherits from ALassoProjectile)

### **Blueprint Setup**
- BP_Lasso: Configure mesh, skeletal mesh, and attachment points
- BP_LassoProjectile: Set collision profile and mesh
- Both should reference CattleGame's existing material library

### **Ability Editor Setup**
- Location: `Content/Blueprints/AbilitySystem/Abilities/`
- Set ability tags:
  - `Ability.Weapon.Lasso`
  - `Ability.Action.PrimaryFire`
- Set replication policy: **ServerInitiated**
- Configure input bindings to use CattleGame's Enhanced Input system

---

## 9. Cleanup Logic
- On ability end:
  - Destroy projectile.
  - Clear cable/tether.
  - Stop looping pull audio.
  - Remove force from target.

- Ensure cleanup happens on:
  - Input release
  - Owner death
  - Weapon switch
  - Projectile timeout

---

## 10. Optional Extras
### **Skill Upgrades** (Future Consideration)
- Increased pull strength
- Fire multiple ropes
- Stun targets when lassoed

### **Animations**
- Add montage for throw (use CattleGame's character animation blueprint)
- Add montage for maintaining pull
- Integrate with existing character locomotion system

### **Camera Shake**
- When rope becomes taut
- Use CattleGame's existing camera shake framework

---

## 11. Debugging Tools
- Draw debug lines for tether
- Log projectile hit events
- Print ability activation state
- Use `AbilitySystemDebugger` in PIE
- Show rope tension number on HUD

---

## 12. Implementation Checklist
- [ ] Remove all legacy lasso code and assets
- [ ] Create C++ classes: ALassoProjectile, ALasso
- [ ] Create GameplayAbility_LassoFire
- [ ] Set up Enhanced Input action (IA_LassoFire)
- [ ] Integrate with CattleGame inventory system
- [ ] Create Blueprint derivatives (BP_Lasso, BP_LassoProjectile)
- [ ] Import and set up audio assets
- [ ] Create VFX for projectile and impact
- [ ] Configure ability tags and replication
- [ ] Add cable/tether visual component
- [ ] Test in single-player
- [ ] Test in multiplayer/replication
- [ ] Create debug visualization tools
- [ ] Document any breaking changes for team

---

## Final Notes
This system is **modular**, **GAS-native**, replicates cleanly, and is designed for CattleGame's existing architecture. **Breaking changes from legacy systems are expected and acceptable** as this establishes a proper foundation for future weapon systems.

