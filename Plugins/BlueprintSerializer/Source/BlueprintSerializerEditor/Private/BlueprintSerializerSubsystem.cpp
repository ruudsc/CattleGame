// Copyright CattleGame. All Rights Reserved.

#include "BlueprintSerializerSubsystem.h"
#include "BlueprintToJson.h"
#include "JsonToBlueprint.h"
#include "BlueprintMergeHelper.h"
#include "BlueprintJsonFormat.h"
#include "Engine/Blueprint.h"

FString UBlueprintSerializerSubsystem::SerializeBlueprintToString(UBlueprint *Blueprint)
{
    if (!Blueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("SerializeBlueprintToString: Blueprint is null"));
        return FString();
    }

    FString JsonString;
    if (FBlueprintToJson::SerializeBlueprintToString(Blueprint, JsonString, true))
    {
        return JsonString;
    }

    return FString();
}

UBlueprint *UBlueprintSerializerSubsystem::DeserializeBlueprintFromString(const FString &JsonString, const FString &PackagePath, const FString &BlueprintName)
{
    if (JsonString.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("DeserializeBlueprintFromString: JSON string is empty"));
        return nullptr;
    }

    return FJsonToBlueprint::DeserializeBlueprintFromString(JsonString, PackagePath, BlueprintName);
}

bool UBlueprintSerializerSubsystem::MergeJsonStringIntoBlueprint(UBlueprint *TargetBlueprint, const FString &JsonString)
{
    if (!TargetBlueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("MergeJsonStringIntoBlueprint: Target Blueprint is null"));
        return false;
    }

    if (JsonString.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("MergeJsonStringIntoBlueprint: JSON string is empty"));
        return false;
    }

    FBlueprintJsonData JsonData;
    if (!FJsonToBlueprint::ParseJsonString(JsonString, JsonData))
    {
        UE_LOG(LogTemp, Error, TEXT("MergeJsonStringIntoBlueprint: Failed to parse JSON"));
        return false;
    }

    return FJsonToBlueprint::MergeJsonIntoBlueprint(TargetBlueprint, JsonData);
}

bool UBlueprintSerializerSubsystem::MergeJsonFileIntoBlueprint(UBlueprint *TargetBlueprint, const FString &FilePath)
{
    if (!TargetBlueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("MergeJsonFileIntoBlueprint: Target Blueprint is null"));
        return false;
    }

    return FJsonToBlueprint::MergeJsonFileIntoBlueprint(TargetBlueprint, FilePath);
}

bool UBlueprintSerializerSubsystem::OpenMergeEditor(UBlueprint *OriginalBlueprint, UBlueprint *ModifiedBlueprint)
{
    return FBlueprintMergeHelper::OpenMergeEditor(OriginalBlueprint, ModifiedBlueprint);
}

FString UBlueprintSerializerSubsystem::GetDiffSummary(UBlueprint *BlueprintA, UBlueprint *BlueprintB)
{
    return FBlueprintMergeHelper::GetDiffSummary(BlueprintA, BlueprintB);
}
