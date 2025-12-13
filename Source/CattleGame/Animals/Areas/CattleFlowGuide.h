// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CattleFlowGuide.generated.h"

class USplineComponent;
class UBoxComponent;
class UTextureRenderTarget2D;
class UMaterialInterface;
class UMaterialInstanceDynamic;

/**
 * ACattleFlowGuide
 *
 * Paintable flowmap guide that encourages cattle to move in a specific direction.
 * Uses a spline to define the path and a render target for painted flow vectors.
 *
 * Editor workflow:
 * 1. Place actor and adjust spline to define guide path
 * 2. Set influence area with the box extent
 * 3. Use Editor Utility Widget brush to paint custom flow directions
 *
 * Place in level and search "CattleFlow" in actor panel.
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Cattle Flow Guide"))
class CATTLEGAME_API ACattleFlowGuide : public AActor
{
    GENERATED_BODY()

public:
    ACattleFlowGuide();

    // ===== Flow Configuration =====

    /** How strongly this flow guide influences animal movement */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cattle Flow", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float FlowStrength = 0.5f;

    /** Resolution of the flowmap texture */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cattle Flow|Texture", meta = (ClampMin = "64", ClampMax = "1024"))
    int32 FlowmapResolution = 256;

    /** Whether to use the painted flowmap (false = use spline direction only) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cattle Flow")
    bool bUsePaintedFlowmap = false;

    /** Priority when overlapping with other flow guides */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cattle Flow")
    int32 Priority = 0;

    // ===== Flow Sampling =====

    /** Sample the flow direction at a world location */
    UFUNCTION(BlueprintCallable, Category = "Cattle Flow")
    FVector SampleFlowAtLocation(const FVector &Location, float &OutWeight) const;

    /** Check if a location is within the flow guide area */
    UFUNCTION(BlueprintCallable, Category = "Cattle Flow")
    bool IsLocationInFlowArea(const FVector &Location) const;

    /** Get the flow direction from the spline at nearest point */
    UFUNCTION(BlueprintCallable, Category = "Cattle Flow")
    FVector GetSplineFlowDirection(const FVector &Location) const;

    // ===== Flowmap Painting (Editor) =====

    /** Initialize or reinitialize the flowmap render target */
    UFUNCTION(BlueprintCallable, Category = "Cattle Flow|Painting")
    void InitializeFlowmap();

    /** Paint flow direction at a world location (for editor brush tool) */
    UFUNCTION(BlueprintCallable, Category = "Cattle Flow|Painting")
    void PaintFlowAtLocation(const FVector &WorldLocation, const FVector &FlowDirection, float BrushRadius, float BrushStrength);

    /** Clear the flowmap to default (follow spline) */
    UFUNCTION(BlueprintCallable, Category = "Cattle Flow|Painting")
    void ClearFlowmap();

    /** Get the flowmap render target for visualization */
    UFUNCTION(BlueprintCallable, Category = "Cattle Flow|Painting")
    UTextureRenderTarget2D *GetFlowmapRenderTarget() const { return FlowmapRenderTarget; }

    /** Bake spline direction into flowmap */
    UFUNCTION(BlueprintCallable, Category = "Cattle Flow|Painting")
    void BakeSplineToFlowmap();

    // ===== Debug =====

    /** Draw debug visualization for this flow guide */
    UFUNCTION(BlueprintCallable, Category = "Cattle Flow|Debug")
    void DrawDebugFlow(float Duration = 0.0f) const;

    /** Color for debug visualization */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cattle Flow|Debug")
    FColor DebugColor = FColor::Cyan;

    /** Show debug in editor */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cattle Flow|Debug")
    bool bShowDebugInEditor = true;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void OnConstruction(const FTransform &Transform) override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent) override;
#endif

    // ===== Components =====

    /** Root scene component */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USceneComponent> SceneRoot;

    /** Spline defining the flow path */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USplineComponent> FlowSpline;

    /** Box defining the flow influence area */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UBoxComponent> InfluenceBox;

    // ===== Flowmap Data =====

    /** Render target for painted flowmap */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Cattle Flow|Texture")
    TObjectPtr<UTextureRenderTarget2D> FlowmapRenderTarget;

    /** CPU-side flowmap data for fast sampling */
    TArray<FVector2D> FlowmapData;

    // ===== Helper Functions =====

    /** Convert world location to UV coordinates */
    FVector2D WorldToFlowmapUV(const FVector &WorldLocation) const;

    /** Convert UV coordinates to world location */
    FVector FlowmapUVToWorld(const FVector2D &UV) const;

    /** Sample the flowmap texture at UV coordinates */
    FVector2D SampleFlowmapAtUV(const FVector2D &UV) const;

    /** Get the area subsystem */
    class UCattleAreaSubsystem *GetAreaSubsystem() const;

    /** Register with area subsystem */
    void RegisterWithSubsystem();

    /** Unregister from area subsystem */
    void UnregisterFromSubsystem();
};
