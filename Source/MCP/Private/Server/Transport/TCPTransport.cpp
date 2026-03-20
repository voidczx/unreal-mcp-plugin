// Copyright Epic Games, Inc. All Rights Reserved.
// From Penguin Assistant Start
#include "Server/Transport/TCPTransport.h"
#include "SocketSubsystem.h"
#include "Common/TcpSocketBuilder.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "Interfaces/IPv4/IPv4Endpoint.h"
#include "HAL/PlatformTime.h"
#include "HAL/RunnableThread.h"
#include "Async/Async.h"

FTCPTransport::FTCPTransport()
	: ServerPort(20260)
	, bIsRunning(false)
	, ListenSocket(nullptr)
	, AcceptThread(nullptr)
{
}

FTCPTransport::~FTCPTransport()
{
	Stop();
}

bool FTCPTransport::Initialize(int32 Port)
{
	ServerPort = Port;
	return true;
}

void FTCPTransport::Start()
{
	if (bIsRunning)
	{
		return;
	}

	// Create TCP listening socket using FTcpSocketBuilder
	ListenSocket = FTcpSocketBuilder(TEXT("MCPServerListen"))
		.AsReusable()
		.BoundToAddress(FIPv4Address::Any)
		.BoundToPort(ServerPort)
		.Listening(8);

	if (!ListenSocket)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create TCP listen socket"));
		return;
	}

	// Set socket options
	ListenSocket->SetReuseAddr(true);
	ListenSocket->SetNoDelay(true);

	// Get actual bound port
	TSharedRef<FInternetAddr> Addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	ListenSocket->GetAddress(*Addr);
	int32 BoundPort = Addr->GetPort();
	if (BoundPort != ServerPort)
	{
		UE_LOG(LogTemp, Log, TEXT("TCP server bound to port %d (requested %d)"), BoundPort, ServerPort);
		ServerPort = BoundPort;
	}

	bIsRunning = true;
	UE_LOG(LogTemp, Log, TEXT("MCP TCP Transport started on port %d"), ServerPort);

	// Start background thread for accepting connections
	AcceptThread = FRunnableThread::Create(this, TEXT("MCPAcceptThread"), 128 * 1024, TPri_Normal);
}

uint32 FTCPTransport::Run()
{
	while (bIsRunning && ListenSocket)
	{
		TSharedRef<FInternetAddr> RemoteAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();

		bool bHasPendingConnection = false;
		ListenSocket->HasPendingConnection(bHasPendingConnection);
		if (bHasPendingConnection)
		{
			FSocket* NewSocket = ListenSocket->Accept(*RemoteAddr, TEXT("MCPServerListen"));
			if (NewSocket)
			{
				NewSocket->SetNoDelay(true);

				FString ClientId = RemoteAddr->ToString(false);
				UE_LOG(LogTemp, Log, TEXT("Client connected: %s"), *ClientId);

				// Add to client map
				{
					FScopeLock Lock(&ClientMapLock);
					ConnectedClients.Add(ClientId, NewSocket);
				}

				// Notify client connected
				if (OnClientConnected.IsBound())
				{
					OnClientConnected.Execute(ClientId);
				}

				// Handle client in separate thread using async task
				Async(EAsyncExecution::ThreadPool, [this, NewSocket, ClientId]()
				{
					ReceiveLoop(NewSocket, ClientId);
				});
			}
		}
		else
		{
			// Sleep to avoid busy waiting
			FPlatformProcess::Sleep(0.01f);
		}
	}

	return 0;
}

void FTCPTransport::Stop()
{
	if (!bIsRunning)
	{
		return;
	}

	bIsRunning = false;

	// Wait for accept thread to finish
	if (AcceptThread)
	{
		AcceptThread->WaitForCompletion();
		delete AcceptThread;
		AcceptThread = nullptr;
	}

	// Close all client sockets
	{
		FScopeLock Lock(&ClientMapLock);
		for (TPair<FString, FSocket*>& Pair : ConnectedClients)
		{
			if (Pair.Value)
			{
				Pair.Value->Close();
				ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Pair.Value);
			}
		}
		ConnectedClients.Empty();
	}

	// Close listen socket
	if (ListenSocket)
	{
		ListenSocket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ListenSocket);
		ListenSocket = nullptr;
	}

	UE_LOG(LogTemp, Log, TEXT("MCP TCP Transport stopped"));
}

void FTCPTransport::ReceiveLoop(FSocket* Socket, const FString& ClientId)
{
	TArray<uint8> ReceiveBuffer;
	ReceiveBuffer.SetNum(8192);

	TArray<uint8> MessageBuffer;

	while (bIsRunning && Socket && Socket->GetConnectionState() == ESocketConnectionState::SCS_Connected)
	{
		int32 BytesRead = 0;
		ESocketReceiveFlags::Type RecvFlags = ESocketReceiveFlags::None;

		if (Socket->Recv(ReceiveBuffer.GetData(), ReceiveBuffer.Num(), BytesRead, RecvFlags))
		{
			if (BytesRead > 0)
			{
				// Append to message buffer
				MessageBuffer.Append(ReceiveBuffer.GetData(), BytesRead);

				// Process complete messages (4-byte length prefix + JSON body)
				while (MessageBuffer.Num() >= HEADER_SIZE)
				{
					// Read message length from header (big-endian)
					int32 MessageLength = 0;
					MessageLength |= MessageBuffer[0] << 24;
					MessageLength |= MessageBuffer[1] << 16;
					MessageLength |= MessageBuffer[2] << 8;
					MessageLength |= MessageBuffer[3];

					if (MessageLength <= 0 || MessageLength > 1024 * 1024)  // Max 1MB
					{
						UE_LOG(LogTemp, Warning, TEXT("Invalid message length: %d"), MessageLength);
						break;
					}

					// Check if we have the complete message
					if (MessageBuffer.Num() < HEADER_SIZE + MessageLength)
					{
						break;  // Wait for more data
					}

					// Extract message body
					TArray<uint8> MessageBody;
					MessageBody.Append(MessageBuffer.GetData() + HEADER_SIZE, MessageLength);

					// Remove processed message from buffer
					MessageBuffer.RemoveAt(0, HEADER_SIZE + MessageLength, false);

					// Convert to string and process
					FString Message = UTF8_TO_TCHAR(MessageBody.GetData());
					UE_LOG(LogTemp, Verbose, TEXT("TCP received: %s"), *Message);

					if (OnMessage.IsBound())
					{
						OnMessage.Execute(ClientId, Message);
					}
				}
			}
		}
		else
		{
			// Connection error or disconnected
			uint32 SocketError = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLastErrorCode();
			if (SocketError != SE_NO_ERROR)
			{
				UE_LOG(LogTemp, Warning, TEXT("Socket error: %d"), SocketError);
			}
			break;
		}

		// Small sleep to avoid CPU spinning
		FPlatformProcess::Sleep(0.001f);
	}

	// Cleanup - remove from client map and close socket
	{
		FScopeLock Lock(&ClientMapLock);
		ConnectedClients.Remove(ClientId);
	}

	if (Socket)
	{
		Socket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
	}

	UE_LOG(LogTemp, Log, TEXT("Client disconnected: %s"), *ClientId);

	if (OnClientDisconnected.IsBound())
	{
		OnClientDisconnected.Execute(ClientId);
	}
}

void FTCPTransport::Send(const FString& ClientId, const FString& Message)
{
	if (!bIsRunning)
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot send: not running"));
		return;
	}

	FSocket* ClientSocket = nullptr;
	{
		FScopeLock Lock(&ClientMapLock);
		ClientSocket = ConnectedClients.FindRef(ClientId);
	}

	if (!ClientSocket)
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot send: client %s not found"), *ClientId);
		return;
	}

	if (SendToSocket(ClientSocket, Message))
	{
		UE_LOG(LogTemp, Verbose, TEXT("TCP sent to %s: %s"), *ClientId, *Message);
	}
}

bool FTCPTransport::SendToSocket(FSocket* Socket, const FString& Message)
{
	if (!Socket || Socket->GetConnectionState() != ESocketConnectionState::SCS_Connected)
	{
		return false;
	}

	// Convert message to UTF-8
	FTCHARToUTF8 Convert(*Message);
	int32 MessageLength = Convert.Length();

	// Create buffer: 4-byte length header + message body
	TArray<uint8> SendBuffer;
	SendBuffer.SetNum(HEADER_SIZE + MessageLength);

	// Write length header (big-endian)
	SendBuffer[0] = (MessageLength >> 24) & 0xFF;
	SendBuffer[1] = (MessageLength >> 16) & 0xFF;
	SendBuffer[2] = (MessageLength >> 8) & 0xFF;
	SendBuffer[3] = MessageLength & 0xFF;

	// Copy message body
	if (MessageLength > 0)
	{
		FMemory::Memcpy(SendBuffer.GetData() + HEADER_SIZE, (uint8*)Convert.Get(), MessageLength);
	}

	// Send
	int32 BytesSent = 0;
	return Socket->Send(SendBuffer.GetData(), SendBuffer.Num(), BytesSent);
}
// From Penguin Assistant End
