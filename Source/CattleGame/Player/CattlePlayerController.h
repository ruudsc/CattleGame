class ACattleCharacter;
// Copyright (c) CattleGame

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CattlePlayerController.generated.h"

class UInputMappingContext;

/**
 * Central PlayerController responsible for input mapping lifecycle.
 * Moves Enhanced Input mapping ownership from Character to the controller
 * for reliable client initialization across possession/restart/network PIE.
 */
UCLASS()
class CATTLEGAME_API ACattlePlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    ACattlePlayerController();

protected:
    // Mapping context assigned in BP (user will re-parent existing BP to this class)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputMappingContext> DefaultMappingContext;

    // Priority to use when adding the mapping context (higher wins)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    int32 MappingPriority = 0;

    // Apply mapping context if we're the local controller
    void ApplyDefaultInputContext(const TCHAR *ReasonTag);

    // Track what we last applied to avoid duplicates
    UPROPERTY(Transient)
    TObjectPtr<UInputMappingContext> LastAppliedContext;

    virtual void BeginPlay() override;
    virtual void OnPossess(APawn *InPawn) override;
    virtual void OnRep_PlayerState() override;
};
