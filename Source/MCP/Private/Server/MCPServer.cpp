// Copyright Epic Games, Inc. All Rights Reserved.
// From Penguin Assistant Start
#include "MCPServer.h"
#include "Services/MCPAssetService.h"
#include "Services/MCPCommandService.h"
#include "Services/MCPEditorService.h"
#include "Server/Transport/StdioTransport.h"
#include "Server/Transport/TCPTransport.h"
#include "JsonObject.h"
#include "JsonObjectConverter.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "AssetData.h"

FMCPServer::FMCPServer()
	: bIsInitialized(false)
{
}

FMCPServer::~FMCPServer()
{
	Shutdown();
}

void FMCPServer::Initialize(TSharedPtr<FMCPAssetService> InAssetService,
	TSharedPtr<FMCPCommandService> InCommandService,
	TSharedPtr<FMCPEditorService> InEditorService)
{
	AssetService = InAssetService;
	CommandService = InCommandService;
	EditorService = InEditorService;

	RegisterTools();

	bIsInitialized = true;
}

void FMCPServer::Shutdown()
{
	StopTransport();

	Connections.Empty();
	AvailableTools.Empty();

	bIsInitialized = false;
}

void FMCPServer::AddConnection(const FString& ClientId, TSharedPtr<IMCPAgent> Agent)
{
	Connections.Add(ClientId, Agent);
}

void FMCPServer::RemoveConnection(const FString& ClientId)
{
	Connections.Remove(ClientId);
}

TSharedPtr<IMCPAgent> FMCPServer::GetConnection(const FString& ClientId)
{
	return Connections.FindRef(ClientId);
}

void FMCPServer::ProcessMessage(const FString& ClientId, const FMCPMesage& Message)
{
	// Route message to appropriate handler based on method string
	if (Message.Method == TEXT("initialize"))
	{
		// Handle initialize - return server capabilities
		FMCPMesage Response = BuildInitializeResponse(Message);
		SendResponse(ClientId, Response);
	}
	else if (Message.Method == TEXT("tools/list"))
	{
		// Return available tools
		FMCPToolsListResult ToolsResult = BuildToolsListResponse();

		FMCPMesage Response;
		Response.Id = Message.Id;
		Response.JsonRpc = TEXT("2.0");

		// Build tools array JSON
		TArray<TSharedPtr<FJsonValue>> ToolsArray;
		for (const FMCPTool& Tool : ToolsResult.Tools)
		{
			TSharedPtr<FJsonObject> ToolJson = MakeShared<FJsonObject>();
			ToolJson->SetStringField(TEXT("name"), Tool.Name);
			ToolJson->SetStringField(TEXT("description"), Tool.Description);
			if (Tool.InputSchema.IsValid())
			{
				ToolJson->SetObjectField(TEXT("inputSchema"), Tool.InputSchema);
			}
			ToolsArray.Add(MakeShared<FJsonValueObject>(ToolJson));
		}

		TSharedPtr<FJsonObject> ResultJson = MakeShared<FJsonObject>();
		ResultJson->SetArrayField(TEXT("tools"), ToolsArray);
		Response.Result = ResultJson;

		SendResponse(ClientId, Response);
	}
	else if (Message.Method == TEXT("tools/call"))
	{
		// Parse tool call request from params
		FMCPToolCallRequest Request;
		if (Message.Params.IsValid())
		{
			Request.Name = Message.Params->GetStringField(TEXT("name"));
			if (Message.Params->HasField(TEXT("arguments")))
			{
				Request.Arguments = Message.Params->GetObjectField(TEXT("arguments"));
			}
		}

		FMCPToolCallResult Result;
		ProcessToolCall(Request, Result);

		// Build response
		FMCPMesage Response;
		Response.Id = Message.Id;
		Response.JsonRpc = TEXT("2.0");

		TSharedPtr<FJsonObject> ResultJson = MakeShared<FJsonObject>();
		ResultJson->SetStringField(TEXT("tool"), Result.ToolCallId);
		if (Result.Content.IsValid())
		{
			ResultJson->SetObjectField(TEXT("content"), Result.Content);
		}
		Response.Result = ResultJson;

		SendResponse(ClientId, Response);
	}
	else if (Message.Method == TEXT("ping"))
	{
		// Handle ping - return pong
		FMCPMesage Response;
		Response.Id = Message.Id;
		Response.JsonRpc = TEXT("2.0");
		Response.Result = MakeShared<FJsonObject>();
		SendResponse(ClientId, Response);
	}
	else
	{
		// Unknown method - return error
		FMCPMesage Response;
		Response.Id = Message.Id;
		Response.JsonRpc = TEXT("2.0");
		Response.Type = TEXT("Error");

		TSharedPtr<FJsonObject> ErrorJson = MakeShared<FJsonObject>();
		ErrorJson->SetNumberField(TEXT("code"), -32601);
		ErrorJson->SetStringField(TEXT("message"), FString::Printf(TEXT("Method not found: %s"), *Message.Method));
		Response.Result = ErrorJson;

		SendResponse(ClientId, Response);
	}
}

FMCPMesage FMCPServer::CreateResponse(const FMCPMesage& Request, bool bSuccess, const FString& Result)
{
	FMCPMesage Response;
	Response.Id = Request.Id;
	Response.JsonRpc = TEXT("2.0");
	Response.Type = bSuccess ? TEXT("Response") : TEXT("Error");

	if (bSuccess)
	{
		TSharedPtr<FJsonObject> ResultJson = MakeShared<FJsonObject>();
		ResultJson->SetStringField(TEXT("content"), Result);
		Response.Result = ResultJson;
	}
	else
	{
		Response.Error = Result;
	}

	return Response;
}

void FMCPServer::SendResponse(const FString& ClientId, const FMCPMesage& Response)
{
	// Serialize response to JSON
	TSharedPtr<FJsonObject> ResponseJson = MakeShared<FJsonObject>();
	ResponseJson->SetStringField(TEXT("jsonrpc"), Response.JsonRpc);
	ResponseJson->SetStringField(TEXT("id"), Response.Id);

	if (Response.Result.IsValid())
	{
		ResponseJson->SetObjectField(TEXT("result"), Response.Result);
	}

	if (!Response.Error.IsEmpty())
	{
		TSharedPtr<FJsonObject> ErrorJson = MakeShared<FJsonObject>();
		ErrorJson->SetStringField(TEXT("message"), Response.Error);
		ResponseJson->SetObjectField(TEXT("error"), ErrorJson);
	}

	FString JsonStr;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonStr);
	FJsonSerializer::Serialize(ResponseJson.ToSharedRef(), Writer);

	// Send via appropriate transport
	if (ClientId == TEXT("stdio"))
	{
		if (StdioTransport.IsValid())
		{
			StdioTransport->Send(JsonStr);
		}
	}
	else if (TCPTransport.IsValid())
	{
		// For TCP clients, ClientId is the IP:Port address
		TCPTransport->Send(ClientId, JsonStr);
	}

	UE_LOG(LogTemp, Log, TEXT("MCP Response sent: %s"), *JsonStr);
}

FMCPMesage FMCPServer::BuildInitializeResponse(const FMCPMesage& Request)
{
	FMCPMesage Response;
	Response.Id = Request.Id;
	Response.JsonRpc = TEXT("2.0");

	TSharedPtr<FJsonObject> ResultJson = MakeShared<FJsonObject>();
	ResultJson->SetStringField(TEXT("protocolVersion"), TEXT("2024-11-05"));

	// Capabilities
	TSharedPtr<FJsonObject> CapabilitiesJson = MakeShared<FJsonObject>();
	CapabilitiesJson->SetBoolField(TEXT("tools"), true);
	CapabilitiesJson->SetBoolField(TEXT("resources"), false);
	CapabilitiesJson->SetBoolField(TEXT("prompts"), false);
	ResultJson->SetObjectField(TEXT("capabilities"), CapabilitiesJson);

	// Server info
	TSharedPtr<FJsonObject> ServerInfoJson = MakeShared<FJsonObject>();
	ServerInfoJson->SetStringField(TEXT("name"), TEXT("unreal-editor-mcp"));
	ServerInfoJson->SetStringField(TEXT("version"), TEXT("1.0.0"));
	ResultJson->SetObjectField(TEXT("serverInfo"), ServerInfoJson);

	Response.Result = ResultJson;

	return Response;
}

FMCPToolsListResult FMCPServer::BuildToolsListResponse()
{
	FMCPToolsListResult Result;
	Result.Tools = AvailableTools;
	return Result;
}

void FMCPServer::StartStdioTransport()
{
	StdioTransport = MakeShared<FStdioTransport>();

	StdioTransport->OnMessage.BindLambda([this](const FString& Message)
	{
		// Parse and process JSON-RPC message
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Message);

		if (FJsonSerializer::Deserialize(Reader, JsonObject))
		{
			FMCPMesage Message;
			Message.JsonRpc = JsonObject->GetStringField(TEXT("jsonrpc"));
			Message.Id = JsonObject->GetStringField(TEXT("id"));
			Message.Method = JsonObject->GetStringField(TEXT("method"));
			Message.Params = JsonObject->GetObjectField(TEXT("params"));

			ProcessMessage(TEXT("stdio"), Message);
		}
	});

	StdioTransport->Initialize();
	StdioTransport->Start();
}

void FMCPServer::StartTCPTransport(int32 Port)
{
	TCPTransport = MakeShared<FTCPTransport>();

	TCPTransport->OnMessage.BindLambda([this](const FString& ClientId, const FString& Message)
	{
		// Parse and process JSON-RPC message
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Message);

		if (FJsonSerializer::Deserialize(Reader, JsonObject))
		{
			FMCPMesage MCPMessage;
			MCPMessage.JsonRpc = JsonObject->GetStringField(TEXT("jsonrpc"));
			MCPMessage.Id = JsonObject->GetStringField(TEXT("id"));
			MCPMessage.Method = JsonObject->GetStringField(TEXT("method"));
			MCPMessage.Params = JsonObject->GetObjectField(TEXT("params"));

			ProcessMessage(ClientId, MCPMessage);
		}
	});

	TCPTransport->Initialize(Port);
	TCPTransport->Start();
}

void FMCPServer::StopTransport()
{
	if (StdioTransport.IsValid())
	{
		StdioTransport->Stop();
		StdioTransport.Reset();
	}

	if (TCPTransport.IsValid())
	{
		TCPTransport->Stop();
		TCPTransport.Reset();
	}
}

void FMCPServer::RegisterTools()
{
	AvailableTools.Empty();

	// Asset tools
	FMCPTool ListAssetsTool;
	ListAssetsTool.Name = TEXT("list_assets");
	ListAssetsTool.Description = TEXT("List all assets in a directory path");
	ListAssetsTool.InputSchema = MakeShared<FJsonObject>();
	ListAssetsTool.InputSchema->SetStringField(TEXT("type"), TEXT("object"));
	ListAssetsTool.InputSchema->SetStringField(TEXT("path"), TEXT("/Game"));
	ListAssetsTool.InputSchema->SetBoolField(TEXT("recursive"), false);
	AvailableTools.Add(ListAssetsTool);

	FMCPTool GetAssetTool;
	GetAssetTool.Name = TEXT("get_asset");
	GetAssetTool.Description = TEXT("Get asset metadata by path");
	GetAssetTool.InputSchema = MakeShared<FJsonObject>();
	GetAssetTool.InputSchema->SetStringField(TEXT("type"), TEXT("object"));
	GetAssetTool.InputSchema->SetStringField(TEXT("asset_path"), TEXT(""));
	AvailableTools.Add(GetAssetTool);

	FMCPTool GetDependenciesTool;
	GetDependenciesTool.Name = TEXT("get_dependencies");
	GetDependenciesTool.Description = TEXT("Get asset dependencies");
	GetDependenciesTool.InputSchema = MakeShared<FJsonObject>();
	GetDependenciesTool.InputSchema->SetStringField(TEXT("type"), TEXT("object"));
	GetDependenciesTool.InputSchema->SetStringField(TEXT("asset_path"), TEXT(""));
	AvailableTools.Add(GetDependenciesTool);

	FMCPTool GetReferencersTool;
	GetReferencersTool.Name = TEXT("get_referencers");
	GetReferencersTool.Description = TEXT("Get assets that reference this asset");
	GetReferencersTool.InputSchema = MakeShared<FJsonObject>();
	GetReferencersTool.InputSchema->SetStringField(TEXT("type"), TEXT("object"));
	GetReferencersTool.InputSchema->SetStringField(TEXT("asset_path"), TEXT(""));
	AvailableTools.Add(GetReferencersTool);

	FMCPTool CreateAssetTool;
	CreateAssetTool.Name = TEXT("create_asset");
	CreateAssetTool.Description = TEXT("Create a new asset");
	CreateAssetTool.InputSchema = MakeShared<FJsonObject>();
	CreateAssetTool.InputSchema->SetStringField(TEXT("type"), TEXT("object"));
	CreateAssetTool.InputSchema->SetStringField(TEXT("name"), TEXT(""));
	CreateAssetTool.InputSchema->SetStringField(TEXT("path"), TEXT("/Game"));
	CreateAssetTool.InputSchema->SetStringField(TEXT("class"), TEXT(""));
	AvailableTools.Add(CreateAssetTool);

	FMCPTool DuplicateAssetTool;
	DuplicateAssetTool.Name = TEXT("duplicate_asset");
	DuplicateAssetTool.Description = TEXT("Duplicate an existing asset");
	DuplicateAssetTool.InputSchema = MakeShared<FJsonObject>();
	DuplicateAssetTool.InputSchema->SetStringField(TEXT("type"), TEXT("object"));
	DuplicateAssetTool.InputSchema->SetStringField(TEXT("source_path"), TEXT(""));
	DuplicateAssetTool.InputSchema->SetStringField(TEXT("dest_path"), TEXT(""));
	AvailableTools.Add(DuplicateAssetTool);

	FMCPTool DeleteAssetTool;
	DeleteAssetTool.Name = TEXT("delete_asset");
	DeleteAssetTool.Description = TEXT("Delete an asset");
	DeleteAssetTool.InputSchema = MakeShared<FJsonObject>();
	DeleteAssetTool.InputSchema->SetStringField(TEXT("type"), TEXT("object"));
	DeleteAssetTool.InputSchema->SetStringField(TEXT("asset_path"), TEXT(""));
	AvailableTools.Add(DeleteAssetTool);

	FMCPTool RenameAssetTool;
	RenameAssetTool.Name = TEXT("rename_asset");
	RenameAssetTool.Description = TEXT("Rename an asset");
	RenameAssetTool.InputSchema = MakeShared<FJsonObject>();
	RenameAssetTool.InputSchema->SetStringField(TEXT("type"), TEXT("object"));
	RenameAssetTool.InputSchema->SetStringField(TEXT("old_path"), TEXT(""));
	RenameAssetTool.InputSchema->SetStringField(TEXT("new_path"), TEXT(""));
	AvailableTools.Add(RenameAssetTool);

	// Command tools
	FMCPTool ExecuteCommandTool;
	ExecuteCommandTool.Name = TEXT("execute_command");
	ExecuteCommandTool.Description = TEXT("Execute an editor console command");
	ExecuteCommandTool.InputSchema = MakeShared<FJsonObject>();
	ExecuteCommandTool.InputSchema->SetStringField(TEXT("type"), TEXT("object"));
	ExecuteCommandTool.InputSchema->SetStringField(TEXT("command"), TEXT(""));
	AvailableTools.Add(ExecuteCommandTool);

	FMCPTool ExecutePythonTool;
	ExecutePythonTool.Name = TEXT("execute_python");
	ExecutePythonTool.Description = TEXT("Execute a Python script");
	ExecutePythonTool.InputSchema = MakeShared<FJsonObject>();
	ExecutePythonTool.InputSchema->SetStringField(TEXT("type"), TEXT("object"));
	ExecutePythonTool.InputSchema->SetStringField(TEXT("script"), TEXT(""));
	AvailableTools.Add(ExecutePythonTool);

	// Editor tools
	FMCPTool SaveAssetTool;
	SaveAssetTool.Name = TEXT("save_asset");
	SaveAssetTool.Description = TEXT("Save an asset");
	SaveAssetTool.InputSchema = MakeShared<FJsonObject>();
	SaveAssetTool.InputSchema->SetStringField(TEXT("type"), TEXT("object"));
	SaveAssetTool.InputSchema->SetStringField(TEXT("asset_path"), TEXT(""));
	AvailableTools.Add(SaveAssetTool);

	FMCPTool CompileBlueprintTool;
	CompileBlueprintTool.Name = TEXT("compile_blueprint");
	CompileBlueprintTool.Description = TEXT("Compile a blueprint");
	CompileBlueprintTool.InputSchema = MakeShared<FJsonObject>();
	CompileBlueprintTool.InputSchema->SetStringField(TEXT("type"), TEXT("object"));
	CompileBlueprintTool.InputSchema->SetStringField(TEXT("blueprint_path"), TEXT(""));
	AvailableTools.Add(CompileBlueprintTool);
}

TArray<FMCPTool> FMCPServer::GetAvailableTools() const
{
	return AvailableTools;
}

void FMCPServer::ProcessToolCall(const FMCPToolCallRequest& Request, FMCPToolCallResult& OutResult)
{
	FString Result = ExecuteTool(Request.Name, Request.Arguments);
	OutResult.ToolCallId = Request.Name;

	TSharedPtr<FJsonObject> ContentJson = MakeShared<FJsonObject>();
	ContentJson->SetStringField(TEXT("result"), Result);
	OutResult.Content = ContentJson;
}

FString FMCPServer::ExecuteTool(const FString& ToolName, const TSharedPtr<FJsonObject>& Arguments)
{
	if (!AssetService.IsValid() || !CommandService.IsValid() || !EditorService.IsValid())
	{
		return TEXT("Error: Services not initialized");
	}

	FString Result;

	if (ToolName == TEXT("list_assets"))
	{
		FString Path = Arguments->GetStringField(TEXT("path"));
		bool bRecursive = Arguments->GetBoolField(TEXT("recursive"));
		FString Filter = Arguments->GetStringField(TEXT("filter"));

		TArray<FAssetData> Assets;
		if (AssetService->ListAssets(Path, bRecursive, Filter, Assets))
		{
			TArray<TSharedPtr<FJsonValue>> AssetArray;
			for (const FAssetData& Asset : Assets)
			{
				TSharedPtr<FJsonObject> AssetJson = MakeShared<FJsonObject>();
				AssetJson->SetStringField(TEXT("path"), Asset.ObjectPath.ToString());
				AssetJson->SetStringField(TEXT("class"), Asset.AssetClass.ToString());
				AssetArray.Add(MakeShared<FJsonValueObject>(AssetJson));
			}

			TSharedPtr<FJsonObject> ResultJson = MakeShared<FJsonObject>();
			ResultJson->SetArrayField(TEXT("assets"), AssetArray);

			FString JsonStr;
			TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonStr);
			FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);

			Result = JsonStr;
		}
		else
		{
			Result = TEXT("Error: Failed to list assets");
		}
	}
	else if (ToolName == TEXT("get_asset"))
	{
		FString AssetPath = Arguments->GetStringField(TEXT("asset_path"));

		TSharedPtr<FJsonObject> Metadata;
		if (AssetService->GetAssetMetadata(AssetPath, Metadata))
		{
			FString JsonStr;
			TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonStr);
			FJsonSerializer::Serialize(Metadata.ToSharedRef(), Writer);
			Result = JsonStr;
		}
		else
		{
			Result = TEXT("Error: Asset not found");
		}
	}
	else if (ToolName == TEXT("get_dependencies"))
	{
		FString AssetPath = Arguments->GetStringField(TEXT("asset_path"));

		TArray<FName> Dependencies;
		if (AssetService->GetDependencies(AssetPath, Dependencies))
		{
			TArray<TSharedPtr<FJsonValue>> DepArray;
			for (const FName& Dep : Dependencies)
			{
				DepArray.Add(MakeShared<FJsonValueString>(Dep.ToString()));
			}

			TSharedPtr<FJsonObject> ResultJson = MakeShared<FJsonObject>();
			ResultJson->SetArrayField(TEXT("dependencies"), DepArray);

			FString JsonStr;
			TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonStr);
			FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);
			Result = JsonStr;
		}
		else
		{
			Result = TEXT("Error: Failed to get dependencies");
		}
	}
	else if (ToolName == TEXT("get_referencers"))
	{
		FString AssetPath = Arguments->GetStringField(TEXT("asset_path"));

		TArray<FName> Referencers;
		if (AssetService->GetReferencers(AssetPath, Referencers))
		{
			TArray<TSharedPtr<FJsonValue>> RefArray;
			for (const FName& Ref : Referencers)
			{
				RefArray.Add(MakeShared<FJsonValueString>(Ref.ToString()));
			}

			TSharedPtr<FJsonObject> ResultJson = MakeShared<FJsonObject>();
			ResultJson->SetArrayField(TEXT("referencers"), RefArray);

			FString JsonStr;
			TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonStr);
			FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);
			Result = JsonStr;
		}
		else
		{
			Result = TEXT("Error: Failed to get referencers");
		}
	}
	else if (ToolName == TEXT("create_asset"))
	{
		FString Name = Arguments->GetStringField(TEXT("name"));
		FString Path = Arguments->GetStringField(TEXT("path"));
		FString AssetClass = Arguments->GetStringField(TEXT("class"));

		if (AssetService->CreateAsset(Name, Path, AssetClass))
		{
			Result = TEXT("Success");
		}
		else
		{
			Result = TEXT("Error: Failed to create asset");
		}
	}
	else if (ToolName == TEXT("duplicate_asset"))
	{
		FString SourcePath = Arguments->GetStringField(TEXT("source_path"));
		FString DestPath = Arguments->GetStringField(TEXT("dest_path"));

		if (AssetService->DuplicateAsset(SourcePath, DestPath))
		{
			Result = TEXT("Success");
		}
		else
		{
			Result = TEXT("Error: Failed to duplicate asset");
		}
	}
	else if (ToolName == TEXT("delete_asset"))
	{
		FString AssetPath = Arguments->GetStringField(TEXT("asset_path"));

		if (AssetService->DeleteAsset(AssetPath))
		{
			Result = TEXT("Success");
		}
		else
		{
			Result = TEXT("Error: Failed to delete asset");
		}
	}
	else if (ToolName == TEXT("rename_asset"))
	{
		FString OldPath = Arguments->GetStringField(TEXT("old_path"));
		FString NewPath = Arguments->GetStringField(TEXT("new_path"));

		if (AssetService->RenameAsset(OldPath, NewPath))
		{
			Result = TEXT("Success");
		}
		else
		{
			Result = TEXT("Error: Failed to rename asset");
		}
	}
	else if (ToolName == TEXT("execute_command"))
	{
		FString Command = Arguments->GetStringField(TEXT("command"));

		if (CommandService->ExecuteCommand(Command, Result))
		{
			// Result contains the output
		}
		else
		{
			Result = TEXT("Error: Failed to execute command");
		}
	}
	else if (ToolName == TEXT("execute_python"))
	{
		FString Script = Arguments->GetStringField(TEXT("script"));

		if (CommandService->ExecutePython(Script, Result))
		{
			// Result contains the output
		}
		else
		{
			Result = TEXT("Error: Failed to execute Python");
		}
	}
	else if (ToolName == TEXT("save_asset"))
	{
		FString AssetPath = Arguments->GetStringField(TEXT("asset_path"));

		if (EditorService->SaveAsset(AssetPath))
		{
			Result = TEXT("Success");
		}
		else
		{
			Result = TEXT("Error: Failed to save asset");
		}
	}
	else if (ToolName == TEXT("compile_blueprint"))
	{
		FString BlueprintPath = Arguments->GetStringField(TEXT("blueprint_path"));

		if (EditorService->CompileBlueprint(BlueprintPath))
		{
			Result = TEXT("Success");
		}
		else
		{
			Result = TEXT("Error: Failed to compile blueprint");
		}
	}
	else
	{
		Result = FString::Printf(TEXT("Error: Unknown tool '%s'"), *ToolName);
	}

	return Result;
}
// From Penguin Assistant End
