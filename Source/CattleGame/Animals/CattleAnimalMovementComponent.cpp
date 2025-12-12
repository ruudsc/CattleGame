// Copyright Epic Games, Inc. All Rights Reserved.

#include "CattleAnimalMovementComponent.h"
#include "GameFramework/Character.h"

UCattleAnimalMovementComponent::UCattleAnimalMovementComponent()
{
    // Enable tick for physics decay and influence processing
    PrimaryComponentTick.bCanEverTick = true;

    // Default movement settings for animals
    MaxWalkSpeed = WalkingSpeed;
    MaxAcceleration = 1024.0f;
    BrakingDecelerationWalking = 1024.0f;
    GroundFriction = 8.0f;

    // Allow rotation to follow movement
    bOrientRotationToMovement = true;
    RotationRate = FRotator(0.0f, 360.0f, 0.0f);

    // Disable controller rotation - let movement handle it
    bUseControllerDesiredRotation = false;
}

void UCattleAnimalMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
    // Decay physics velocity over time
    DecayPhysicsVelocity(DeltaTime);

    // Add physics velocity to movement
    if (!PhysicsVelocity.IsNearlyZero())
    {
        // Add physics velocity as additional velocity
        AddInputVector(PhysicsVelocity.GetSafeNormal(), true);
        Velocity += PhysicsVelocity * DeltaTime;
    }

    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UCattleAnimalMovementComponent::AddPhysicsImpulse(FVector Impulse, bool bVelocityChange)
{
    if (bVelocityChange)
    {
        // Directly set velocity (ignore mass)
        PhysicsVelocity += Impulse * PhysicsInfluenceMultiplier;
    }
    else
    {
        // Apply impulse considering mass
        float EffectiveMass = Mass > 0.0f ? Mass : 100.0f;
        PhysicsVelocity += (Impulse / EffectiveMass) * PhysicsInfluenceMultiplier;
    }

    ClampPhysicsVelocity();
}

void UCattleAnimalMovementComponent::AddPhysicsForce(FVector Force)
{
    // Apply force over time (expects to be called every frame)
    float EffectiveMass = Mass > 0.0f ? Mass : 100.0f;

    // F = ma -> a = F/m, then v += a * dt (dt handled in tick)
    PhysicsVelocity += (Force / EffectiveMass) * PhysicsInfluenceMultiplier * GetWorld()->GetDeltaSeconds();

    ClampPhysicsVelocity();
}

void UCattleAnimalMovementComponent::SetAreaInfluence(FVector Direction, float SpeedModifier)
{
    AreaInfluenceDirection = Direction.GetSafeNormal();
    AreaSpeedModifier = FMath::Clamp(SpeedModifier, 0.1f, 3.0f);
}

void UCattleAnimalMovementComponent::ClearAreaInfluence()
{
    AreaInfluenceDirection = FVector::ZeroVector;
    AreaSpeedModifier = 1.0f;
}

void UCattleAnimalMovementComponent::SetFlowDirection(FVector Direction)
{
    FlowDirection = Direction.GetSafeNormal();
}

void UCattleAnimalMovementComponent::SetMovementMode_Grazing()
{
    MaxWalkSpeed = GrazingSpeed * AreaSpeedModifier;
}

void UCattleAnimalMovementComponent::SetMovementMode_Walking()
{
    MaxWalkSpeed = WalkingSpeed * AreaSpeedModifier;
}

void UCattleAnimalMovementComponent::SetMovementMode_Panic()
{
    MaxWalkSpeed = PanicSpeed * AreaSpeedModifier;
}

void UCattleAnimalMovementComponent::PhysicsVolumeChanged(APhysicsVolume *NewVolume)
{
    Super::PhysicsVolumeChanged(NewVolume);
    // Could add water/special volume handling here
}

void UCattleAnimalMovementComponent::DecayPhysicsVelocity(float DeltaTime)
{
    if (!PhysicsVelocity.IsNearlyZero())
    {
        // Exponential decay
        PhysicsVelocity *= FMath::Exp(-PhysicsVelocityDecay * DeltaTime);

        // Zero out if very small
        if (PhysicsVelocity.SizeSquared() < 1.0f)
        {
            PhysicsVelocity = FVector::ZeroVector;
        }
    }
}

FVector UCattleAnimalMovementComponent::CalculateInfluencedVelocity(float DeltaTime) const
{
    FVector FinalDirection = FVector::ZeroVector;

    // Add area influence
    if (!AreaInfluenceDirection.IsNearlyZero())
    {
        FinalDirection += AreaInfluenceDirection * AreaInfluenceStrength;
    }

    // Add flow influence
    if (!FlowDirection.IsNearlyZero())
    {
        FinalDirection += FlowDirection * FlowInfluenceStrength;
    }

    return FinalDirection.GetSafeNormal() * MaxWalkSpeed;
}

void UCattleAnimalMovementComponent::ClampPhysicsVelocity()
{
    if (PhysicsVelocity.SizeSquared() > MaxPhysicsVelocity * MaxPhysicsVelocity)
    {
        PhysicsVelocity = PhysicsVelocity.GetSafeNormal() * MaxPhysicsVelocity;
    }
}
