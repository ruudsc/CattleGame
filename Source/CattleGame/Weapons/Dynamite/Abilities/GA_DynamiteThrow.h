#pragma once

#include "CattleGame/AbilitySystem/Abilities/GA_Weapon.h"
#include "GA_DynamiteThrow.generated.h"

class ADynamite;

/**
 * UGA_DynamiteThrow
 *
 * Gameplay ability for throwing dynamite.
 * Spawns a dynamite projectile with arc trajectory.
 */
UCLASS(Blueprintable)
class CATTLEGAME_API UGA_DynamiteThrow : public UGA_Weapon
{
    GENERATED_BODY()

public:
    UGA_DynamiteThrow();

    /**
     * Called when ability is activated
     */
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData *TriggerEventData) override;

    /**
     * Can the ability be activated?
     * Checks if dynamite exists and has ammo
     */
    virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo, const FGameplayTagContainer *SourceTags = nullptr, const FGameplayTagContainer *TargetTags = nullptr, OUT FGameplayTagContainer *OptionalRelevantTags = nullptr) const override;

protected:
    /**
     * Get dynamite weapon (cast to ADynamite)
     */
    ADynamite *GetDynamiteWeapon() const;

    /**
     * Called when throw ability starts
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "Dynamite Ability")
    void OnThrowStarted(ADynamite *Dynamite);

    /**
     * Throw the dynamite
     */
    void ThrowDynamite();
};
