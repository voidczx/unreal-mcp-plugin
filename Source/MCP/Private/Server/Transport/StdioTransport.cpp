// Copyright Epic Games, Inc. All Rights Reserved.
// From Penguin Assistant Start
#include "Server/Transport/StdioTransport.h"

FStdioTransport::FStdioTransport()
	: bIsRunning(false)
{
}

FStdioTransport::~FStdioTransport()
{
	Stop();
}

bool FStdioTransport::Initialize()
{
	// Stdio is always available on Windows
	return true;
}

void FStdioTransport::Start()
{
	if (bIsRunning)
	{
		return;
	}

	bIsRunning = true;
	UE_LOG(LogTemp, Log, TEXT("MCP Stdio Transport started"));
}

void FStdioTransport::Stop()
{
	if (!bIsRunning)
	{
		return;
	}

	bIsRunning = false;
}

void FStdioTransport::Send(const FString& Message)
{
	// Write to stdout - will be handled by external process
	UE_LOG(LogTemp, Log, TEXT("MCP Response: %s"), *Message);
}

FString FStdioTransport::ReadLine()
{
	// Simplified - read from console would require platform-specific code
	return FString();
}

void FStdioTransport::ReadLoop()
{
	while (bIsRunning)
	{
		FString Line = ReadLine();
		if (!Line.IsEmpty() && OnMessage.IsBound())
		{
			OnMessage.Execute(Line);
		}
	}
}
// From Penguin Assistant End