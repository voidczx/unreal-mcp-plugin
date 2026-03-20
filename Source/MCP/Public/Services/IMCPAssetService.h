// Copyright Epic Games, Inc. All Rights Reserved.
// From Penguin Assistant Start
#pragma once

#include "CoreMinimal.h"
#include "IMCPService.h"
#include "AssetRegistry/AssetData.h"

/**
 * Asset Service Interface
 * Provides asset operations accessible via MCP protocol
 */
class IMCPAssetService : public IMCPServiceBase
{
public:
	// Asset listing
	virtual bool ListAssets(const FString& Path, bool bRecursive, const FString& Filter, TArray<FAssetData>& OutAssets) = 0;
	virtual bool ListAssetsByClass(const FString& ClassName, TArray<FAssetData>& OutAssets) = 0;
	virtual bool ListAssetsByTags(const TArray<FString>& Tags, TArray<FAssetData>& OutAssets) = 0;

	// Asset querying
	virtual bool GetAsset(const FString& AssetPath, FAssetData& OutAssetData) = 0;
	virtual bool GetAssetMetadata(const FString& AssetPath, TSharedPtr<FJsonObject>& OutMetadata) = 0;
	virtual bool GetAssetContent(const FString& AssetPath, FString& OutContent) = 0;

	// Asset creation/modification
	virtual bool CreateAsset(const FString& AssetName, const FString& AssetPath, const FString& AssetClass) = 0;
	virtual bool DuplicateAsset(const FString& SourcePath, const FString& DestPath) = 0;
	virtual bool DeleteAsset(const FString& AssetPath) = 0;
	virtual bool RenameAsset(const FString& OldPath, const FString& NewPath) = 0;

	// Asset import/export
	virtual bool ImportAsset(const FString& SourceFile, const FString& DestPath) = 0;
	virtual bool ExportAsset(const FString& AssetPath, const FString& DestFile) = 0;

	// Dependency analysis
	virtual bool GetDependencies(const FString& AssetPath, TArray<FName>& OutDependencies) = 0;
	virtual bool GetReferencers(const FString& AssetPath, TArray<FName>& OutReferencers) = 0;
};
// From Penguin Assistant End