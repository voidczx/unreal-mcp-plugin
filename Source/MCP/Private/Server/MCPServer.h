// Copyright Epic Games, Inc. All Rights Reserved.
// From Penguin Assistant Start
#pragma once

#include "CoreMinimal.h"
#include "IMCPServer.h"
#include "MCPMessage.h"

/**
 * MCP Server Implementation
 * Manages connections and message routing
 */
class FMCPAssetService;
class FMCPCommandService;
class FMCPEditorService;
class FStdioTransport;
class FTCPTransport;

class FMCPServer : public IMCPServer
{
public:
	FMCPServer();
	virtual ~FMCPServer();

	// IModuleInterface
	void Initialize(TSharedPtr<FMCPAssetService> InAssetService,
		TSharedPtr<FMCPCommandService> InCommandService,
		TSharedPtr<FMCPEditorService> InEditorService);
	void ShutdownServer();

	// IMCPServer interface
	virtual void AddConnection(const FString& ClientId, TSharedPtr<IMCPAgent> Agent) override;
	virtual void RemoveConnection(const FString& ClientId) override;
	virtual TSharedPtr<IMCPAgent> GetConnection(const FString& ClientId) override;
	virtual void ProcessMessage(const FString& ClientId, const FMCPMesage& Message) override;
	virtual FMCPMesage CreateResponse(const FMCPMesage& Request, bool bSuccess, const FString& Result) override;
	virtual void Shutdown() override;

	// Transport
	void StartStdioTransport();
	void StartTCPTransport(int32 Port = 20260);
	void StopTransport();

	// Tool registration
	void RegisterTools();
	TArray<FMCPTool> GetAvailableTools() const;

private:
	void ProcessToolCall(const FMCPToolCallRequest& Request, FMCPToolCallResult& OutResult);
	FString ExecuteTool(const FString& ToolName, const TSharedPtr<FJsonObject>& Arguments);
	void SendResponse(const FString& ClientId, const FMCPMesage& Response);
	FMCPMesage BuildInitializeResponse(const FMCPMesage& Request);
	FMCPToolsListResult BuildToolsListResponse();

	TSharedPtr<FMCPAssetService> AssetService;
	TSharedPtr<FMCPCommandService> CommandService;
	TSharedPtr<FMCPEditorService> EditorService;

	TSharedPtr<FStdioTransport> StdioTransport;
	TSharedPtr<FTCPTransport> TCPTransport;

	TMap<FString, TSharedPtr<IMCPAgent>> Connections;
	TArray<FMCPTool> AvailableTools;

	bool bIsInitialized;
};
// From Penguin Assistant End