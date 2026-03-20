// Copyright Epic Games, Inc. All Rights Reserved.
// From Penguin Assistant Start
#pragma once

#include "CoreMinimal.h"
#include "Services/IMCPEditorService.h"

/**
 * Editor Service Implementation
 * Provides editor operations
 */
class FMCPEditorService : public IMCPEditorService
{
public:
	FMCPEditorService();
	virtual ~FMCPEditorService() = default;

	// IMCPEditorService interface
	virtual bool OpenAsset(const FString& AssetPath) override;
	virtual bool SaveAsset(const FString& AssetPath) override;
	virtual bool SaveAllAssets() override;
	virtual bool CompileBlueprint(const FString& BlueprintPath) override;
	virtual bool ReimportAsset(const FString& AssetPath) override;
	virtual bool LaunchGame() override;
	virtual bool StopGame() override;
	virtual bool BuildProject() override;
};
// From Penguin Assistant End