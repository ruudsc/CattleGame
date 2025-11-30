// Copyright CattleGame. All Rights Reserved.

#include "BlueprintSerializerEditor.h"
#include "BlueprintToJson.h"
#include "JsonToBlueprint.h"
#include "BlueprintMergeHelper.h"
#include "BlueprintSchemaGenerator.h"
#include "BlueprintJsonValidator.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "ToolMenus.h"
#include "Engine/Blueprint.h"
#include "Misc/FileHelper.h"
#include "DesktopPlatformModule.h"
#include "EditorDirectories.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/FileManager.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"

#define LOCTEXT_NAMESPACE "FBlueprintSerializerEditorModule"

void FBlueprintSerializerEditorModule::StartupModule()
{
	// Register menu extensions after ToolMenus is ready
	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FBlueprintSerializerEditorModule::RegisterMenuExtensions));

	// Register console commands
	RegisterConsoleCommands();
}

void FBlueprintSerializerEditorModule::ShutdownModule()
{
	UnregisterMenuExtensions();
	UnregisterConsoleCommands();
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
}

void FBlueprintSerializerEditorModule::RegisterConsoleCommands()
{
	// BlueprintSerializer.GenerateNodeCatalog - Generate a catalog of all K2Node types
	ConsoleCommands.Add(IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("BlueprintSerializer.GenerateNodeCatalog"),
		TEXT("Generate a JSON catalog of all Blueprint K2Node types"),
		FConsoleCommandDelegate::CreateLambda([]()
											  {
			FString CatalogPath = FPaths::ProjectDir() / TEXT("Saved") / TEXT("BlueprintSerializer") / TEXT("node_catalog.json");
			
			// Ensure directory exists
			IFileManager::Get().MakeDirectory(*FPaths::GetPath(CatalogPath), true);
			
			// Generate catalog
			TArray<UClass*> NodeClasses = FBlueprintSchemaGenerator::GetAllK2NodeClasses();
			
			TSharedRef<FJsonObject> CatalogJson = MakeShared<FJsonObject>();
			CatalogJson->SetStringField(TEXT("GeneratedAt"), FDateTime::Now().ToString());
			CatalogJson->SetNumberField(TEXT("NodeTypeCount"), NodeClasses.Num());
			
			TArray<TSharedPtr<FJsonValue>> NodeTypesArray;
			for (UClass* NodeClass : NodeClasses)
			{
				TSharedRef<FJsonObject> NodeTypeJson = MakeShared<FJsonObject>();
				NodeTypeJson->SetStringField(TEXT("ClassName"), NodeClass->GetName());
				NodeTypeJson->SetStringField(TEXT("ClassPath"), NodeClass->GetPathName());
				NodeTypeJson->SetStringField(TEXT("Category"), FBlueprintSchemaGenerator::GetNodeCategory(NodeClass));
				
				// Get parent class
				if (UClass* ParentClass = NodeClass->GetSuperClass())
				{
					NodeTypeJson->SetStringField(TEXT("ParentClass"), ParentClass->GetName());
				}
				
				// Check if it's latent
				bool bIsLatent = FBlueprintSchemaGenerator::IsNodeLatent(NodeClass);
				NodeTypeJson->SetBoolField(TEXT("bIsLatent"), bIsLatent);
				
				NodeTypesArray.Add(MakeShared<FJsonValueObject>(NodeTypeJson));
			}
			
			CatalogJson->SetArrayField(TEXT("NodeTypes"), NodeTypesArray);
			
			FString OutputString;
			TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
			FJsonSerializer::Serialize(CatalogJson, Writer);
			
			if (FFileHelper::SaveStringToFile(OutputString, *CatalogPath))
			{
				UE_LOG(LogTemp, Log, TEXT("BlueprintSerializer: Generated node catalog with %d node types to %s"), NodeClasses.Num(), *CatalogPath);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("BlueprintSerializer: Failed to save node catalog to %s"), *CatalogPath);
			} })));

	// BlueprintSerializer.GenerateMasterSchema - Generate a master schema for all Blueprint features
	ConsoleCommands.Add(IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("BlueprintSerializer.GenerateMasterSchema"),
		TEXT("Generate a comprehensive JSON schema for all Blueprint features"),
		FConsoleCommandDelegate::CreateLambda([]()
											  {
			FString SchemaPath = FPaths::ProjectDir() / TEXT("Saved") / TEXT("BlueprintSerializer") / TEXT("blueprint_master_schema.json");
			
			// Ensure directory exists
			IFileManager::Get().MakeDirectory(*FPaths::GetPath(SchemaPath), true);
			
			// Generate master schema
			FBlueprintSerializerSchema Schema = FBlueprintSchemaGenerator::GenerateFullSchema();
			FString SchemaJson;
			if (FBlueprintSchemaGenerator::ExportSchemaToString(Schema, SchemaJson))
			{
				if (FFileHelper::SaveStringToFile(SchemaJson, *SchemaPath))
				{
					UE_LOG(LogTemp, Log, TEXT("BlueprintSerializer: Generated master schema with %d node types to %s"), Schema.NodeSchemas.Num(), *SchemaPath);
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("BlueprintSerializer: Failed to save master schema to %s"), *SchemaPath);
				}
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("BlueprintSerializer: Failed to export schema to string"));
			} })));

	// BlueprintSerializer.ValidateFile - Validate a Blueprint JSON file
	ConsoleCommands.Add(IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("BlueprintSerializer.ValidateFile"),
		TEXT("Validate a Blueprint JSON file. Usage: BlueprintSerializer.ValidateFile <path>"),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString> &Args)
													  {
			if (Args.Num() == 0)
			{
				UE_LOG(LogTemp, Warning, TEXT("Usage: BlueprintSerializer.ValidateFile <path>"));
				return;
			}
			
			FString FilePath = Args[0];
			if (!FPaths::FileExists(FilePath))
			{
				// Try relative to project
				FilePath = FPaths::ProjectDir() / Args[0];
			}
			
			if (!FPaths::FileExists(FilePath))
			{
				UE_LOG(LogTemp, Error, TEXT("BlueprintSerializer: File not found: %s"), *Args[0]);
				return;
			}
			
			FBlueprintJsonValidationResult Result = FBlueprintJsonValidator::ValidateJsonFile(FilePath);
			
			UE_LOG(LogTemp, Log, TEXT("BlueprintSerializer: Validation Result for %s"), *FilePath);
			UE_LOG(LogTemp, Log, TEXT("  Valid: %s"), Result.bIsValid ? TEXT("Yes") : TEXT("No"));
			UE_LOG(LogTemp, Log, TEXT("  Errors: %s, Warnings: %s"), Result.HasErrors() ? TEXT("Yes") : TEXT("No"), Result.HasWarnings() ? TEXT("Yes") : TEXT("No"));
			
			for (const FBlueprintJsonValidationIssue& Issue : Result.Issues)
			{
				FString Severity = Issue.Severity == FBlueprintJsonValidationIssue::ESeverity::Error ? TEXT("ERROR") :
								   Issue.Severity == FBlueprintJsonValidationIssue::ESeverity::Warning ? TEXT("WARNING") : TEXT("INFO");
				if (!Issue.NodeGuid.IsEmpty())
				{
					UE_LOG(LogTemp, Log, TEXT("  [%s] Node %s: %s"), *Severity, *Issue.NodeGuid, *Issue.Message);
				}
				else
				{
					UE_LOG(LogTemp, Log, TEXT("  [%s] %s"), *Severity, *Issue.Message);
				}
			} })));
}

void FBlueprintSerializerEditorModule::UnregisterConsoleCommands()
{
	for (IConsoleObject *Command : ConsoleCommands)
	{
		IConsoleManager::Get().UnregisterConsoleObject(Command);
	}
	ConsoleCommands.Empty();
}

void FBlueprintSerializerEditorModule::RegisterMenuExtensions()
{
	// Extend the Content Browser asset context menu
	UToolMenu *Menu = UToolMenus::Get()->ExtendMenu("ContentBrowser.AssetContextMenu.Blueprint");
	if (Menu)
	{
		FToolMenuSection &Section = Menu->AddSection("BlueprintSerializer", LOCTEXT("BlueprintSerializerSection", "Blueprint Serializer"));

		Section.AddMenuEntry(
			"ExportBlueprintToJson",
			LOCTEXT("ExportBlueprintToJson", "Export to JSON"),
			LOCTEXT("ExportBlueprintToJsonTooltip", "Export this Blueprint to an AI-readable JSON file"),
			FSlateIcon(),
			FToolMenuExecuteAction::CreateLambda([](const FToolMenuContext &Context)
												 {
				// Get selected assets from Content Browser
				FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
				TArray<FAssetData> SelectedAssets;
				ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);

				for (const FAssetData& AssetData : SelectedAssets)
				{
					if (UBlueprint* Blueprint = Cast<UBlueprint>(AssetData.GetAsset()))
					{
						// Open save dialog
						IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
						if (DesktopPlatform)
						{
							TArray<FString> SaveFilenames;
							const FString DefaultPath = FEditorDirectories::Get().GetLastDirectory(ELastDirectory::GENERIC_EXPORT);
							const FString DefaultFilename = Blueprint->GetName() + TEXT(".json");

							bool bSaved = DesktopPlatform->SaveFileDialog(
								FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
								TEXT("Export Blueprint to JSON"),
								DefaultPath,
								DefaultFilename,
								TEXT("JSON Files (*.json)|*.json"),
								0,
								SaveFilenames
							);

							if (bSaved && SaveFilenames.Num() > 0)
							{
								FString JsonString;
								if (FBlueprintToJson::SerializeBlueprintToString(Blueprint, JsonString, true))
								{
									if (FFileHelper::SaveStringToFile(JsonString, *SaveFilenames[0]))
									{
										FEditorDirectories::Get().SetLastDirectory(ELastDirectory::GENERIC_EXPORT, FPaths::GetPath(SaveFilenames[0]));
										
										FNotificationInfo Info(FText::Format(LOCTEXT("ExportSuccess", "Exported {0} to JSON"), FText::FromString(Blueprint->GetName())));
										Info.ExpireDuration = 3.0f;
										FSlateNotificationManager::Get().AddNotification(Info);
									}
								}
								else
								{
									FNotificationInfo Info(LOCTEXT("ExportFailed", "Failed to export Blueprint to JSON"));
									Info.ExpireDuration = 3.0f;
									FSlateNotificationManager::Get().AddNotification(Info);
								}
							}
						}
					}
				} }));

		Section.AddMenuEntry(
			"ImportBlueprintFromJson",
			LOCTEXT("ImportBlueprintFromJson", "Import from JSON..."),
			LOCTEXT("ImportBlueprintFromJsonTooltip", "Import a Blueprint from a JSON file and merge into this Blueprint"),
			FSlateIcon(),
			FToolMenuExecuteAction::CreateLambda([](const FToolMenuContext &Context)
												 {
				// Get selected assets from Content Browser
				FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
				TArray<FAssetData> SelectedAssets;
				ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);

				if (SelectedAssets.Num() == 0)
				{
					FNotificationInfo Info(LOCTEXT("NoAssetSelected", "Please select a Blueprint to merge into"));
					Info.ExpireDuration = 3.0f;
					FSlateNotificationManager::Get().AddNotification(Info);
					return;
				}

				UBlueprint* TargetBlueprint = Cast<UBlueprint>(SelectedAssets[0].GetAsset());
				if (!TargetBlueprint)
				{
					FNotificationInfo Info(LOCTEXT("NotABlueprint", "Selected asset is not a Blueprint"));
					Info.ExpireDuration = 3.0f;
					FSlateNotificationManager::Get().AddNotification(Info);
					return;
				}

				// Open file dialog
				IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
				if (DesktopPlatform)
				{
					TArray<FString> OpenFilenames;
					const FString DefaultPath = FEditorDirectories::Get().GetLastDirectory(ELastDirectory::GENERIC_IMPORT);

					bool bOpened = DesktopPlatform->OpenFileDialog(
						FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
						TEXT("Import Blueprint from JSON"),
						DefaultPath,
						TEXT(""),
						TEXT("JSON Files (*.json)|*.json"),
						0,
						OpenFilenames
					);

					if (bOpened && OpenFilenames.Num() > 0)
					{
						FEditorDirectories::Get().SetLastDirectory(ELastDirectory::GENERIC_IMPORT, FPaths::GetPath(OpenFilenames[0]));

						// Merge the JSON into the Blueprint
						if (FJsonToBlueprint::MergeJsonFileIntoBlueprint(TargetBlueprint, OpenFilenames[0]))
						{
							FNotificationInfo Info(FText::Format(LOCTEXT("MergeSuccess", "Merged JSON into {0}"), FText::FromString(TargetBlueprint->GetName())));
							Info.ExpireDuration = 3.0f;
							FSlateNotificationManager::Get().AddNotification(Info);

							// Optionally open diff view to show what was added
							// FBlueprintMergeHelper::OpenMergeEditor(TargetBlueprint, TargetBlueprint);
						}
						else
						{
							FNotificationInfo Info(LOCTEXT("MergeFailed", "Failed to merge JSON into Blueprint"));
							Info.ExpireDuration = 3.0f;
							FSlateNotificationManager::Get().AddNotification(Info);
						}
					}
				} }));

		// Add Schema Generation menu entry
		Section.AddMenuEntry(
			"GenerateBlueprintSchema",
			LOCTEXT("GenerateBlueprintSchema", "Generate Schema"),
			LOCTEXT("GenerateBlueprintSchemaTooltip", "Generate a JSON schema file for this Blueprint type"),
			FSlateIcon(),
			FToolMenuExecuteAction::CreateLambda([](const FToolMenuContext &Context)
												 {
				// Get selected assets from Content Browser
				FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
				TArray<FAssetData> SelectedAssets;
				ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);

				for (const FAssetData& AssetData : SelectedAssets)
				{
					if (UBlueprint* Blueprint = Cast<UBlueprint>(AssetData.GetAsset()))
					{
						// Open save dialog
						IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
						if (DesktopPlatform)
						{
							TArray<FString> SaveFilenames;
							const FString DefaultPath = FEditorDirectories::Get().GetLastDirectory(ELastDirectory::GENERIC_EXPORT);
							const FString DefaultFilename = Blueprint->GetName() + TEXT("_schema.json");

							bool bSaved = DesktopPlatform->SaveFileDialog(
								FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
								TEXT("Save Blueprint Schema"),
								DefaultPath,
								DefaultFilename,
								TEXT("JSON Files (*.json)|*.json"),
								0,
								SaveFilenames
							);

							if (bSaved && SaveFilenames.Num() > 0)
							{
								// Generate full schema (node types used by this Blueprint could be filtered later)
								FBlueprintSerializerSchema Schema = FBlueprintSchemaGenerator::GenerateFullSchema();
								FString SchemaJson;
								if (FBlueprintSchemaGenerator::ExportSchemaToString(Schema, SchemaJson))
								{
									if (FFileHelper::SaveStringToFile(SchemaJson, *SaveFilenames[0]))
									{
										FEditorDirectories::Get().SetLastDirectory(ELastDirectory::GENERIC_EXPORT, FPaths::GetPath(SaveFilenames[0]));
									
										FNotificationInfo Info(FText::Format(LOCTEXT("SchemaGenSuccess", "Generated schema with {0} node types"), FText::AsNumber(Schema.NodeSchemas.Num())));
										Info.ExpireDuration = 3.0f;
										FSlateNotificationManager::Get().AddNotification(Info);
									}
									else
									{
										FNotificationInfo Info(LOCTEXT("SchemaGenFailed", "Failed to save schema file"));
										Info.ExpireDuration = 3.0f;
										FSlateNotificationManager::Get().AddNotification(Info);
									}
								}
								else
								{
									FNotificationInfo Info(LOCTEXT("SchemaGenFailed", "Failed to save schema file"));
									Info.ExpireDuration = 3.0f;
									FSlateNotificationManager::Get().AddNotification(Info);
								}
							}
						}
					}
				} }));

		// Add Validate JSON menu entry
		Section.AddMenuEntry(
			"ValidateBlueprintJson",
			LOCTEXT("ValidateBlueprintJson", "Validate JSON File..."),
			LOCTEXT("ValidateBlueprintJsonTooltip", "Validate a Blueprint JSON file against the schema"),
			FSlateIcon(),
			FToolMenuExecuteAction::CreateLambda([](const FToolMenuContext &Context)
												 {
				// Open file dialog to select JSON file
				IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
				if (DesktopPlatform)
				{
					TArray<FString> OpenFilenames;
					const FString DefaultPath = FEditorDirectories::Get().GetLastDirectory(ELastDirectory::GENERIC_IMPORT);

					bool bOpened = DesktopPlatform->OpenFileDialog(
						FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
						TEXT("Select Blueprint JSON to Validate"),
						DefaultPath,
						TEXT(""),
						TEXT("JSON Files (*.json)|*.json"),
						0,
						OpenFilenames
					);

					if (bOpened && OpenFilenames.Num() > 0)
					{
						FEditorDirectories::Get().SetLastDirectory(ELastDirectory::GENERIC_IMPORT, FPaths::GetPath(OpenFilenames[0]));

						FBlueprintJsonValidationResult Result = FBlueprintJsonValidator::ValidateJsonFile(OpenFilenames[0]);
						
						if (Result.bIsValid)
						{
							FString Message = TEXT("JSON is valid!");
							
							if (Result.HasWarnings())
							{
								Message += TEXT("\n\nWarnings:");
								for (const FBlueprintJsonValidationIssue& Issue : Result.Issues)
								{
									if (Issue.Severity == FBlueprintJsonValidationIssue::ESeverity::Warning)
									{
										Message += TEXT("\n- ") + Issue.Message;
									}
								}
							}
							
							FNotificationInfo Info(FText::FromString(Message));
							Info.ExpireDuration = 5.0f;
							FSlateNotificationManager::Get().AddNotification(Info);
						}
						else
						{
							FString ErrorMessage = TEXT("Validation failed:\n");
							for (const FBlueprintJsonValidationIssue& Issue : Result.Issues)
							{
								if (Issue.Severity == FBlueprintJsonValidationIssue::ESeverity::Error)
								{
									ErrorMessage += TEXT("\n- ") + Issue.Message;
								}
							}
							
							FNotificationInfo Info(FText::FromString(ErrorMessage));
							Info.ExpireDuration = 5.0f;
							FSlateNotificationManager::Get().AddNotification(Info);
						}
					}
				} }));
	}
}

void FBlueprintSerializerEditorModule::UnregisterMenuExtensions()
{
	if (UToolMenus *ToolMenus = UToolMenus::TryGet())
	{
		ToolMenus->UnregisterOwnerByName("BlueprintSerializer");
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FBlueprintSerializerEditorModule, BlueprintSerializerEditor)
