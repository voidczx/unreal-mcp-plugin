// Copyright Epic Games, Inc. All Rights Reserved.
// From Penguin Assistant Start
#pragma once

#include "CoreMinimal.h"
#include "JsonObject.h"
#include "Services/IMCPAssetService.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"

/**
 * Asset Service Implementation
 * Provides asset operations using IAssetRegistry and IAssetTools
 */
class FMCPAssetService : public IMCPAssetService
{
public:
	FMCPAssetService();
	virtual ~FMCPAssetService() override;

	// IMCPAssetService interface
	virtual bool ListAssets(const FString& Path, bool bRecursive, const FString& Filter, TArray<FAssetData>& OutAssets) override;
	virtual bool ListAssetsByClass(const FString& ClassName, TArray<FAssetData>& OutAssets) override;
	virtual bool ListAssetsByTags(const TArray<FString>& Tags, TArray<FAssetData>& OutAssets) override;
	virtual bool GetAsset(const FString& AssetPath, FAssetData& OutAssetData) override;
	virtual bool GetAssetMetadata(const FString& AssetPath, TSharedPtr<FJsonObject>& OutMetadata) override;
	virtual bool GetAssetContent(const FString& AssetPath, FString& OutContent) override;
	virtual bool CreateAsset(const FString& AssetName, const FString& AssetPath, const FString& AssetClass) override;
	virtual bool DuplicateAsset(const FString& SourcePath, const FString& DestPath) override;
	virtual bool DeleteAsset(const FString& AssetPath) override;
	virtual bool RenameAsset(const FString& OldPath, const FString& NewPath) override;
	virtual bool ImportAsset(const FString& SourceFile, const FString& DestPath) override;
	virtual bool ExportAsset(const FString& AssetPath, const FString& DestFile) override;
	virtual bool GetDependencies(const FString& AssetPath, TArray<FName>& OutDependencies) override;
	virtual bool GetReferencers(const FString& AssetPath, TArray<FName>& OutReferencers) override;

private:
	bool EnsureAssetRegistryLoaded();
	bool EnsureAssetToolsLoaded();
};
// From Penguin Assistant End