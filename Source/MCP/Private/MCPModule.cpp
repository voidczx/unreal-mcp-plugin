// Copyright Epic Games, Inc. All Rights Reserved.

// From Penguin Assistant Start
#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "IMCPModule.h"
#include "Server/MCPServer.h"
#include "Services/MCPAssetService.h"
#include "Services/MCPCommandService.h"
#include "Services/MCPEditorService.h"
#include "IConsoleManager.h"

/**
 * MCP Module Implementation
 */
class FMCPModule : public IMCPModule
{
public:
	FMCPModule()
		: ServerPort(20260)
		, bServerRunning(false)
	{
	}

	// IModuleInterface implementation
	virtual void StartupModule() override
	{
		// Initialize services
		AssetService = MakeShared<FMCPAssetService>();
		CommandService = MakeShared<FMCPCommandService>();
		EditorService = MakeShared<FMCPEditorService>();

		// Initialize server
		Server = MakeShared<FMCPServer>();
		Server->Initialize(AssetService, CommandService, EditorService);

		// Register console commands
		RegisterConsoleCommands();

		UE_LOG(LogTemp, Log, TEXT("MCP Module Started"));
	}

	virtual void ShutdownModule() override
	{
		StopServer();

		Server.Reset();
		AssetService.Reset();
		CommandService.Reset();
		EditorService.Reset();

		UE_LOG(LogTemp, Log, TEXT("MCP Module Shutdown"));
	}

	// IMCPModule interface
	virtual bool StartServer(const FString& TransportType = TEXT("stdio")) override
	{
		if (bServerRunning)
		{
			return true;
		}

		if (!Server.IsValid())
		{
			return false;
		}

		if (TransportType == TEXT("stdio"))
		{
			Server->StartStdioTransport();
		}
		else if (TransportType == TEXT("tcp"))
		{
			Server->StartTCPTransport(ServerPort);
		}

		bServerRunning = true;
		UE_LOG(LogTemp, Log, TEXT("MCP Server Started with transport: %s"), *TransportType);

		return true;
	}

	virtual void StopServer() override
	{
		if (!bServerRunning)
		{
			return;
		}

		if (Server.IsValid())
		{
			Server->StopTransport();
		}

		bServerRunning = false;
		UE_LOG(LogTemp, Log, TEXT("MCP Server Stopped"));
	}

	virtual bool IsServerRunning() const override
	{
		return bServerRunning;
	}

	virtual TSharedPtr<IMCPServer> GetServer() override
	{
		return Server;
	}

	virtual void SetServerPort(int32 Port) override
	{
		ServerPort = Port;
	}

	virtual int32 GetServerPort() const override
	{
		return ServerPort;
	}

private:
	void RegisterConsoleCommands()
	{
		// MCP.Start - Start server with stdio transport
		static FAutoConsoleCommand StartStdioCmd(
			TEXT("MCP.Start"),
			TEXT("Start MCP server with stdio transport"),
			FConsoleCommandDelegate::CreateRaw(this, &FMCPModule::StartStdioCommand),
			ECVF_Default
		);

		// MCP.StartTCP - Start server with TCP transport
		static FAutoConsoleCommand StartTCPCmd(
			TEXT("MCP.StartTCP"),
			TEXT("Start MCP server with TCP transport (port 20260)"),
			FConsoleCommandDelegate::CreateRaw(this, &FMCPModule::StartTCPCommand),
			ECVF_Default
		);

		// MCP.Stop - Stop MCP server
		static FAutoConsoleCommand StopCmd(
			TEXT("MCP.Stop"),
			TEXT("Stop MCP server"),
			FConsoleCommandDelegate::CreateRaw(this, &FMCPModule::StopCommand),
			ECVF_Default
		);

		UE_LOG(LogTemp, Log, TEXT("MCP console commands registered"));
	}

	void StartStdioCommand()
	{
		if (StartServer(TEXT("stdio")))
		{
			UE_LOG(LogTemp, Log, TEXT("MCP server started with stdio transport"));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to start MCP server"));
		}
	}

	void StartTCPCommand()
	{
		if (StartServer(TEXT("tcp")))
		{
			UE_LOG(LogTemp, Log, TEXT("MCP server started with TCP transport on port %d"), ServerPort);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to start MCP server"));
		}
	}

	void StopCommand()
	{
		StopServer();
		UE_LOG(LogTemp, Log, TEXT("MCP server stopped"));
	}

	TSharedPtr<FMCPServer> Server;
	TSharedPtr<FMCPAssetService> AssetService;
	TSharedPtr<FMCPCommandService> CommandService;
	TSharedPtr<FMCPEditorService> EditorService;

	int32 ServerPort;
	bool bServerRunning;
};

IMPLEMENT_MODULE(FMCPModule, MCP)
// From Penguin Assistant End