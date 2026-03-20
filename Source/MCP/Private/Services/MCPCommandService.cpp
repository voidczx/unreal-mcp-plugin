// Copyright Epic Games, Inc. All Rights Reserved.
// From Penguin Assistant Start
#include "Services/MCPCommandService.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Editor.h"
#include "ModuleManager.h"
#include "OutputDevice.h"

FMCPCommandService::FMCPCommandService()
{
}

bool FMCPCommandService::ExecuteCommand(const FString& Command, FString& OutResult)
{
	if (GEditor)
	{
		GEditor->Exec(GEditor->GetEditorWorldContext().World(), *Command, *GError);
		return true;
	}
	return false;
}

bool FMCPCommandService::ExecutePython(const FString& Script, FString& OutResult)
{
	// Check if Python plugin is available
	if (FModuleManager::Get().IsModuleLoaded("PythonScriptPlugin"))
	{
		// Python execution would be handled by the Python plugin
		// This is a placeholder for the actual implementation
		OutResult = TEXT("Python execution requires PythonScriptPlugin");
		return false;
	}

	OutResult = TEXT("PythonScriptPlugin not available");
	return false;
}
// From Penguin Assistant End
