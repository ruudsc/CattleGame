#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LassoLoopActor.generated.h"

class UStaticMeshComponent;

/**
 * Simple lasso loop actor.
 * Displays a static mesh (torus/ring) to represent the lasso around a target.
 */
UCLASS()
class CATTLEGAME_API ALassoLoopActor : public AActor
{
	GENERATED_BODY()

public:
	ALassoLoopActor();

protected:
	virtual void BeginPlay() override;

	// The visible loop mesh
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> LoopMesh;

	// Scale of the loop mesh
	UPROPERTY(EditDefaultsOnly, Category = "Lasso|Visuals")
	float LoopScale = 1.0f;
};
