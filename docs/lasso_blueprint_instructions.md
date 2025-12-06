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
*   **Targeting Logic**: The system now prefers the **Procedural Spline Wrap** if `LassoableComponent` is configured.
*   **Simple Loop (Fallback)**: You can still set **Loop Mesh Class** in `BP_Lasso` as a fallback.

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
| `AimAssistRadius` | 200 | Detection sphere radius |
| `AimAssistAngle` | 30 | Cone half-angle (degrees) |
| `AimAssistLerpSpeed` | 8 | How quickly projectile curves to target |

### 2.3 Visual Loop (RDR2 Style)
1. Select the `RopeLoopMesh` component (StaticMesh).
2. Assign your Lasso Loop mesh (e.g. a simple torus/ring).
3. This creates the visual of the loop flying through the air.

---

## 3. Target Configuration (Lassoable Actors)

**[NEW]** We use `LassoableComponent` instead of Tags.

### 3.1 Components
1. Open your Cattle/NPC Character Blueprint.
2. Add a `LassoableComponent`.
3. (Procedural Wrap) Review the `WrapSpline` component child.

### 3.2 Configure Procedural Wrap
1. Select `LassoableComponent`.
2. **Editor**: In the Viewport, edit the **WrapSpline** points to form a closed ring around the neck/horns.
3. **Details**: Assign a `WrapLoopActorClass`.
    - Create a Blueprint inheriting from `LassoLoopActor`.
    - In that BP, assign a Rope Mesh and Material.
4. **Preview**: Click the **Preview Wrap** button in the LassoableComponent Details panel to verifying the shape.

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
- [ ] Press primary fire → lasso throws with **Visible Loop Mesh**
- [ ] Aim near a target → projectile curves toward it (`LassoableComponent` detection)
- [ ] On hit → **Procedural Spline Mesh** wraps perfectly around the defined neck path
- [ ] Hold primary fire → both entities pulled toward each other
- [ ] Press secondary fire → target released, rope retracts
- [ ] Wait for cooldown → can throw again

---

## 6. Troubleshooting

| Issue | Solution |
|-------|----------|
| Lasso doesn't throw | Check `CanFire()` - must be in Idle state with no cooldown |
| No arc visible | Increase `GravityScale` on projectile |
| Aim assist not working | Verify targets have `LassoableComponent` |
| Rope not visible | Check `RopeCable` material and visibility |
| Wrap shape is wrong | Edit `WrapSpline` in the target Blueprint and click "Preview Wrap" |
