#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LassoLoopActor.generated.h"

class USplineComponent;
class USplineMeshComponent;

/**
 * Procedural rope loop actor.
 * Spawns SplineMeshComponents to visualize a rope wrapping around a target.
 */
UCLASS()
class CATTLEGAME_API ALassoLoopActor : public AActor
{
	GENERATED_BODY()
	
public:	
	ALassoLoopActor();

	// Initialize the loop visuals from a source spline (e.g. from LassoableComponent)
	UFUNCTION(BlueprintCallable, Category = "Lasso")
	void InitFromSpline(USplineComponent* SourceSpline);

protected:
	virtual void BeginPlay() override;

	// The spline that defines the shape
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USplineComponent> SplineComponent;

	// Array of created spline meshes
	UPROPERTY()
	TArray<TObjectPtr<USplineMeshComponent>> SplineMeshes;

	// Mesh to use for the rope segments
	UPROPERTY(EditDefaultsOnly, Category = "Lasso|Visuals")
	TObjectPtr<UStaticMesh> RopeMeshAsset;

	// Material override (optional)
	UPROPERTY(EditDefaultsOnly, Category = "Lasso|Visuals")
	TObjectPtr<UMaterialInterface> RopeMaterial;

	// Width/Scale of the rope
	UPROPERTY(EditDefaultsOnly, Category = "Lasso|Visuals")
	float RopeWidth = 3.0f;
};
