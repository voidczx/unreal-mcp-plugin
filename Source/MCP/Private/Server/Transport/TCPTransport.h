// Copyright Epic Games, Inc. All Rights Reserved.
// From Penguin Assistant Start
#pragma once

#include "CoreMinimal.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Common/TcpSocketBuilder.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "Interfaces/IPv4/IPv4Endpoint.h"
#include "HAL/PlatformTime.h"
#include "HAL/RunnableThread.h"
#include "Runnable.h"

/**
 * TCP Transport for MCP
 * Uses TCP socket for JSON-RPC communication
 */
class FTCPTransport : public FRunnable
{
public:
	FTCPTransport();
	~FTCPTransport();

	bool Initialize(int32 Port = 20260);
	void Start();
	void Stop();
	void Send(const FString& ClientId, const FString& Message);

	// FRunnable interface
	virtual uint32 Run() override;

	DECLARE_DELEGATE_TwoParams(FOnMessageReceived, const FString&, const FString&);
	FOnMessageReceived OnMessage;

	DECLARE_DELEGATE_OneParam(FOnClientConnected, const FString&);
	FOnClientConnected OnClientConnected;

	DECLARE_DELEGATE_OneParam(FOnClientDisconnected, const FString&);
	FOnClientDisconnected OnClientDisconnected;

private:
	void ReceiveLoop(FSocket* ClientSocket, const FString& ClientId);
	bool SendToSocket(FSocket* Socket, const FString& Message);

	int32 ServerPort;
	bool bIsRunning;
	FSocket* ListenSocket;
	FRunnableThread* AcceptThread;

	// Map of connected clients (ClientId -> Socket)
	TMap<FString, FSocket*> ConnectedClients;
	FCriticalSection ClientMapLock;

	// Message framing: 4-byte length prefix + JSON body
	static const int32 HEADER_SIZE = 4;
};
// From Penguin Assistant End