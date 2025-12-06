#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LassoableComponent.generated.h"

class UStaticMeshComponent;
class USplineComponent;

/**
 * Lassoable Component - Marks an actor as lassoable and provides attachment metadata.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CATTLEGAME_API ULassoableComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULassoableComponent();

	virtual void BeginPlay() override;
	virtual void OnRegister() override;

	// ===== PROCEDURAL WRAP =====

	/** Spline defining the wrap path (e.g., around neck) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lasso|Wrap")
	TObjectPtr<USplineComponent> WrapSpline;

	/** Actor class to spawn for procedural wrap (must have SplineMesh logic) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lasso|Wrap")
	TSubclassOf<AActor> WrapLoopActorClass;

	/** Temporary editor preview actor */
	UPROPERTY()
	TObjectPtr<AActor> EditorPreviewActor;

	/** PREVIEW WRAP BUTTON - Click to test wrap shape in editor */
	UFUNCTION(CallInEditor, Category = "Lasso|Wrap")
	void PreviewWrap();

	/** Clear the preview actor */
	UFUNCTION(CallInEditor, Category = "Lasso|Wrap")
	void ClearPreview();

	// ===== ATTACHMENT METADATA =====

	/** Socket/bone name where lasso loop should attach (e.g., "pelvis", "spine_02") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lasso|Attachment")
	FName AttachSocketName = FName("pelvis");

	/** Offset from socket for loop mesh placement */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lasso|Attachment")
	FVector AttachOffset = FVector::ZeroVector;

	/** Rotation offset for loop mesh */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lasso|Attachment")
	FRotator AttachRotation = FRotator::ZeroRotator;

	/** Scale for the loop mesh on this target */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lasso|Attachment")
	FVector LoopScale = FVector(1.0f);

	// ===== STATE =====

	/** Is this target currently lassoed? */
	UPROPERTY(BlueprintReadOnly, Category = "Lasso|State")
	bool bIsLassoed = false;

	/** The lasso weapon that caught this target */
	UPROPERTY(BlueprintReadOnly, Category = "Lasso|State")
	TWeakObjectPtr<AActor> LassoOwner;

	// ===== API =====

	/** Called when this target is caught by a lasso */
	UFUNCTION(BlueprintCallable, Category = "Lasso")
	void OnCaptured(AActor* LassoWeaponOwner);

	/** Called when this target is released from lasso */
	UFUNCTION(BlueprintCallable, Category = "Lasso")
	void OnReleased();

	/** Get the world transform for loop mesh attachment */
	UFUNCTION(BlueprintCallable, Category = "Lasso")
	FTransform GetLoopAttachTransform() const;

	// ===== BLUEPRINT EVENTS =====

	/** Called when captured - for VFX, sound, animation triggers */
	UFUNCTION(BlueprintImplementableEvent, Category = "Lasso|Events")
	void OnLassoCaptured();

	/** Called when released */
	UFUNCTION(BlueprintImplementableEvent, Category = "Lasso|Events")
	void OnLassoReleased();

protected:
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
