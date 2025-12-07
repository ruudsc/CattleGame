# Cattle Volume System: Editor User Guide

This guide explains how to use the **Cattle Volume System** in the Unreal Editor to shape AI behavior for cows (CattleAnimals).

## 1. Overview

The system uses **Component-Based Volumes**.
*   **The Container**: `ACattleVolume` (A shapeable 3D volume).
*   **The Behavior**: Logic Components (e.g., Guide, Avoid, Graze) added to the container.

This allows you to mix and match behaviors and use Unreal's standard **Brush Editing** tools to define complex shapes (like winding paths or specific grazing zones).

---

## 2. Basic Workflow

### Step 1: Place a Volume
1.  Open the **Place Actors** panel.
2.  Search for `CattleVolume`.
3.  Drag it into the level.

### Step 2: Shape the Volume
Since `ACattleVolume` inherits from `AVolume`, you can edit its shape using **Brush Editing**:
1.  Select the Volume in the Viewport.
2.  Switch to **Brush Editing Mode** (Shift + F5, or select "Brush Editing" in the Modes dropdown).
3.  Click on vertices/edges/faces to move them and shape the volume to fit your terrain.
4.  *Tip*: You can mostly just scale the default box if you don't need complex shapes.

### Step 3: Add Logic
1.  With the Volume selected, look at the **Details Panel**.
2.  Click the **+ Add** button (green button).
3.  Search for "Cattle" to find logic components:
    *   `Cattle Guide Logic`: For creating paths/flow.
    *   `Cattle Avoid Logic`: For repelling cattle.
    *   `Cattle Graze Logic`: For designating grazing areas.
4.  Select the one you need.

---

## 3. Creating Specific Volumes

### A. Creating a Guide Path (Flowmap)
*Encourages AI to follow a specific path.*

1.  **Place & Shape**: Place a `CattleVolume` covering the path area (e.g., a canyon or road).
2.  **Add Logic**: Add a `Cattle Guide Logic` component.
3.  **Add Spline**:
    *   Click **+ Add** again and add a `Spline` component.
    *   A Spline line will appear in the viewport.
4.  **Edit Spline**:
    *   Select the Spline points in the viewport.
    *   Hold `Alt` + Drag points to create new segments.
    *   Shape the spline to define the "Flow" direction.
    *   The Logic Component naturally directs cattle to follow this spline downstream.
5.  **Visualize**:
    *   (Future Feature) You should see debug arrows indicating flow direction inside the volume.

### B. Creating a Grazing Zone
*Marks an area where cattle are allowed/encouraged to eat.*

1.  **Place & Shape**: Place a `CattleVolume` over the grassy area.
2.  **Add Logic**: Add a `Cattle Graze Logic` component.
3.  **Configure Effect**:
    *   Select the `CattleGrazeLogic` in the Details panel.
    *   Find **Apply Effect Class**.
    *   Select a Gameplay Effect (e.g., `GE_Cattle_GrazingState`).
    *   *Result*: Any cow entering this zone gets this Gameplay Effect, which might grant a tag `State.Cattle.Grazing`. The AI Controller can read this tag to start grazing behavior.

### C. Creating an Avoidance Zone
*Repels cattle (e.g., fire, cliffs).*

1.  **Place & Shape**: Place a `CattleVolume`.
2.  **Add Logic**: Add a `Cattle Avoid Logic` component.
3.  **Configure**:
    *   Currently, this logic pushes cattle away from the **Center** of the volume.
    *   Useful for circular hazards.

---

## 4. Advanced Configuration (GAS)

All Logic Components can apply a **Gameplay Effect (GE)**.

*   **Apply Effect Class**: The GE class to apply when the animal **Enters** the volume.
*   **Removal**: The GE is automatically removed when the animal **Exits** the volume.

**Use Cases**:
*   *Speed Modifiers*: Apply a GE that boosts movement speed in a "Stampede Channel".
*   *State Tags*: Apply a `Tag.State.InWater` GE for water volumes to change movement animations.
