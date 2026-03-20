// Copyright Epic Games, Inc. All Rights Reserved.
// From Penguin Assistant Start
#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * MCP Plugin Module Interface
 * Provides access to MCP server and all services
 */
class IMCPServer;

class IMCPModule : public IModuleInterface
{
public:
	static IMCPModule& Get()
	{
		return FModuleManager::LoadModuleChecked<IMCPModule>("MCP");
	}

	// Server lifecycle
	virtual bool StartServer(const FString& TransportType = TEXT("stdio")) = 0;
	virtual void StopServer() = 0;
	virtual bool IsServerRunning() const = 0;

	// Server access
	virtual TSharedPtr<IMCPServer> GetServer() = 0;

	// Configuration
	virtual void SetServerPort(int32 Port) = 0;
	virtual int32 GetServerPort() const = 0;
};
// From Penguin Assistant End