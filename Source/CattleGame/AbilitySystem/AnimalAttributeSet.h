// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"
#include "AnimalAttributeSet.generated.h"

/**
 * UAnimalAttributeSet
 *
 * Attributes specific to cattle animals for the behavior system.
 * Works with GameplayEffects to modify animal behavior states.
 */
UCLASS()
class CATTLEGAME_API UAnimalAttributeSet : public UAttributeSet
{
    GENERATED_BODY()

public:
    UAnimalAttributeSet();

    // ===== Fear/Panic Attributes =====

    /** Current fear level (0 = calm, 100 = maximum panic) */
    UPROPERTY(BlueprintReadOnly, Category = "Animal|Fear", ReplicatedUsing = OnRep_Fear)
    FGameplayAttributeData Fear;

    /** Maximum fear threshold before panic state */
    UPROPERTY(BlueprintReadOnly, Category = "Animal|Fear", ReplicatedUsing = OnRep_MaxFear)
    FGameplayAttributeData MaxFear;

    /** Rate at which fear naturally decays per second */
    UPROPERTY(BlueprintReadOnly, Category = "Animal|Fear", ReplicatedUsing = OnRep_FearDecayRate)
    FGameplayAttributeData FearDecayRate;

    // ===== Calm/Lure Attributes =====

    /** Current calm level - counteracts fear */
    UPROPERTY(BlueprintReadOnly, Category = "Animal|Calm", ReplicatedUsing = OnRep_CalmLevel)
    FGameplayAttributeData CalmLevel;

    /** How susceptible the animal is to lure effects (1.0 = normal) */
    UPROPERTY(BlueprintReadOnly, Category = "Animal|Calm", ReplicatedUsing = OnRep_LureSusceptibility)
    FGameplayAttributeData LureSusceptibility;

    // ===== Herd Attributes =====

    /** How strongly this animal follows herd behavior (0 = independent, 1 = strong herd instinct) */
    UPROPERTY(BlueprintReadOnly, Category = "Animal|Herd", ReplicatedUsing = OnRep_HerdAffinity)
    FGameplayAttributeData HerdAffinity;

    /** Radius within which this animal considers others as part of its herd */
    UPROPERTY(BlueprintReadOnly, Category = "Animal|Herd", ReplicatedUsing = OnRep_HerdRadius)
    FGameplayAttributeData HerdRadius;

    // ===== Movement Attributes =====

    /** Movement speed modifier from current state */
    UPROPERTY(BlueprintReadOnly, Category = "Animal|Movement", ReplicatedUsing = OnRep_SpeedModifier)
    FGameplayAttributeData SpeedModifier;

    // ===== Meta Attributes (not replicated) =====

    /** Incoming fear change (meta attribute for calculations) */
    UPROPERTY(BlueprintReadOnly, Category = "Animal|Meta")
    FGameplayAttributeData IncomingFear;

    /** Incoming calm change (meta attribute for calculations) */
    UPROPERTY(BlueprintReadOnly, Category = "Animal|Meta")
    FGameplayAttributeData IncomingCalm;

    // ===== Attribute Accessors =====

    static FGameplayAttribute GetFearAttribute();
    static FGameplayAttribute GetMaxFearAttribute();
    static FGameplayAttribute GetFearDecayRateAttribute();
    static FGameplayAttribute GetCalmLevelAttribute();
    static FGameplayAttribute GetLureSusceptibilityAttribute();
    static FGameplayAttribute GetHerdAffinityAttribute();
    static FGameplayAttribute GetHerdRadiusAttribute();
    static FGameplayAttribute GetSpeedModifierAttribute();
    static FGameplayAttribute GetIncomingFearAttribute();
    static FGameplayAttribute GetIncomingCalmAttribute();

    // Getters
    float GetFear() const { return Fear.GetCurrentValue(); }
    float GetMaxFear() const { return MaxFear.GetCurrentValue(); }
    float GetFearDecayRate() const { return FearDecayRate.GetCurrentValue(); }
    float GetCalmLevel() const { return CalmLevel.GetCurrentValue(); }
    float GetLureSusceptibility() const { return LureSusceptibility.GetCurrentValue(); }
    float GetHerdAffinity() const { return HerdAffinity.GetCurrentValue(); }
    float GetHerdRadius() const { return HerdRadius.GetCurrentValue(); }
    float GetSpeedModifier() const { return SpeedModifier.GetCurrentValue(); }
    float GetIncomingFear() const { return IncomingFear.GetCurrentValue(); }
    float GetIncomingCalm() const { return IncomingCalm.GetCurrentValue(); }

    // Setters
    void SetFear(float NewValue);
    void SetMaxFear(float NewValue);
    void SetFearDecayRate(float NewValue);
    void SetCalmLevel(float NewValue);
    void SetLureSusceptibility(float NewValue);
    void SetHerdAffinity(float NewValue);
    void SetHerdRadius(float NewValue);
    void SetSpeedModifier(float NewValue);
    void SetIncomingFear(float NewValue);
    void SetIncomingCalm(float NewValue);

    // ===== Replication =====

    UFUNCTION()
    void OnRep_Fear(const FGameplayAttributeData &OldValue);

    UFUNCTION()
    void OnRep_MaxFear(const FGameplayAttributeData &OldValue);

    UFUNCTION()
    void OnRep_FearDecayRate(const FGameplayAttributeData &OldValue);

    UFUNCTION()
    void OnRep_CalmLevel(const FGameplayAttributeData &OldValue);

    UFUNCTION()
    void OnRep_LureSusceptibility(const FGameplayAttributeData &OldValue);

    UFUNCTION()
    void OnRep_HerdAffinity(const FGameplayAttributeData &OldValue);

    UFUNCTION()
    void OnRep_HerdRadius(const FGameplayAttributeData &OldValue);

    UFUNCTION()
    void OnRep_SpeedModifier(const FGameplayAttributeData &OldValue);

    // ===== Overrides =====

    virtual void PreAttributeChange(const FGameplayAttribute &Attribute, float &NewValue) override;
    virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData &Data) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

    // ===== Helper Functions =====

    /** Get fear as a normalized percentage (0-1) */
    UFUNCTION(BlueprintCallable, Category = "Animal")
    float GetFearPercent() const;

    /** Check if animal is in panic state */
    UFUNCTION(BlueprintCallable, Category = "Animal")
    bool IsPanicked() const;

    /** Panic threshold as percentage of MaxFear */
    UPROPERTY(EditDefaultsOnly, Category = "Animal|Fear")
    float PanicThreshold = 0.7f;
};
