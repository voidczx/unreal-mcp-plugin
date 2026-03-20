// Copyright Epic Games, Inc. All Rights Reserved.
// From Penguin Assistant Start
#include "MCPEditorService.h"
#include "Editor.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "AssetRegistryModule.h"
#include "Editor/EditorEngine.h"
#include "Subsystems/AssetEditorSubsystem.h"

class UBlueprint;

FMCPEditorService::FMCPEditorService()
{
}

bool FMCPEditorService::OpenAsset(const FString& AssetPath)
{
	if (!GEditor)
	{
		return false;
	}

	UObject* Asset = LoadObject<UObject>(nullptr, *AssetPath);
	if (Asset)
	{
		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Asset);
		return true;
	}
	return false;
}

bool FMCPEditorService::SaveAsset(const FString& AssetPath)
{
	if (!GEditor)
	{
		return false;
	}

	UObject* Asset = LoadObject<UObject>(nullptr, *AssetPath);
	if (Asset && Asset->IsAsset())
	{
		return Asset->MarkPackageDirty();
	}
	return false;
}

bool FMCPEditorService::SaveAllAssets()
{
	if (!GEditor)
	{
		return false;
	}

	return false;
}

bool FMCPEditorService::CompileBlueprint(const FString& BlueprintPath)
{

	return false;
}

bool FMCPEditorService::ReimportAsset(const FString& AssetPath)
{

	return false;
}

bool FMCPEditorService::LaunchGame()
{
	if (!GEditor)
	{
		return false;
	}

	// Play in editor
	GEditor->PlayInEditor(GEditor->GetEditorWorldContext().World(), false);
	return true;
}

bool FMCPEditorService::StopGame()
{
	if (!GEditor)
	{
		return false;
	}

	return false;
}

bool FMCPEditorService::BuildProject()
{
	if (!GEditor)
	{
		return false;
	}

	// This would typically trigger a build through the editor
	// For now, we return false as full build requires more complex setup
	return false;
}
// From Penguin Assistant End
