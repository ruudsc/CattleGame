# Lasso Blueprint Setup Instructions

## Overview
RDR2-style visual model: `HandCoilMesh → CableComponent → LoopMeshOnTarget`

---

## 1. BP_Lasso Configuration

### 1.1 Verify Parent Class
Ensure `BP_Lasso` has parent class `ALasso` (from `Lasso.h`).

### 1.2 Configure Projectile Class
Set **Projectile Class** to `BP_LassoProjectile` (invisible arc for hit detection)

### 1.3 Configure Loop Mesh Class
Set **Loop Mesh Class** to your loop mesh actor (a simple torus/ring that appears on captured targets).

### 1.4 Set Weapon Abilities
Add to `WeaponAbilities` array:

| Index | Ability Class | Input Action |
|-------|---------------|--------------|
| 0 | `GA_LassoThrow` | `IA_PrimaryFire` |
| 1 | `GA_LassoRelease` | `IA_SecondaryFire` |

### 1.5 Set Up Hand Coil Mesh
Select `HandCoilMesh` component and assign a static mesh (rope coil in hand)

| Property | Default | Description |
|----------|---------|-------------|
| `MaxConstraintLength` | 1200 | Max rope length before pull kicks in |
| `PullForce` | 2000 | Force applied when rope stretched |
| `PullReelSpeed` | 200 | How fast rope shortens while pulling |
| `ThrowCooldown` | 0.5 | Seconds before can throw again |
| `RetractDuration` | 0.5 | Retract animation duration |

### 1.6 Configure Cable (Optional)
The `RopeCable` component is the visual rope during tether:
- Set `Cable Width`, `Num Segments`, and material as desired
- Cable visibility is auto-managed by code

---

## 2. BP_LassoProjectile Configuration

### 2.1 Verify Parent Class
Ensure `BP_LassoProjectile` has parent class `ALassoProjectile`.

### 2.2 Configure Arc & Aim Assist
In the **Details** panel:

| Property | Default | Description |
|----------|---------|-------------|
| `InitialSpeed` | 2500 | Throw velocity |
| `GravityScale` | 0.5 | Arc curvature (0 = straight, 1 = full gravity) |
| `MaxFlightTime` | 1.5 | Auto-miss after this many seconds |
| `AimAssistRadius` | 200 | Detection sphere radius |
| `AimAssistAngle` | 30 | Cone half-angle (degrees) |
| `AimAssistLerpSpeed` | 8 | How quickly projectile curves to target |

### 2.3 Visual Loop (RDR2 Style)
1. Select the `RopeLoopMesh` component (StaticMesh).
2. Assign your Lasso Loop mesh (e.g. a simple torus/ring).
3. This creates the visual of the loop flying through the air.

---

## 3. Target Configuration (Lassoable Actors)

Use `LassoableComponent` to mark actors as lasso targets.

### 3.1 Add Component
1. Open your Cattle/NPC Character Blueprint.
2. Add a `LassoableComponent`.

### 3.2 Configure Attachment
In the **Details** panel of `LassoableComponent`:

| Property | Default | Description |
|----------|---------|-------------|
| `AttachSocketName` | `pelvis` | Bone/socket where loop attaches |
| `AttachOffset` | (0,0,0) | Position offset from socket |
| `AttachRotation` | (0,0,0) | Rotation offset for loop mesh |
| `LoopScale` | (1,1,1) | Scale of the loop mesh on this target |

### 3.3 Blueprint Events
The component provides events you can use for VFX/sound:
- `OnLassoCaptured` - Called when this target is caught
- `OnLassoReleased` - Called when released

### 3.4 Cattle Animals
`ACattleAnimal` automatically includes a `LassoableComponent` and reacts to being captured:
- Adds fear (configurable via `LassoFearAmount`)
- Switches to panic movement mode while lassoed

---

## 4. BP_Player / Inventory Integration

### 4.1 Add Lasso to Weapon Slots
In your player's `InventoryComponent`:
1. Ensure slot 1 is configured for the lasso
2. **Weapon Slot ID** in `BP_Lasso` should be `1`

### 4.2 Input Mapping
Ensure Enhanced Input actions are defined:
- `IA_PrimaryFire` - Left Mouse / Right Trigger
- `IA_SecondaryFire` - Right Mouse / Left Trigger

---

## 5. Testing Checklist

- [ ] Equip lasso via weapon switching
- [ ] Press primary fire → lasso throws with visible loop mesh
- [ ] Aim near a target → projectile curves toward it (aim assist)
- [ ] On hit → loop mesh attaches to target at configured socket
- [ ] Hold primary fire → both entities pulled toward each other
- [ ] Press secondary fire → target released, rope retracts
- [ ] Wait for cooldown → can throw again
- [ ] Cattle animals panic when lassoed

---

## 6. Troubleshooting

| Issue | Solution |
|-------|----------|
| Lasso doesn't throw | Check `CanFire()` - must be in Idle state with no cooldown |
| No arc visible | Increase `GravityScale` on projectile |
| Aim assist not working | Verify targets have `LassoableComponent` |
| Rope not visible | Check `RopeCable` material and visibility |
| Loop in wrong position | Adjust `AttachSocketName` and `AttachOffset` on target's `LassoableComponent` |
| Animal doesn't react | Ensure it's an `ACattleAnimal` or bind to `OnCapturedDelegate` |
