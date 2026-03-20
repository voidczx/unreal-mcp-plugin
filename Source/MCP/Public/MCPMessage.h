// Copyright Epic Games, Inc. All Rights Reserved.
// From Penguin Assistant Start
#pragma once

#include "CoreMinimal.h"
#include "JsonObject.h"

/**
 * MCP Protocol Message Types
 * Based on Model Context Protocol specification (JSON-RPC 2.0)
 */
enum class EMCPMessageType : uint8
{
	Initialize,
	Initialized,
	ToolsList,
	ToolsCall,
	ResourcesList,
	ResourcesRead,
	Prompt,
	Complete,
	Response,
	Error
};

struct FMCPMesage
{
	FString Id;
	FString JsonRpc;
	FString Type;
	FString Method;
	TSharedPtr<FJsonObject> Params;
	TSharedPtr<FJsonObject> Result;
	FString Error;
};

struct FMCPTool
{
	FString Name;
	FString Description;
	TSharedPtr<FJsonObject> InputSchema;
};

struct FMCPToolCallRequest
{
	FString Name;
	TSharedPtr<FJsonObject> Arguments;
};

struct FMCPToolCallResult
{
	FString ToolCallId;
	TSharedPtr<FJsonObject> Content;
};

struct FMCPToolsListResult
{
	TArray<FMCPTool> Tools;
};
// From Penguin Assistant End
