// Copyright Epic Games, Inc. All Rights Reserved.
// From Penguin Assistant Start
#include "Services/MCPAssetService.h"

#include "AssetExportTask.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "AssetViewUtils.h"
#include "Editor.h"
#include "JsonSerializer.h"
#include "UObject/UObjectGlobals.h"
#include "Factories/Factory.h"
#include "EditorFramework/AssetImportData.h"
#include "ObjectTools.h"
#include "PackageTools.h"
#include "Exporters/Exporter.h"

FMCPAssetService::FMCPAssetService()
{
}

bool FMCPAssetService::EnsureAssetRegistryLoaded()
{
	if (!FModuleManager::Get().IsModuleLoaded("AssetRegistry"))
	{
		FModuleManager::Get().LoadModule("AssetRegistry");
	}
	return FModuleManager::Get().IsModuleLoaded("AssetRegistry");
}

bool FMCPAssetService::EnsureAssetToolsLoaded()
{
	if (!FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		FModuleManager::Get().LoadModule("AssetTools");
	}
	return FModuleManager::Get().IsModuleLoaded("AssetTools");
}

bool FMCPAssetService::ListAssets(const FString& Path, bool bRecursive, const FString& Filter, TArray<FAssetData>& OutAssets)
{
	if (!EnsureAssetRegistryLoaded())
	{
		return false;
	}

	FAssetRegistryModule& RegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& Registry = RegistryModule.Get();

	// Ensure the asset registry is ready
	if (!Registry.IsLoadingAssets())
	{
		Registry.WaitForCompletion();
	}

	FARFilter AssetFilter;

	// Handle path - ensure it starts with /Game or similar
	FString ValidPath = Path;
	if (!ValidPath.StartsWith(TEXT("/")))
	{
		ValidPath = TEXT("/Game/") + ValidPath;
	}
	AssetFilter.PackagePaths.Add(FName(*ValidPath));
	AssetFilter.bRecursivePaths = bRecursive;

	if (!Filter.IsEmpty())
	{
		AssetFilter.ClassNames.Add(FName(*Filter));
	}

	return Registry.GetAssets(AssetFilter, OutAssets);
}

bool FMCPAssetService::ListAssetsByClass(const FString& ClassName, TArray<FAssetData>& OutAssets)
{
	if (!EnsureAssetRegistryLoaded())
	{
		return false;
	}

	FAssetRegistryModule& RegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& Registry = RegistryModule.Get();

	// Ensure the asset registry is ready
	if (!Registry.IsLoadingAssets())
	{
		Registry.WaitForCompletion();
	}

	FARFilter AssetFilter;
	AssetFilter.ClassNames.Add(FName(*ClassName));
	AssetFilter.bRecursiveClasses = true;

	return Registry.GetAssets(AssetFilter, OutAssets);
}

bool FMCPAssetService::ListAssetsByTags(const TArray<FString>& Tags, TArray<FAssetData>& OutAssets)
{
	if (!EnsureAssetRegistryLoaded())
	{
		return false;
	}

	FAssetRegistryModule& RegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& Registry = RegistryModule.Get();

	// Ensure the asset registry is ready
	if (!Registry.IsLoadingAssets())
	{
		Registry.WaitForCompletion();
	}

	FARFilter AssetFilter;
	for (const FString& Tag : Tags)
	{
		AssetFilter.TagsAndValues.Add(FName(*Tag));
	}

	return Registry.GetAssets(AssetFilter, OutAssets);
}

bool FMCPAssetService::GetAsset(const FString& AssetPath, FAssetData& OutAssetData)
{
	if (!EnsureAssetRegistryLoaded())
	{
		return false;
	}

	FAssetRegistryModule& RegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& Registry = RegistryModule.Get();

	// Ensure the asset registry is ready
	if (!Registry.IsLoadingAssets())
	{
		Registry.WaitForCompletion();
	}

	FString ValidPath = AssetPath;
	if (!ValidPath.StartsWith(TEXT("/")))
	{
		ValidPath = TEXT("/Game/") + ValidPath;
	}

	OutAssetData = Registry.GetAssetByObjectPath(FName(*ValidPath));

	return OutAssetData.IsValid();
}

bool FMCPAssetService::GetAssetMetadata(const FString& AssetPath, TSharedPtr<FJsonObject>& OutMetadata)
{
	if (!EnsureAssetRegistryLoaded())
	{
		return false;
	}

	FAssetRegistryModule& RegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& Registry = RegistryModule.Get();

	// Ensure the asset registry is ready
	if (!Registry.IsLoadingAssets())
	{
		Registry.WaitForCompletion();
	}

	FString ValidPath = AssetPath;
	if (!ValidPath.StartsWith(TEXT("/")))
	{
		ValidPath = TEXT("/Game/") + ValidPath;
	}

	FAssetData AssetData = Registry.GetAssetByObjectPath(FName(*ValidPath));

	if (!AssetData.IsValid())
	{
		return false;
	}

	OutMetadata = MakeShared<FJsonObject>();

	// Add basic asset info
	OutMetadata->SetStringField(TEXT("PackageName"), AssetData.PackageName.ToString());
	OutMetadata->SetStringField(TEXT("AssetName"), AssetData.AssetName.ToString());
	OutMetadata->SetStringField(TEXT("AssetClass"), AssetData.AssetClass.ToString());
	OutMetadata->SetStringField(TEXT("PackagePath"), AssetData.PackagePath.ToString());
	OutMetadata->SetStringField(TEXT("ObjectPath"), AssetData.ObjectPath.ToString());

	// Get tags from TagsAndValues
	const FAssetDataTagMapSharedView& TagsAndValues = AssetData.TagsAndValues;
	TArray<TSharedPtr<FJsonValue>> TagArray;
	for (const auto& TagPair : TagsAndValues)
	{
		TSharedPtr<FJsonObject> TagObj = MakeShared<FJsonObject>();
		TagObj->SetStringField(TEXT("Key"), TagPair.Key.ToString());
		TagObj->SetStringField(TEXT("Value"), TagPair.Value);
		TagArray.Add(MakeShared<FJsonValueObject>(TagObj));
	}
	OutMetadata->SetArrayField(TEXT("Tags"), TagArray);
	return true;
}

bool FMCPAssetService::GetAssetContent(const FString& AssetPath, FString& OutContent)
{
	if (!EnsureAssetRegistryLoaded())
	{
		return false;
	}

	FAssetRegistryModule& RegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& Registry = RegistryModule.Get();

	FString ValidPath = AssetPath;
	if (!ValidPath.StartsWith(TEXT("/")))
	{
		ValidPath = TEXT("/Game/") + ValidPath;
	}

	FAssetData AssetData = Registry.GetAssetByObjectPath(FName(*ValidPath));

	if (!AssetData.IsValid())
	{
		return false;
	}

	UObject* AssetObject = AssetData.GetAsset();
	if (!AssetObject)
	{
		return false;
	}

	// Get basic asset info
	TSharedPtr<FJsonObject> AssetJson = MakeShared<FJsonObject>();
	AssetJson->SetStringField(TEXT("ObjectPath"), AssetObject->GetPathName());
	AssetJson->SetStringField(TEXT("Class"), AssetObject->GetClass()->GetName());
	AssetJson->SetStringField(TEXT("Name"), AssetObject->GetName());

	// Serialize to JSON
	FString JsonStr;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonStr);
	FJsonSerializer::Serialize(AssetJson.ToSharedRef(), Writer);

	OutContent = JsonStr;
	return true;
}

bool FMCPAssetService::CreateAsset(const FString& AssetName, const FString& AssetPath, const FString& AssetClass)
{
	if (!EnsureAssetToolsLoaded())
	{
		return false;
	}

	FAssetToolsModule& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	IAssetTools& AssetToolsInterface = AssetTools.Get();

	// Validate asset name
	FString ValidAssetName = AssetName.IsEmpty() ? TEXT("NewAsset") : AssetName;
	FString ValidAssetPath = AssetPath.IsEmpty() ? TEXT("/Game") : AssetPath;
	FString ValidAssetClass = AssetClass.IsEmpty() ? TEXT("Object") : AssetClass;

	// Find the class
	UClass* Class = FindObject<UClass>(ANY_PACKAGE, *ValidAssetClass);
	if (!Class)
	{
		// Try to find a Blueprint class
		FString BlueprintClassName = FString::Printf(TEXT("Blueprint_%s"), *ValidAssetClass);
		Class = FindObject<UClass>(ANY_PACKAGE, *BlueprintClassName);
	}

	// If no class found, use UObject as default
	if (!Class)
	{
		Class = UObject::StaticClass();
	}

	// Create asset with factory
	UFactory* Factory = nullptr;

	// Try to find a factory for the class
	for (TObjectIterator<UFactory> It; It; ++It)
	{
		if (It->SupportedClass == Class)
		{
			Factory = *It;
			break;
		}
	}

	UObject* NewAsset = AssetToolsInterface.CreateAsset(ValidAssetName, ValidAssetPath, Class, Factory);

	return NewAsset != nullptr;
}

bool FMCPAssetService::DuplicateAsset(const FString& SourcePath, const FString& DestPath)
{
	if (!EnsureAssetToolsLoaded())
	{
		return false;
	}

	FAssetToolsModule& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	IAssetTools& AssetToolsInterface = AssetTools.Get();

	FString ValidSourcePath = SourcePath;
	FString ValidDestPath = DestPath;

	if (!ValidSourcePath.StartsWith(TEXT("/")))
	{
		ValidSourcePath = TEXT("/Game/") + ValidSourcePath;
	}

	if (!ValidDestPath.StartsWith(TEXT("/")))
	{
		ValidDestPath = TEXT("/Game/") + ValidDestPath;
	}

	// Parse destination path to get folder and name
	FString DestFolder, DestName;
	if (!ValidDestPath.Split(TEXT("/"), &DestFolder, &DestName, ESearchCase::CaseSensitive, ESearchDir::FromEnd))
	{
		DestFolder = ValidDestPath;
		DestName = TEXT("Duplicate");
	}

	// Ensure destination folder is valid
	if (DestFolder.IsEmpty())
	{
		DestFolder = TEXT("/Game");
	}

	// Load source asset
	UObject* SourceObject = LoadObject<UObject>(nullptr, *ValidSourcePath);
	if (!SourceObject)
	{
		return false;
	}

	// Duplicate using IAssetTools
	UObject* NewAsset = AssetToolsInterface.DuplicateAsset(DestName, DestFolder, SourceObject);

	return NewAsset != nullptr;
}

bool FMCPAssetService::DeleteAsset(const FString& AssetPath)
{
	FAssetRegistryModule& RegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& Registry = RegistryModule.Get();

	FString ValidPath = AssetPath;
	if (!ValidPath.StartsWith(TEXT("/")))
	{
		ValidPath = TEXT("/Game/") + ValidPath;
	}

	// Get asset data
	FAssetData AssetData = Registry.GetAssetByObjectPath(FName(*ValidPath));

	if (!AssetData.IsValid())
	{
		return false;
	}

	// Get the asset object
	UObject* AssetObject = AssetData.GetAsset();
	if (!AssetObject)
	{
		return false;
	}

	// Create array for deletion
	TArray<UObject*> AssetsToDelete;
	AssetsToDelete.Add(AssetObject);

	// Delete assets using ObjectTools
	int32 DeletedCount = ObjectTools::DeleteObjects(AssetsToDelete, false);

	return DeletedCount > 0;
}

bool FMCPAssetService::RenameAsset(const FString& OldPath, const FString& NewPath)
{
	if (!EnsureAssetToolsLoaded())
	{
		return false;
	}

	FAssetRegistryModule& RegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& Registry = RegistryModule.Get();

	FString ValidOldPath = OldPath;
	FString ValidNewPath = NewPath;

	if (!ValidOldPath.StartsWith(TEXT("/")))
	{
		ValidOldPath = TEXT("/Game/") + ValidOldPath;
	}

	if (!ValidNewPath.StartsWith(TEXT("/")))
	{
		ValidNewPath = TEXT("/Game/") + ValidNewPath;
	}

	// Get source asset data
	FAssetData SourceAssetData = Registry.GetAssetByObjectPath(FName(*ValidOldPath));

	if (!SourceAssetData.IsValid())
	{
		return false;
	}

	// Parse new path
	FString NewFolder, NewName;
	if (!ValidNewPath.Split(TEXT("/"), &NewFolder, &NewName, ESearchCase::CaseSensitive, ESearchDir::FromEnd))
	{
		NewFolder = ValidNewPath;
		NewName = SourceAssetData.AssetName.ToString();
	}

	if (NewFolder.IsEmpty())
	{
		NewFolder = SourceAssetData.PackagePath.ToString();
	}

	// Create rename data
	FAssetRenameData RenameData;
	RenameData.Asset = SourceAssetData.GetAsset();
	RenameData.NewPackagePath = NewFolder;
	RenameData.NewName = NewName;

	TArray<FAssetRenameData> RenameList;
	RenameList.Add(RenameData);

	// Perform rename
	FAssetToolsModule& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	bool bSuccess = AssetTools.Get().RenameAssets(RenameList);

	return bSuccess;
}

bool FMCPAssetService::ImportAsset(const FString& SourceFile, const FString& DestPath)
{
	if (!EnsureAssetToolsLoaded())
	{
		return false;
	}

	// Check if source file exists
	if (!FPaths::FileExists(SourceFile))
	{
		return false;
	}

	FString ValidDestPath = DestPath.IsEmpty() ? TEXT("/Game") : DestPath;
	if (!ValidDestPath.StartsWith(TEXT("/")))
	{
		ValidDestPath = TEXT("/Game/") + ValidDestPath;
	}

	FAssetToolsModule& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	IAssetTools& AssetToolsInterface = AssetTools.Get();

	// Import single file
	TArray<FString> Files;
	Files.Add(SourceFile);

	TArray<UObject*> ImportedAssets = AssetToolsInterface.ImportAssets(Files, ValidDestPath);

	return ImportedAssets.Num() > 0;
}

bool FMCPAssetService::ExportAsset(const FString& AssetPath, const FString& DestFile)
{
	if (!EnsureAssetRegistryLoaded())
	{
		return false;
	}

	FAssetRegistryModule& RegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& Registry = RegistryModule.Get();

	FString ValidPath = AssetPath;
	if (!ValidPath.StartsWith(TEXT("/")))
	{
		ValidPath = TEXT("/Game/") + ValidPath;
	}

	FAssetData AssetData = Registry.GetAssetByObjectPath(FName(*ValidPath));

	if (!AssetData.IsValid())
	{
		return false;
	}

	UObject* AssetObject = AssetData.GetAsset();
	if (!AssetObject)
	{
		return false;
	}

	// Use FExporters to export
	FString ExportFilename = DestFile;
	if (ExportFilename.IsEmpty())
	{
		ExportFilename = FPaths::GetBaseFilename(AssetPath) + TEXT(".uasset");
	}

	// Create export task
	UAssetExportTask* ExportTask = NewObject<UAssetExportTask>();
	ExportTask->Object = AssetObject;
	ExportTask->Filename = ExportFilename;
	ExportTask->bSelected = false;
	ExportTask->bReplaceIdentical = true;
	ExportTask->bPrompt = false;
	bool bSuccess = UExporter::RunAssetExportTask(ExportTask);
	return bSuccess;
}

bool FMCPAssetService::GetDependencies(const FString& AssetPath, TArray<FName>& OutDependencies)
{
	if (!EnsureAssetRegistryLoaded())
	{
		return false;
	}

	FAssetRegistryModule& RegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& Registry = RegistryModule.Get();

	FString ValidPath = AssetPath;
	if (!ValidPath.StartsWith(TEXT("/")))
	{
		ValidPath = TEXT("/Game/") + ValidPath;
	}

	// Get dependencies using the correct API
	bool bSuccess = Registry.GetDependencies(FName(*ValidPath), OutDependencies,
		UE::AssetRegistry::EDependencyCategory::Package,
		UE::AssetRegistry::EDependencyQuery::Hard);

	// Also get soft dependencies
	TArray<FName> SoftDependencies;
	Registry.GetDependencies(FName(*ValidPath), SoftDependencies,
		UE::AssetRegistry::EDependencyCategory::Package,
		UE::AssetRegistry::EDependencyQuery::Soft);

	// Merge soft dependencies
	for (const FName& SoftDep : SoftDependencies)
	{
		if (!OutDependencies.Contains(SoftDep))
		{
			OutDependencies.Add(SoftDep);
		}
	}

	return bSuccess || OutDependencies.Num() > 0;
}

bool FMCPAssetService::GetReferencers(const FString& AssetPath, TArray<FName>& OutReferencers)
{
	if (!EnsureAssetRegistryLoaded())
	{
		return false;
	}

	FAssetRegistryModule& RegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& Registry = RegistryModule.Get();

	FString ValidPath = AssetPath;
	if (!ValidPath.StartsWith(TEXT("/")))
	{
		ValidPath = TEXT("/Game/") + ValidPath;
	}

	// Get hard referencers
	bool bSuccess = Registry.GetReferencers(FName(*ValidPath), OutReferencers,
		UE::AssetRegistry::EDependencyCategory::Package,
		UE::AssetRegistry::EDependencyQuery::Hard);

	// Also get soft referencers
	TArray<FName> SoftReferencers;
	Registry.GetReferencers(FName(*ValidPath), SoftReferencers,
		UE::AssetRegistry::EDependencyCategory::Package,
		UE::AssetRegistry::EDependencyQuery::Soft);

	// Merge soft referencers
	for (const FName& SoftRef : SoftReferencers)
	{
		if (!OutReferencers.Contains(SoftRef))
		{
			OutReferencers.Add(SoftRef);
		}
	}

	return bSuccess || OutReferencers.Num() > 0;
}
// From Penguin Assistant End
