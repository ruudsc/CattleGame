// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "CattleAnimalMovementComponent.generated.h"

class UCattleAreaSubsystem;

/**
 * UCattleAnimalMovementComponent
 *
 * Custom character movement component for cattle animals that can be influenced by:
 * - Physics impulses (lasso, explosions)
 * - Area effects (graze, panic, avoid)
 * - Flow guides (directional flowmaps)
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class CATTLEGAME_API UCattleAnimalMovementComponent : public UCharacterMovementComponent
{
    GENERATED_BODY()

public:
    UCattleAnimalMovementComponent();

    // ===== PHYSICS INFLUENCE =====

    /** How much external physics impulses affect movement (0 = none, 1 = full) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cattle Movement|Physics")
    float PhysicsInfluenceMultiplier = 1.0f;

    /** Maximum velocity that can be applied from physics impulses */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cattle Movement|Physics")
    float MaxPhysicsVelocity = 1200.0f;

    /** How quickly physics velocity decays (per second) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cattle Movement|Physics")
    float PhysicsVelocityDecay = 5.0f;

    /** Current accumulated physics velocity from external forces */
    UPROPERTY(BlueprintReadOnly, Category = "Cattle Movement|Physics")
    FVector PhysicsVelocity = FVector::ZeroVector;

    /** Apply an impulse to the animal's movement */
    UFUNCTION(BlueprintCallable, Category = "Cattle Movement")
    void AddPhysicsImpulse(FVector Impulse, bool bVelocityChange = false);

    /** Apply a continuous force to the animal's movement */
    UFUNCTION(BlueprintCallable, Category = "Cattle Movement")
    void AddPhysicsForce(FVector Force);

    // ===== AREA INFLUENCE =====

    /** Current desired movement direction from area effects */
    UPROPERTY(BlueprintReadOnly, Category = "Cattle Movement|Area")
    FVector AreaInfluenceDirection = FVector::ZeroVector;

    /** Current movement speed modifier from area effects */
    UPROPERTY(BlueprintReadOnly, Category = "Cattle Movement|Area")
    float AreaSpeedModifier = 1.0f;

    /** How strongly area effects influence movement (0 = none, 1 = full) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cattle Movement|Area")
    float AreaInfluenceStrength = 1.0f;

    /** Set the current area influence (called by AI or area system) */
    UFUNCTION(BlueprintCallable, Category = "Cattle Movement")
    void SetAreaInfluence(FVector Direction, float SpeedModifier);

    /** Clear area influence */
    UFUNCTION(BlueprintCallable, Category = "Cattle Movement")
    void ClearAreaInfluence();

    // ===== FLOW GUIDE =====

    /** Current flow direction from flowmap sampling */
    UPROPERTY(BlueprintReadOnly, Category = "Cattle Movement|Flow")
    FVector FlowDirection = FVector::ZeroVector;

    /** How strongly flow guides influence movement */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cattle Movement|Flow")
    float FlowInfluenceStrength = 0.5f;

    /** Set the current flow direction (called by flow guide system) */
    UFUNCTION(BlueprintCallable, Category = "Cattle Movement")
    void SetFlowDirection(FVector Direction);

    // ===== MOVEMENT SPEEDS =====

    /** Base walking speed when grazing/idle */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cattle Movement|Speeds")
    float GrazingSpeed = 100.0f;

    /** Walking speed when moving normally */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cattle Movement|Speeds")
    float WalkingSpeed = 300.0f;

    /** Running speed when fleeing/panicked */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cattle Movement|Speeds")
    float PanicSpeed = 600.0f;

    /** Set movement speed based on behavior state */
    UFUNCTION(BlueprintCallable, Category = "Cattle Movement")
    void SetMovementMode_Grazing();

    UFUNCTION(BlueprintCallable, Category = "Cattle Movement")
    void SetMovementMode_Walking();

    UFUNCTION(BlueprintCallable, Category = "Cattle Movement")
    void SetMovementMode_Panic();

    // ===== OVERRIDES =====

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
    virtual void PhysicsVolumeChanged(APhysicsVolume *NewVolume) override;

protected:
    /** Apply physics velocity decay */
    void DecayPhysicsVelocity(float DeltaTime);

    /** Calculate final velocity including all influences */
    FVector CalculateInfluencedVelocity(float DeltaTime) const;

    /** Clamp physics velocity to max */
    void ClampPhysicsVelocity();
};
