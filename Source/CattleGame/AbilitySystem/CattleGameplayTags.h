// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "NativeGameplayTags.h"

namespace CattleGameplayTags
{
	// Ability Tags
	extern FNativeGameplayTag Ability_Weapon_Fire;
	extern FNativeGameplayTag Ability_Weapon_Reload;
	extern FNativeGameplayTag Ability_Weapon_Equip;
	extern FNativeGameplayTag Ability_Weapon_SecondaryFire;

	// Lasso Tags
	extern FNativeGameplayTag Ability_Lasso_Fire;
	extern FNativeGameplayTag Ability_Lasso_Release;
	extern FNativeGameplayTag State_Lasso_Active;
	extern FNativeGameplayTag State_Lasso_Throwing;
	extern FNativeGameplayTag State_Lasso_Pulling;
	extern FNativeGameplayTag State_Lasso_Retracting;
	extern FNativeGameplayTag State_Lasso_Reloading;

	// Dynamite Tags
	extern FNativeGameplayTag Ability_Dynamite_Throw;
	extern FNativeGameplayTag State_Dynamite_Reloading;

	// Trumpet Tags
	extern FNativeGameplayTag Ability_Trumpet_Lure;
	extern FNativeGameplayTag Ability_Trumpet_Scare;
	extern FNativeGameplayTag State_Trumpet_Playing;

	// State Tags
	extern FNativeGameplayTag State_Weapon_Firing;
	extern FNativeGameplayTag State_Weapon_Reloading;
	extern FNativeGameplayTag State_Dead;

	// ===== Animal State Tags =====
	extern FNativeGameplayTag State_Animal_Idle;
	extern FNativeGameplayTag State_Animal_Grazing;
	extern FNativeGameplayTag State_Animal_Walking;
	extern FNativeGameplayTag State_Animal_Panicked;
	extern FNativeGameplayTag State_Animal_Fleeing;
	extern FNativeGameplayTag State_Animal_Lured;
	extern FNativeGameplayTag State_Animal_Lassoed;

	// ===== Animal Attribute Tags =====
	extern FNativeGameplayTag Attribute_Animal_Fear;
	extern FNativeGameplayTag Attribute_Animal_CalmLevel;
	extern FNativeGameplayTag Attribute_Animal_HerdAffinity;

	// ===== Animal Effect Tags =====
	extern FNativeGameplayTag Effect_Animal_Fear;
	extern FNativeGameplayTag Effect_Animal_Calm;
	extern FNativeGameplayTag Effect_Animal_AreaGraze;
	extern FNativeGameplayTag Effect_Animal_AreaPanic;
	extern FNativeGameplayTag Effect_Animal_AreaAvoid;
	extern FNativeGameplayTag Effect_Animal_FlowGuide;

	// Attribute Tags
	extern FNativeGameplayTag Attribute_Health;
	extern FNativeGameplayTag Attribute_MovementSpeed;

	// Effect Tags
	extern FNativeGameplayTag Effect_Damage;

	// GameplayCue Tags
	extern FNativeGameplayTag GameplayCue_Revolver_Fire;
	extern FNativeGameplayTag GameplayCue_Revolver_Fire_Impact;
	extern FNativeGameplayTag GameplayCue_Revolver_Reload;
	extern FNativeGameplayTag GameplayCue_HitReaction;
	extern FNativeGameplayTag GameplayCue_Trumpet_Lure;
	extern FNativeGameplayTag GameplayCue_Trumpet_Scare;
	extern FNativeGameplayTag GameplayCue_Lasso_Throw;
	extern FNativeGameplayTag GameplayCue_Dynamite_Throw;
	extern FNativeGameplayTag GameplayCue_Dynamite_Explode;

	// ===== Animal GameplayCue Tags =====
	extern FNativeGameplayTag GameplayCue_Animal_Panic;
	extern FNativeGameplayTag GameplayCue_Animal_Calm;
}
