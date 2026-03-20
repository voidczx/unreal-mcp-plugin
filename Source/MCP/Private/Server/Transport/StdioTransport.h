// Copyright Epic Games, Inc. All Rights Reserved.
// From Penguin Assistant Start
#pragma once

#include "CoreMinimal.h"

/**
 * Stdio Transport for MCP
 * Uses standard input/output for JSON-RPC communication
 */
class FStdioTransport
{
public:
	FStdioTransport();
	~FStdioTransport();

	bool Initialize();
	void Start();
	void Stop();
	void Send(const FString& Message);

	DECLARE_DELEGATE_OneParam(FOnMessageReceived, const FString&);
	FOnMessageReceived OnMessage;

private:
	void ReadLoop();
	FString ReadLine();

	bool bIsRunning;
};
// From Penguin Assistant End