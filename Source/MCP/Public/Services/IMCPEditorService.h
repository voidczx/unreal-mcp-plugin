// Copyright Epic Games, Inc. All Rights Reserved.
// From Penguin Assistant Start
#pragma once

#include "CoreMinimal.h"
#include "IMCPService.h"

/**
 * Editor Service Interface
 * Provides editor operations accessible via MCP protocol
 */
class IMCPEditorService : public IMCPServiceBase
{
public:
	// Editor operations
	virtual bool OpenAsset(const FString& AssetPath) = 0;
	virtual bool SaveAsset(const FString& AssetPath) = 0;
	virtual bool SaveAllAssets() = 0;
	virtual bool CompileBlueprint(const FString& BlueprintPath) = 0;
	virtual bool ReimportAsset(const FString& AssetPath) = 0;

	// Game preview
	virtual bool LaunchGame() = 0;
	virtual bool StopGame() = 0;

	// Project operations
	virtual bool BuildProject() = 0;
};
// From Penguin Assistant End