// Copyright Epic Games, Inc. All Rights Reserved.
// From Penguin Assistant Start
#pragma once

#include "CoreMinimal.h"
#include "Services/IMCPCommandService.h"

/**
 * Command Service Implementation
 * Provides command execution capabilities
 */
class FMCPCommandService : public IMCPCommandService
{
public:
	FMCPCommandService();
	virtual ~FMCPCommandService() = default;

	// IMCPCommandService interface
	virtual bool ExecuteCommand(const FString& Command, FString& OutResult) override;
	virtual bool ExecutePython(const FString& Script, FString& OutResult) override;
};
// From Penguin Assistant End