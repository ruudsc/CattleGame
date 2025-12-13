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

	// ===== Animal State Tags =====
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Animal_Idle, "State.Animal.Idle", "Animal is idle/standing");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Animal_Grazing, "State.Animal.Grazing", "Animal is grazing (eating, slow movement)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Animal_Walking, "State.Animal.Walking", "Animal is walking normally");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Animal_Panicked, "State.Animal.Panicked", "Animal is in panic state (high fear)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Animal_Fleeing, "State.Animal.Fleeing", "Animal is actively fleeing from threat");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Animal_Lured, "State.Animal.Lured", "Animal is being lured (trumpet, etc.)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Animal_Lassoed, "State.Animal.Lassoed", "Animal is caught by lasso");

	// ===== Animal Attribute Tags =====
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_Animal_Fear, "Attribute.Animal.Fear", "Animal fear level attribute");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_Animal_CalmLevel, "Attribute.Animal.CalmLevel", "Animal calm level attribute");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_Animal_HerdAffinity, "Attribute.Animal.HerdAffinity", "How strongly animal follows herd");

	// ===== Animal Effect Tags =====
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_Animal_Fear, "Effect.Animal.Fear", "Effect that adds fear to animal");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_Animal_Calm, "Effect.Animal.Calm", "Effect that calms animal (reduces fear)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_Animal_AreaGraze, "Effect.Animal.Area.Graze", "Effect from graze area");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_Animal_AreaPanic, "Effect.Animal.Area.Panic", "Effect from panic area");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_Animal_AreaAvoid, "Effect.Animal.Area.Avoid", "Effect from avoid area");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_Animal_FlowGuide, "Effect.Animal.FlowGuide", "Effect from flow guide");

	// Attribute Tags
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_Health, "Attribute.Health", "Tag for health attribute");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attribute_MovementSpeed, "Attribute.MovementSpeed", "Tag for movement speed attribute");

	// Effect Tags
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_Damage, "Effect.Damage", "Tag for damage effects");

	// GameplayCue Tags
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Revolver_Fire, "GameplayCue.Revolver.Fire", "GameplayCue for revolver fire VFX/Audio");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Revolver_Fire_Impact, "GameplayCue.Revolver.Fire.Impact", "GameplayCue for revolver bullet impact VFX/Audio");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Revolver_Reload, "GameplayCue.Revolver.Reload", "GameplayCue for revolver reload VFX/Audio");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_HitReaction, "GameplayCue.HitReaction", "GameplayCue for hit reaction VFX/Audio on damaged pawns");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Trumpet_Lure, "GameplayCue.Trumpet.Lure", "GameplayCue for trumpet lure VFX/Audio");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Trumpet_Scare, "GameplayCue.Trumpet.Scare", "GameplayCue for trumpet scare VFX/Audio");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Lasso_Throw, "GameplayCue.Lasso.Throw", "GameplayCue for lasso throw VFX/Audio");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Dynamite_Throw, "GameplayCue.Dynamite.Throw", "GameplayCue for dynamite throw VFX/Audio");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Dynamite_Explode, "GameplayCue.Dynamite.Explode", "GameplayCue for dynamite explosion VFX/Audio");

	// ===== Animal GameplayCue Tags =====
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Animal_Panic, "GameplayCue.Animal.Panic", "GameplayCue for animal panic VFX/Audio");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Animal_Calm, "GameplayCue.Animal.Calm", "GameplayCue for animal calming VFX/Audio");
}
