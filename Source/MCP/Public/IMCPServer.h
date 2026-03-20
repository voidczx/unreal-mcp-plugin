// Copyright Epic Games, Inc. All Rights Reserved.
// From Penguin Assistant Start
#pragma once

#include "CoreMinimal.h"
#include "MCPMessage.h"

/**
 * MCP Server Interface
 * Manages connections and message routing
 */
class IMCPAgent;

class IMCPServer : public TSharedFromThis<IMCPServer>
{
public:
	virtual ~IMCPServer() = default;

	// Connection management
	virtual void AddConnection(const FString& ClientId, TSharedPtr<IMCPAgent> Agent) = 0;
	virtual void RemoveConnection(const FString& ClientId) = 0;
	virtual TSharedPtr<IMCPAgent> GetConnection(const FString& ClientId) = 0;

	// Message processing
	virtual void ProcessMessage(const FString& ClientId, const FMCPMesage& Message) = 0;
	virtual FMCPMesage CreateResponse(const FMCPMesage& Request, bool bSuccess, const FString& Result) = 0;

	// Server control
	virtual void Shutdown() = 0;
};
// From Penguin Assistant End