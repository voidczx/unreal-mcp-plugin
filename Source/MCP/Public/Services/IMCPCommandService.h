// Copyright Epic Games, Inc. All Rights Reserved.
// From Penguin Assistant Start
#pragma once

#include "CoreMinimal.h"
#include "IMCPService.h"

/**
 * Command Service Interface
 * Provides command execution capabilities via MCP protocol
 */
class IMCPCommandService : public IMCPServiceBase
{
public:
	// Execute editor console command
	virtual bool ExecuteCommand(const FString& Command, FString& OutResult) = 0;

	// Execute Python script
	virtual bool ExecutePython(const FString& Script, FString& OutResult) = 0;
};
// From Penguin Assistant End