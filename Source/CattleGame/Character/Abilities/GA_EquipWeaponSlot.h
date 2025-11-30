#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_EquipWeaponSlot.generated.h"

/**
 * Gameplay Ability for equipping a specific weapon slot (Number Keys 1-6).
 * Each instance of this ability is configured to equip a specific slot index.
 *
 * Usage:
 * - Create 6 instances in the Data Asset
 * - Set SlotIndexToEquip for each (0-5)
 * - Bind to number key input actions (1-6)
 */
UCLASS()
class CATTLEGAME_API UGA_EquipWeaponSlot : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_EquipWeaponSlot();

	/** The weapon slot index to equip when this ability activates (0-5) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon Slot")
	int32 SlotIndexToEquip;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};
