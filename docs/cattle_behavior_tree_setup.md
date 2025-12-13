# Cattle AI Behavior Tree Setup Guide

This guide walks you through setting up the behavior tree for cattle AI in Unreal Editor with full player interaction support.

## Prerequisites

- Blackboard asset created with all required keys (see Key Reference below)
- Project compiled with the AI C++ classes

---

## Step 1: Create the Blackboard Asset

1. In **Content Browser**, right-click → **Artificial Intelligence** → **Blackboard**
2. Name it `BB_CattleAnimal`
3. Add the following keys:

| Key Name | Type | Notes |
|----------|------|-------|
| `TargetLocation` | Vector | Movement destination |
| `TargetActor` | Object (Base: Actor) | Target to approach/flee from |
| `CurrentAreaType` | Int | Use Int since enum may not be visible |
| `FearLevel` | Float | **0.0 to 1.0** - Percentage (0% to 100%) |
| `IsPanicked` | Bool | Panic state |
| `FlowDirection` | Vector | Flow guide direction |
| `HomeLocation` | Vector | Spawn location for wandering |
| `WanderRadius` | Float | Default: 1000 |
| `NearestThreat` | Object (Base: Actor) | Set by CheckNearbyThreats |
| `ThreatDistance` | Float | Distance to nearest threat |
| `HerdDirection` | Vector | Output from HerdBehavior service |
| `NearbyExplosive` | Object (Base: Actor) | **PLAYER ACTION** - Dynamite nearby |
| `IsBeingLured` | Bool | **PLAYER ACTION** - Trumpet lure active |
| `LurerActor` | Object (Base: Actor) | **PLAYER ACTION** - Player doing the luring |
| `IsBeingScared` | Bool | **PLAYER ACTION** - Trumpet scare active |
| `ScarerActor` | Object (Base: Actor) | **PLAYER ACTION** - Player doing the scaring |
| `IsPlayerShooting` | Bool | **PLAYER ACTION** - Player firing nearby |
| `ShooterActor` | Object (Base: Actor) | **PLAYER ACTION** - Player shooting |

---

## Step 2: Create the Behavior Tree Asset

1. In **Content Browser**, right-click → **Artificial Intelligence** → **Behavior Tree**
2. Name it `BT_Animal`
3. Double-click to open the Behavior Tree Editor
4. Click **Root** node → Set **Blackboard Asset** to `BB_CattleAnimal`

---

## Step 3: Build the Complete Tree Structure

### Overview Structure

```
Root
└── Selector (Main)
    ├── [0] LASSOED (Highest Priority)
    ├── [1] DYNAMITE NEARBY
    ├── [2] TRUMPET SCARE
    ├── [3] GUNSHOT SCARED (shooting + high fear)
    ├── [4] GUNSHOT ALERT (shooting + low fear)
    ├── [5] TRUMPET LURE
    ├── [6] PANIC (Fear 0.7-1.0)
    ├── [7] PANIC AREA
    ├── [8] ALERT (Fear 0.3-0.7 + Threat Nearby)
    ├── [9] FLOW GUIDE
    ├── [10] AVOID AREA
    └── [11] CALM (Fear 0.0-0.3) - Default
```

---

## Step 4: Build Each Branch

### Branch 0: LASSOED (Highest Priority)

**Purpose**: Stop all AI behavior when captured by lasso

1. Add **Sequence** node as FIRST child of main Selector
2. Right-click Sequence → **Add Decorator** → `BTDecorator_IsLassoed`
   - **Observer Aborts**: `Both` ← Immediately interrupts other behaviors
3. Add child **Task** → `BTTask_Wait` (built-in)
   - Wait Time: `999.0` (effectively infinite - wait until released)
   
> **Note**: When lassoed, the cattle's movement is controlled by the lasso physics constraint, not AI. This branch just keeps the BT occupied until released.

---

### Branch 1: DYNAMITE NEARBY

**Purpose**: Flee from nearby fusing dynamite

1. Add **Sequence** node
2. Right-click Sequence → **Add Decorator** → `BTDecorator_IsExplosiveNearby`
   - Nearby Explosive Key: `NearbyExplosive`
   - **Observer Aborts**: `Both`
3. Right-click Sequence → **Add Service** → `BTService_DetectPlayerActions`
   - Nearby Explosive Key: `NearbyExplosive`
   - Explosive Detection Radius: `800.0`
4. Right-click Sequence → **Add Service** → `BTService_UpdateCattleState`
5. Add child Task → `BTTask_FleeFromActor`
   - Actor To Flee From Key: `NearbyExplosive`
   - Target Location Key: `TargetLocation`
   - Flee Distance: `1000.0`
6. Add child Task → `BTTask_MoveTo`
   - Blackboard Key: `TargetLocation`

---

### Branch 2: TRUMPET SCARE

**Purpose**: Flee from player using trumpet scare

1. Add **Sequence** node
2. Right-click Sequence → **Add Decorator** → `BTDecorator_Blackboard` (built-in)
   - Blackboard Key: `IsBeingScared`
   - Key Query: `Is Set`
   - **Observer Aborts**: `Both`
3. Right-click Sequence → **Add Service** → `BTService_DetectPlayerActions`
   - Is Being Scared Key: `IsBeingScared`
   - Scarer Actor Key: `ScarerActor`
4. Right-click Sequence → **Add Service** → `BTService_UpdateCattleState`
5. Add child Task → `BTTask_FleeFromActor`
   - Actor To Flee From Key: `ScarerActor`
   - Target Location Key: `TargetLocation`
   - Flee Distance: `800.0`
6. Add child Task → `BTTask_MoveTo`
   - Blackboard Key: `TargetLocation`

---

### Branch 3: GUNSHOT SCARED (High Fear + Shooting)

**Purpose**: Flee in panic when scared and player is shooting

1. Add **Sequence** node
2. Right-click Sequence → **Add Decorator** → `BTDecorator_IsPlayerShooting`
   - Is Player Shooting Key: `IsPlayerShooting`
   - Fear Level Key: `FearLevel`
   - Require Min Fear: `✓`
   - Min Fear Level: `0.3`
   - **Observer Aborts**: `Both`
3. Right-click Sequence → **Add Service** → `BTService_DetectPlayerActions`
4. Right-click Sequence → **Add Service** → `BTService_UpdateCattleState`
5. Add child Task → `BTTask_FleeFromActor`
   - Actor To Flee From Key: `ShooterActor`
   - Target Location Key: `TargetLocation`
   - Flee Distance: `1500.0`
6. Add child Task → `BTTask_MoveTo`
   - Blackboard Key: `TargetLocation`

---

### Branch 4: GUNSHOT ALERT (Low Fear + Shooting)

**Purpose**: Look at shooter curiously when not scared

1. Add **Sequence** node
2. Right-click Sequence → **Add Decorator** → `BTDecorator_IsPlayerShooting`
   - Is Player Shooting Key: `IsPlayerShooting`
   - Fear Level Key: `FearLevel`
   - Require Max Fear: `✓`
   - Max Fear Level: `0.3`
   - **Observer Aborts**: `Both`
3. Right-click Sequence → **Add Service** → `BTService_DetectPlayerActions`
4. Right-click Sequence → **Add Service** → `BTService_UpdateCattleState`
5. Right-click Sequence → **Add Service** → `BTService_CheckNearbyThreats`
   - (This builds fear while watching)
6. Add child Task → `BTTask_LookAtActor`
   - Actor To Look At Key: `ShooterActor`
7. Add child Task → `BTTask_Wait`
   - Wait Time: `1.0`

---

### Branch 5: TRUMPET LURE

**Purpose**: Follow player when lured and calm enough

1. Add **Sequence** node
2. Right-click Sequence → **Add Decorator** → `BTDecorator_IsBeingLured`
   - Is Being Lured Key: `IsBeingLured`
   - Fear Level Key: `FearLevel`
   - Max Fear For Lure: `0.3` (only lured when calm)
   - **Observer Aborts**: `Both`
3. Right-click Sequence → **Add Service** → `BTService_DetectPlayerActions`
   - Is Being Lured Key: `IsBeingLured`
   - Lurer Actor Key: `LurerActor`
4. Right-click Sequence → **Add Service** → `BTService_UpdateCattleState`
5. Add child Task → `BTTask_FollowActor`
   - Actor To Follow Key: `LurerActor`
   - Target Location Key: `TargetLocation`
   - Acceptable Radius: `300.0`
6. Add child Task → `BTTask_MoveTo`
   - Blackboard Key: `TargetLocation`
   - Acceptable Radius: `200.0`

---

### Branch 6: PANIC BEHAVIOR (High Fear)

**Purpose**: Full panic mode when fear is very high

1. Add **Sequence** node under main Selector
2. Right-click Sequence → **Add Decorator** → `BTDecorator_FearLevel`
   - Min Fear Level: `0.7`
   - Max Fear Level: `1.0`
   - **Observer Aborts**: `Both` ← Important!
3. Right-click Sequence → **Add Service** → `BTService_CheckNearbyThreats`
   - Threat Detection Radius: `2500.0` (Unreal units, ~25 meters)
   - Players Are Threat: `✓`
   - Max Fear Per Second: `60.0` (raw fear units/sec, attribute uses 0-100 scale)
   - Fear Start Distance: `2000.0` (fear starts building when closer than this)
   - Nearest Threat Key: `NearestThreat`
   - Threat Distance Key: `ThreatDistance`
4. Right-click Sequence → **Add Service** → `BTService_UpdateCattleState`
   - Interval: `0.1`
5. Add child **Task** → `BTTask_CattleFlee`
   - Flee Distance: `2500.0`
   - Random Angle Variation: `45.0`
   - Threat Actor Key: `NearestThreat`
   - Target Location Key: `TargetLocation`
6. Add child **Task** → `BTTask_MoveTo` (built-in)
   - Blackboard Key: `TargetLocation`
   - Acceptable Radius: `100.0`

---

### Branch 7: PANIC AREA

**Purpose**: Flee when inside a panic-inducing area

1. Add **Sequence** node
2. Add Decorator → `BTDecorator_IsInAreaType`
   - Required Area Type: `Panic` (value 40)
   - **Observer Aborts**: `Both`
3. Add Service → `BTService_UpdateCattleState`
4. Add child Task → `BTTask_CattleFlee`
   - Flee Distance: `1500.0`
5. Add child Task → `BTTask_MoveTo`
   - Blackboard Key: `TargetLocation`

---

### Branch 8: ALERT BEHAVIOR (Medium Fear + Threat Nearby)

**Purpose**: Cautious behavior - maintain distance from player

1. Add **Sequence** node
2. Add Decorator → `BTDecorator_FearLevel`
   - Min Fear Level: `0.3`
   - Max Fear Level: `0.7`
   - **Observer Aborts**: `Both`
3. Add Decorator → `BTDecorator_HasNearbyThreat`
   - Threat Distance Key: `ThreatDistance`
   - Max Threat Distance: `1500.0`
4. Add Service → `BTService_CheckNearbyThreats`
   - Threat Detection Radius: `1500.0`
   - Players Are Threat: `✓`
   - Max Fear Per Second: `35.0`
   - Fear Start Distance: `1200.0`
5. Add Service → `BTService_UpdateCattleState`
6. Add child **Selector** node for alert behaviors:

#### Sub-branch 8a: Keep Distance (threat too close)
1. Add **Sequence** under the Selector
2. Add Decorator → `BTDecorator_HasNearbyThreat`
   - Max Threat Distance: `600.0`
3. Add Task → `BTTask_CattleFlee`
   - Flee Distance: `400.0`
   - Random Angle Variation: `30.0`
4. Add Task → `BTTask_MoveTo`
   - Blackboard Key: `TargetLocation`

#### Sub-branch 8b: Watch and Wait (threat at medium distance)
1. Add **Sequence** under the Selector
2. Add Task → `BTTask_Wait` (built-in)
   - Wait Time: `2.0`
   - Random Deviation: `1.0`

---

### Branch 9: FLOW GUIDE

**Purpose**: Follow flow guide areas

1. Add **Sequence** node
2. Add Decorator → `BTDecorator_IsInAreaType`
   - Required Area Type: `FlowGuide` (value 20)
3. Add Decorator → `BTDecorator_HasFlowDirection`
   - Min Magnitude: `0.1`
4. Add Service → `BTService_UpdateCattleState`
5. Add Task → `BTTask_CattleFollowFlow`
   - Flow Move Distance: `500.0`
6. Add Task → `BTTask_MoveTo`
   - Blackboard Key: `TargetLocation`

---

### Branch 10: AVOID AREA

**Purpose**: Move away from avoid areas

1. Add **Sequence** node
2. Add Decorator → `BTDecorator_IsInAreaType`
   - Required Area Type: `Avoid` (value 30)
3. Add Service → `BTService_UpdateCattleState`
4. Add Task → `BTTask_CattleFlee`
   - Flee Distance: `800.0`
5. Add Task → `BTTask_MoveTo`
   - Blackboard Key: `TargetLocation`

---

### Branch 11: CALM BEHAVIOR (Default)

**Purpose**: Normal grazing and wandering when calm

1. Add **Sequence** node
2. Add Decorator → `BTDecorator_FearLevel`
   - Min Fear Level: `0.0`
   - Max Fear Level: `0.3`
3. Add Service → `BTService_UpdateCattleState`
   - Interval: `0.1`
4. Add Service → `BTService_HerdBehavior`
   - Herd Detection Radius: `1500.0`
   - Cohesion Weight: `0.3`
   - Alignment Weight: `0.2`
   - Separation Weight: `0.5`
   - Separation Distance: `200.0`
5. Add Service → `BTService_CheckNearbyThreats` (passive awareness)
   - Threat Detection Radius: `800.0`
   - Players Are Threat: `✓`
   - Max Fear Per Second: `15.0`
   - Fear Start Distance: `500.0`
6. Add child **Selector** for Graze/Wander:

#### Sub-branch 11a: Grazing
1. Add **Sequence**
2. Add Decorator → `BTDecorator_IsInAreaType`
   - Required Area Type: `Graze` (value 10)
3. Add Task → `BTTask_CattleGraze`
   - Min Graze Time: `4.0`
   - Max Graze Time: `10.0`

#### Sub-branch 11b: Wandering
1. Add **Sequence**
2. Add Task → `BTTask_CattleWander`
   - Min Wander Distance: `200.0`
   - **Home Location Key**: `HomeLocation` ← MUST SET THIS
   - **Wander Radius Key**: `WanderRadius` ← MUST SET THIS
   - **Target Location Key**: `TargetLocation` ← MUST SET THIS
3. Add Task → `BTTask_MoveTo`
   - Blackboard Key: `TargetLocation`
   - Acceptable Radius: `50.0`
4. Add Task → `BTTask_Wait`
   - Wait Time: `1.0`
   - Random Deviation: `0.5`

---

## Step 5: Assign to AI Controller

1. Open `AIC_Animal` Blueprint
2. Click **Class Defaults**
3. Set **Behavior Tree** to `BT_Animal`
4. Set **Default Wander Radius** to `1000.0` (or desired value in Unreal units)
5. Save and Compile

---

## Player Interaction Behavior Summary

| Player Distance | Animal Fear | Behavior |
|-----------------|-------------|----------|
| > 1500 units | Low (0.0-0.3 = 0-30%) | Normal grazing/wandering, passive awareness |
| 800-1500 units | Rising (0.3+ = 30%+) | Alert, watching player, fear building |
| 600-800 units | Medium (0.3-0.7 = 30-70%) | Keeps distance, moves away slowly |
| < 600 units | High (0.7+ = 70%+) | Full panic, flees far away |
| In Panic Area | Any | Immediate flee |

### Fear Dynamics

- **Fear range**: 0.0 to 1.0 (0% to 100%)
- **Fear increases** when player is within `FearStartDistance` of any CheckNearbyThreats service
- **Fear decays** naturally over time (faster in Graze areas) 
- **Observer Aborts: Both** ensures high-priority behaviors interrupt lower ones immediately

---

## Testing Checklist

- [ ] Animals graze/wander when player is far away
- [ ] Animals become alert when player approaches (~1000 units)
- [ ] Animals back away when player gets close (~600 units)
- [ ] Animals flee in panic when player rushes them
- [ ] Fear decays when player moves away
- [ ] Animals follow flow guides correctly
- [ ] Animals flee from panic areas
- [ ] Herd stays loosely together while moving

---

## Troubleshooting

### BT shows "Inactive"
- Check that `AIC_Animal` has Behavior Tree property set
- Check that `BP_Bison` (or animal BP) has AI Controller Class set to `AIC_Animal`
- Check Auto Possess AI is `Placed in World or Spawned`

### Animals don't react to player
- Verify `bPlayersAreThreat` is checked on CheckNearbyThreats services
- Check that player character is NOT derived from `ACattleAnimal`
- Enable LogCattleAI in Output Log to see threat detection

### Animals stuck / don't move
- Verify NavMesh covers the area (press P in editor)
- Check that TargetLocation is being set (enable LogCattleAI)
- Ensure MoveTo task has correct blackboard key selected

### Wander shows "HomeLocation: 0,0,0" or "WanderRadius: 0"
- **On BTTask_CattleWander**: Make sure all 3 blackboard key selectors are set:
  - Home Location Key → `HomeLocation`
  - Wander Radius Key → `WanderRadius`
  - Target Location Key → `TargetLocation`
- **On AIC_Animal Blueprint**: Check Default Wander Radius is not 0 (should be ~1000)

### Fear not building up
- Check `FearStartDistance` on CheckNearbyThreats services
- Verify `MaxFearPerSecond` is set high enough
- Check `ThreatDetectionRadius` covers the player

### Fear value appears wrong (like 801 instead of 0.8)
- **801 is likely `ThreatDistance`, not `FearLevel`!** When no threat is detected, ThreatDistance defaults to `ThreatDetectionRadius + 1` (e.g., 800 + 1 = 801)
- Make sure you're looking at the `FearLevel` blackboard key (0.0-1.0), not `ThreatDistance` (in Unreal units)
- The decorator uses percentage (0.0-1.0) for easier configuration
- Raw fear attribute in AnimalAttributeSet ranges from 0-100 by default

### Understanding the values
| Property | Scale | Meaning |
|----------|-------|---------|
| `FearLevel` (blackboard) | 0.0-1.0 | Percentage (0% to 100%) |
| `ThreatDistance` (blackboard) | Unreal units | Distance to nearest threat in cm (~801 = no threat) |
| `MaxFearPerSecond` (service) | Raw units | Fear added per second (0-100 attribute scale) |
| `FearStartDistance` (service) | Unreal units | Fear builds when closer than this |
| `ThreatDetectionRadius` (service) | Unreal units | Max range to detect threats |

---

## Custom Node Reference

### Tasks
| Node | Description |
|------|-------------|
| **BTTask_CattleWander** | Picks random point within WanderRadius of HomeLocation |
| **BTTask_CattleGraze** | Stops movement, plays grazing for random duration |
| **BTTask_CattleFlee** | Moves away from threat or panic area direction |
| **BTTask_CattleFollowFlow** | Moves in the FlowDirection from area |
| **BTTask_FleeFromActor** | Flees from a blackboard actor (FleeDistance, RandomAngleVariation) |
| **BTTask_FollowActor** | Sets TargetLocation toward a blackboard actor (AcceptableRadius) |
| **BTTask_LookAtActor** | Sets AI focal point toward a blackboard actor (LookDuration) |

### Services
| Node | Description |
|------|-------------|
| **BTService_UpdateCattleState** | Syncs animal state to blackboard |
| **BTService_CheckNearbyThreats** | Scans for players, adds fear based on proximity |
| **BTService_HerdBehavior** | Calculates flocking direction (cohesion/alignment/separation) |
| **BTService_DetectPlayerActions** | Detects explosives, trumpet effects, shooting; updates player action blackboard keys |

### Decorators
| Node | Description |
|------|-------------|
| **BTDecorator_IsLassoed** | True if cattle is captured by lasso (highest priority) |
| **BTDecorator_IsCattlePanicked** | True if animal panic state active |
| **BTDecorator_IsInAreaType** | True if in specified area type |
| **BTDecorator_HasFlowDirection** | True if flow vector has magnitude |
| **BTDecorator_FearLevel** | True if fear within min/max range |
| **BTDecorator_HasNearbyThreat** | True if threat within distance |
| **BTDecorator_IsExplosiveNearby** | True if NearbyExplosive is set (dynamite proximity) |
| **BTDecorator_IsBeingLured** | True if IsBeingLured AND fear below MaxFearForLure threshold |
| **BTDecorator_IsPlayerShooting** | True if IsPlayerShooting with optional fear level conditions |

---

## Testing Tips

1. **Enable AI Debugging**: Press `'` (apostrophe) in-game to show AI debug
2. **Behavior Tree Debugger**: While playing, open BT asset and click "Debug" dropdown to select an AI
3. **Blackboard Inspector**: Shows current values of all blackboard keys
4. **Visual Logger**: `VisLog` command shows AI decision history
5. **LogCattleAI**: Filter Output Log by `LogCattleAI` for detailed AI logging
