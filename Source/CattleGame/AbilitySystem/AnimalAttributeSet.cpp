// Copyright Epic Games, Inc. All Rights Reserved.

#include "AnimalAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"

UAnimalAttributeSet::UAnimalAttributeSet()
{
    // Default attribute values
    Fear.SetBaseValue(0.0f);
    Fear.SetCurrentValue(0.0f);

    MaxFear.SetBaseValue(100.0f);
    MaxFear.SetCurrentValue(100.0f);

    FearDecayRate.SetBaseValue(5.0f);
    FearDecayRate.SetCurrentValue(5.0f);

    CalmLevel.SetBaseValue(0.0f);
    CalmLevel.SetCurrentValue(0.0f);

    LureSusceptibility.SetBaseValue(1.0f);
    LureSusceptibility.SetCurrentValue(1.0f);

    HerdAffinity.SetBaseValue(0.5f);
    HerdAffinity.SetCurrentValue(0.5f);

    HerdRadius.SetBaseValue(1000.0f);
    HerdRadius.SetCurrentValue(1000.0f);

    SpeedModifier.SetBaseValue(1.0f);
    SpeedModifier.SetCurrentValue(1.0f);

    IncomingFear.SetBaseValue(0.0f);
    IncomingFear.SetCurrentValue(0.0f);

    IncomingCalm.SetBaseValue(0.0f);
    IncomingCalm.SetCurrentValue(0.0f);
}

// ===== Attribute Accessors =====

FGameplayAttribute UAnimalAttributeSet::GetFearAttribute()
{
    static FProperty *Property = FindFieldChecked<FProperty>(UAnimalAttributeSet::StaticClass(), GET_MEMBER_NAME_CHECKED(UAnimalAttributeSet, Fear));
    return FGameplayAttribute(Property);
}

FGameplayAttribute UAnimalAttributeSet::GetMaxFearAttribute()
{
    static FProperty *Property = FindFieldChecked<FProperty>(UAnimalAttributeSet::StaticClass(), GET_MEMBER_NAME_CHECKED(UAnimalAttributeSet, MaxFear));
    return FGameplayAttribute(Property);
}

FGameplayAttribute UAnimalAttributeSet::GetFearDecayRateAttribute()
{
    static FProperty *Property = FindFieldChecked<FProperty>(UAnimalAttributeSet::StaticClass(), GET_MEMBER_NAME_CHECKED(UAnimalAttributeSet, FearDecayRate));
    return FGameplayAttribute(Property);
}

FGameplayAttribute UAnimalAttributeSet::GetCalmLevelAttribute()
{
    static FProperty *Property = FindFieldChecked<FProperty>(UAnimalAttributeSet::StaticClass(), GET_MEMBER_NAME_CHECKED(UAnimalAttributeSet, CalmLevel));
    return FGameplayAttribute(Property);
}

FGameplayAttribute UAnimalAttributeSet::GetLureSusceptibilityAttribute()
{
    static FProperty *Property = FindFieldChecked<FProperty>(UAnimalAttributeSet::StaticClass(), GET_MEMBER_NAME_CHECKED(UAnimalAttributeSet, LureSusceptibility));
    return FGameplayAttribute(Property);
}

FGameplayAttribute UAnimalAttributeSet::GetHerdAffinityAttribute()
{
    static FProperty *Property = FindFieldChecked<FProperty>(UAnimalAttributeSet::StaticClass(), GET_MEMBER_NAME_CHECKED(UAnimalAttributeSet, HerdAffinity));
    return FGameplayAttribute(Property);
}

FGameplayAttribute UAnimalAttributeSet::GetHerdRadiusAttribute()
{
    static FProperty *Property = FindFieldChecked<FProperty>(UAnimalAttributeSet::StaticClass(), GET_MEMBER_NAME_CHECKED(UAnimalAttributeSet, HerdRadius));
    return FGameplayAttribute(Property);
}

FGameplayAttribute UAnimalAttributeSet::GetSpeedModifierAttribute()
{
    static FProperty *Property = FindFieldChecked<FProperty>(UAnimalAttributeSet::StaticClass(), GET_MEMBER_NAME_CHECKED(UAnimalAttributeSet, SpeedModifier));
    return FGameplayAttribute(Property);
}

FGameplayAttribute UAnimalAttributeSet::GetIncomingFearAttribute()
{
    static FProperty *Property = FindFieldChecked<FProperty>(UAnimalAttributeSet::StaticClass(), GET_MEMBER_NAME_CHECKED(UAnimalAttributeSet, IncomingFear));
    return FGameplayAttribute(Property);
}

FGameplayAttribute UAnimalAttributeSet::GetIncomingCalmAttribute()
{
    static FProperty *Property = FindFieldChecked<FProperty>(UAnimalAttributeSet::StaticClass(), GET_MEMBER_NAME_CHECKED(UAnimalAttributeSet, IncomingCalm));
    return FGameplayAttribute(Property);
}

// ===== Setters =====

void UAnimalAttributeSet::SetFear(float NewValue)
{
    Fear.SetBaseValue(NewValue);
    Fear.SetCurrentValue(NewValue);
}

void UAnimalAttributeSet::SetMaxFear(float NewValue)
{
    MaxFear.SetBaseValue(NewValue);
    MaxFear.SetCurrentValue(NewValue);
}

void UAnimalAttributeSet::SetFearDecayRate(float NewValue)
{
    FearDecayRate.SetBaseValue(NewValue);
    FearDecayRate.SetCurrentValue(NewValue);
}

void UAnimalAttributeSet::SetCalmLevel(float NewValue)
{
    CalmLevel.SetBaseValue(NewValue);
    CalmLevel.SetCurrentValue(NewValue);
}

void UAnimalAttributeSet::SetLureSusceptibility(float NewValue)
{
    LureSusceptibility.SetBaseValue(NewValue);
    LureSusceptibility.SetCurrentValue(NewValue);
}

void UAnimalAttributeSet::SetHerdAffinity(float NewValue)
{
    HerdAffinity.SetBaseValue(NewValue);
    HerdAffinity.SetCurrentValue(NewValue);
}

void UAnimalAttributeSet::SetHerdRadius(float NewValue)
{
    HerdRadius.SetBaseValue(NewValue);
    HerdRadius.SetCurrentValue(NewValue);
}

void UAnimalAttributeSet::SetSpeedModifier(float NewValue)
{
    SpeedModifier.SetBaseValue(NewValue);
    SpeedModifier.SetCurrentValue(NewValue);
}

// ===== Replication Callbacks =====

void UAnimalAttributeSet::OnRep_Fear(const FGameplayAttributeData &OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UAnimalAttributeSet, Fear, OldValue);
}

void UAnimalAttributeSet::OnRep_MaxFear(const FGameplayAttributeData &OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UAnimalAttributeSet, MaxFear, OldValue);
}

void UAnimalAttributeSet::OnRep_FearDecayRate(const FGameplayAttributeData &OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UAnimalAttributeSet, FearDecayRate, OldValue);
}

void UAnimalAttributeSet::OnRep_CalmLevel(const FGameplayAttributeData &OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UAnimalAttributeSet, CalmLevel, OldValue);
}

void UAnimalAttributeSet::OnRep_LureSusceptibility(const FGameplayAttributeData &OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UAnimalAttributeSet, LureSusceptibility, OldValue);
}

void UAnimalAttributeSet::OnRep_HerdAffinity(const FGameplayAttributeData &OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UAnimalAttributeSet, HerdAffinity, OldValue);
}

void UAnimalAttributeSet::OnRep_HerdRadius(const FGameplayAttributeData &OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UAnimalAttributeSet, HerdRadius, OldValue);
}

void UAnimalAttributeSet::OnRep_SpeedModifier(const FGameplayAttributeData &OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UAnimalAttributeSet, SpeedModifier, OldValue);
}

// ===== Attribute System Overrides =====

void UAnimalAttributeSet::PreAttributeChange(const FGameplayAttribute &Attribute, float &NewValue)
{
    Super::PreAttributeChange(Attribute, NewValue);

    // Clamp Fear to valid range
    if (Attribute == GetFearAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxFear());
    }
    // Clamp CalmLevel
    else if (Attribute == GetCalmLevelAttribute())
    {
        NewValue = FMath::Max(0.0f, NewValue);
    }
    // Clamp SpeedModifier
    else if (Attribute == GetSpeedModifierAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.1f, 3.0f);
    }
    // Clamp HerdAffinity
    else if (Attribute == GetHerdAffinityAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, 1.0f);
    }
}

void UAnimalAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData &Data)
{
    Super::PostGameplayEffectExecute(Data);

    // Process IncomingFear meta attribute
    if (Data.EvaluatedData.Attribute == GetIncomingFearAttribute())
    {
        const float LocalIncomingFear = GetIncomingFear();
        SetIncomingFear(0.0f);

        if (LocalIncomingFear > 0.0f)
        {
            const float NewFear = FMath::Clamp(GetFear() + LocalIncomingFear, 0.0f, GetMaxFear());
            SetFear(NewFear);
        }
    }
    // Process IncomingCalm meta attribute
    else if (Data.EvaluatedData.Attribute == GetIncomingCalmAttribute())
    {
        const float LocalIncomingCalm = GetIncomingCalm();
        SetIncomingCalm(0.0f);

        if (LocalIncomingCalm > 0.0f)
        {
            // Calm reduces fear and increases calm level
            const float NewFear = FMath::Max(0.0f, GetFear() - LocalIncomingCalm * GetLureSusceptibility());
            SetFear(NewFear);

            const float NewCalmLevel = GetCalmLevel() + LocalIncomingCalm * GetLureSusceptibility();
            SetCalmLevel(NewCalmLevel);
        }
    }
}

void UAnimalAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION_NOTIFY(UAnimalAttributeSet, Fear, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UAnimalAttributeSet, MaxFear, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UAnimalAttributeSet, FearDecayRate, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UAnimalAttributeSet, CalmLevel, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UAnimalAttributeSet, LureSusceptibility, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UAnimalAttributeSet, HerdAffinity, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UAnimalAttributeSet, HerdRadius, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UAnimalAttributeSet, SpeedModifier, COND_None, REPNOTIFY_Always);
}

// ===== Helper Functions =====

float UAnimalAttributeSet::GetFearPercent() const
{
    const float MaxFearValue = GetMaxFear();
    if (MaxFearValue > 0.0f)
    {
        return GetFear() / MaxFearValue;
    }
    return 0.0f;
}

bool UAnimalAttributeSet::IsPanicked() const
{
    return GetFearPercent() >= PanicThreshold;
}

void UAnimalAttributeSet::SetIncomingFear(float NewValue)
{
    IncomingFear.SetBaseValue(NewValue);
    IncomingFear.SetCurrentValue(NewValue);
}

void UAnimalAttributeSet::SetIncomingCalm(float NewValue)
{
    IncomingCalm.SetBaseValue(NewValue);
    IncomingCalm.SetCurrentValue(NewValue);
}
