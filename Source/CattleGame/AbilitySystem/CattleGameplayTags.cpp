// Copyright Epic Games, Inc. All Rights Reserved.

#include "CattleGameplayTags.h"

namespace CattleGameplayTags
{
	// Ability Tags
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Weapon_Fire, "Ability.Weapon.Fire", "Tag for weapon fire ability");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Weapon_Reload, "Ability.Weapon.Reload", "Tag for weapon reload ability");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Weapon_Equip, "Ability.Weapon.Equip", "Tag for weapon equip ability");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Weapon_SecondaryFire, "Ability.Weapon.SecondaryFire", "Tag for weapon secondary fire ability");

	// Lasso Tags
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Lasso_Fire, "Ability.Lasso.Fire", "Tag for lasso fire ability");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Lasso_Release, "Ability.Lasso.Release", "Tag for lasso release ability");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Lasso_Active, "State.Lasso.Active", "Tag indicating lasso is currently active");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Lasso_Throwing, "State.Lasso.Throwing", "Tag indicating lasso is being thrown");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Lasso_Pulling, "State.Lasso.Pulling", "Tag indicating lasso is pulling/retracting");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Lasso_Retracting, "State.Lasso.Retracting", "Tag indicating lasso projectile is retracting back to player");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Lasso_Reloading, "State.Lasso.Reloading", "Tag indicating lasso is being reloaded");

	// Dynamite Tags
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Dynamite_Throw, "Ability.Dynamite.Throw", "Tag for dynamite throw ability");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dynamite_Reloading, "State.Dynamite.Reloading", "Tag indicating dynamite is being reloaded");

	// Trumpet Tags
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Trumpet_Lure, "Ability.Trumpet.Lure", "Tag for trumpet lure ability");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Trumpet_Scare, "Ability.Trumpet.Scare", "Tag for trumpet scare ability");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Trumpet_Playing, "State.Trumpet.Playing", "Tag indicating trumpet is currently playing");

	// State Tags
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Weapon_Firing, "State.Weapon.Firing", "Tag indicating character is currently firing");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Weapon_Reloading, "State.Weapon.Reloading", "Tag indicating character is currently reloading");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dead, "State.Dead", "Tag indicating character is dead");

	// Attribute Tags
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_Health, "Attribute.Health", "Tag for health attribute");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_MovementSpeed, "Attribute.MovementSpeed", "Tag for movement speed attribute");

	// Effect Tags
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_Damage, "Effect.Damage", "Tag for damage effects");
}
