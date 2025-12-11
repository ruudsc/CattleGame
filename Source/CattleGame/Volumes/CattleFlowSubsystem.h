#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ICattleFlowSource.h"
#include "CattleFlowSubsystem.generated.h"

/**
 * World Subsystem that manages proximity-based cattle flow sources (splines).
 * Volume-based sources use direct overlap events and don't need this subsystem.
 * Spline-based sources register here and cattle query for nearby influences.
 */
UCLASS()
class CATTLEGAME_API UCattleFlowSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // USubsystem interface
    virtual void Initialize(FSubsystemCollectionBase &Collection) override;
    virtual void Deinitialize() override;
    virtual bool ShouldCreateSubsystem(UObject *Outer) const override;

    /**
     * Register a proximity-based flow source (e.g., guide spline).
     * Called by the source actor on BeginPlay.
     */
    UFUNCTION(BlueprintCallable, Category = "Cattle Flow")
    void RegisterProximitySource(TScriptInterface<ICattleFlowSource> Source);

    /**
     * Unregister a proximity-based flow source.
     * Called by the source actor on EndPlay.
     */
    UFUNCTION(BlueprintCallable, Category = "Cattle Flow")
    void UnregisterProximitySource(TScriptInterface<ICattleFlowSource> Source);

    /**
     * Query all proximity-based sources within range of a location.
     * @param Location World location to query from.
     * @param QueryRadius Maximum distance to search (uses source's InfluenceRadius if larger).
     * @return Array of nearby flow sources.
     */
    UFUNCTION(BlueprintCallable, Category = "Cattle Flow")
    TArray<TScriptInterface<ICattleFlowSource>> QueryNearbyProximitySources(const FVector &Location, float QueryRadius) const;

    /**
     * Get all registered proximity sources (for debugging).
     */
    UFUNCTION(BlueprintCallable, Category = "Cattle Flow")
    const TArray<TScriptInterface<ICattleFlowSource>> &GetAllProximitySources() const { return RegisteredProximitySources; }

protected:
    /** All registered proximity-based flow sources (splines) */
    UPROPERTY()
    TArray<TScriptInterface<ICattleFlowSource>> RegisteredProximitySources;
};
