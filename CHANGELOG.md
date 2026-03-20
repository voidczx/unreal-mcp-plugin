# Changelog

All notable changes to this project will be documented in this file.

---

## 2026-03-20 - TCP Transport Implementation

### Changes

Replaced HTTP transport with TCP transport:

- **Removed**: HTTPTransport
- **Added**: TCPTransport, supports multiple client connections
- **Added**: 4-byte length prefix for message framing
- **Changed**: Console command renamed to `MCP.StartTCP`

### Client Script Update

Created `MCPScript` directory for client scripts:

- `MCPScript/mcp-server.js` - TCP mode MCP bridge server
- `--tcp` mode: Starts TCP server, Claude Code connects to port 20261, bridge connects to Unreal Editor port 20260
- Default mode (stdio): Maintains original stdio mode compatibility

### Claude Code Configuration Update

Updated `C:\Users\Admin\.claude\settings.json`:

- Changed `mcpServers.unreal-editor.args` to `["--tcp"]`
- Updated script path to `MCPScript/mcp-server.js`

### Build Fixes

Fixed TCPTransport.cpp compilation errors:

- Used `FTcpSocketBuilder` instead of `FUdpSocketBuilder`
- Used `FIPv4Address` instead of `IPv4Address`
- Used `FRunnable` interface for accept thread
- Used `TMap<FString, FSocket*>` for managing multiple client connections
- Added `Sockets` and `Networking` module dependencies

---

## 2026-03-20 - Asset Service Implementation

### Changes

Implemented `FMCPAssetService` asset operations:

| Method | Description |
|--------|-------------|
| `ListAssets` | List assets using IAssetRegistry |
| `GetAsset` | Get asset data |
| `GetAssetMetadata` | Get asset metadata (package name, class, path, tags) |
| `GetAssetContent` | Get asset content |
| `CreateAsset` | Create asset using IAssetTools |
| `DuplicateAsset` | Duplicate asset |
| `DeleteAsset` | Delete asset using ObjectTools::DeleteObjects |
| `RenameAsset` | Rename asset |
| `ImportAsset` | Import asset |
| `ExportAsset` | Export asset |
| `GetDependencies` | Get dependencies |
| `GetReferencers` | Get referencers |

---

## 2026-03-19 - Initial Release

### Changes

Created MCP plugin with the following components:

### Plugin Configuration
- `MCP.uplugin` - Plugin descriptor file

### Build Files
- `MCP.Build.cs` - Build configuration

### Public Headers
- `IMCPModule.h` - Module interface
- `IMCPServer.h` - Server interface
- `IMCPService.h` - Service base class interface
- `MCPMessage.h` - MCP message definitions
- `IMCPAssetService.h` - Asset service interface
- `IMCPCommandService.h` - Command service interface
- `IMCPEditorService.h` - Editor service interface

### Private Implementation
- `MCPModule.cpp` - Module implementation
- `MCPServer.h` / `MCPServer.cpp` - Server implementation
- `StdioTransport.h` / `StdioTransport.cpp` - Stdio transport
- `TCPTransport.h` / `TCPTransport.cpp` - TCP transport (placeholder)
- `MCPAssetService.h` / `MCPAssetService.cpp` - Asset service
- `MCPCommandService.h` / `MCPCommandService.cpp` - Command service
- `MCPEditorService.h` / `MCPEditorService.cpp` - Editor service

### Configuration Changes

- Port changed from 8080 to 20260 (to avoid conflicts)
- `ProcessMessage` function fully implemented, supports initialize, tools/list, tools/call, ping methods