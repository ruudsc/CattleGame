#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LassoableComponent.generated.h"

class UStaticMeshComponent;

/**
 * Lassoable Component - Marks an actor as lassoable and provides attachment metadata.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class CATTLEGAME_API ULassoableComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULassoableComponent();

	virtual void BeginPlay() override;

	// ===== ATTACHMENT METADATA =====

	/** Socket/bone name where lasso loop should attach (e.g., "pelvis", "spine_02") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lasso|Attachment")
	FName AttachSocketName = FName("pelvis");

	/** Socket/bone name where the rope cable end attaches (for smooth cable following) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lasso|Attachment")
	FName RopeAttachSocketName = FName("LassoRopeAttachment");

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
	void OnCaptured(AActor *LassoWeaponOwner);

	/** Called when this target is released from lasso */
	UFUNCTION(BlueprintCallable, Category = "Lasso")
	void OnReleased();

	/** Get the world transform for loop mesh attachment */
	UFUNCTION(BlueprintCallable, Category = "Lasso")
	FTransform GetLoopAttachTransform() const;

	// ===== BLUEPRINT EVENTS =====

	/** Called when captured - for VFX, sound, animation triggers (BlueprintImplementableEvent) */
	UFUNCTION(BlueprintImplementableEvent, Category = "Lasso|Events")
	void OnLassoCaptured();

	/** Called when released (BlueprintImplementableEvent) */
	UFUNCTION(BlueprintImplementableEvent, Category = "Lasso|Events")
	void OnLassoReleased();

	// ===== NATIVE DELEGATES =====

	/** Native delegate for C++ classes to respond to capture */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLassoCapturedNative, AActor *, LassoOwner);

	/** Native delegate for C++ classes to respond to release */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLassoReleasedNative);

	/** Broadcast when captured - bind in C++ or Blueprint */
	UPROPERTY(BlueprintAssignable, Category = "Lasso|Events")
	FOnLassoCapturedNative OnCapturedDelegate;

	/** Broadcast when released - bind in C++ or Blueprint */
	UPROPERTY(BlueprintAssignable, Category = "Lasso|Events")
	FOnLassoReleasedNative OnReleasedDelegate;
};
