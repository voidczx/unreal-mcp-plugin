# Unreal MCP Plugin

MCP (Model Context Protocol) plugin for Unreal Editor - enables AI agents to interact with Unreal Editor for asset operations, command execution, and editor automation.

## Architecture

```
Claude Code <--TCP:20261--> mcp-server.js (Bridge) <--TCP:20260--> Unreal Editor (MCP Plugin)
```

### Transport Layer

| Transport | Description |
|-----------|-------------|
| **StdioTransport** | Standard input/output JSON-RPC communication (MCP standard mode) |
| **TCPTransport** | TCP socket-based communication, supports multiple clients, port 20260 |

TCP Transport uses 4-byte length prefix for message framing.

## Tools

### Asset Service

| Tool | Description |
|------|-------------|
| `list_assets` | List assets under a path |
| `get_asset` | Get asset metadata |
| `get_dependencies` | Get asset dependencies |
| `get_referencers` | Get asset referencers |
| `create_asset` | Create a new asset |
| `duplicate_asset` | Duplicate an asset |
| `delete_asset` | Delete an asset |
| `rename_asset` | Rename an asset |

### Command Service

| Tool | Description |
|------|-------------|
| `execute_command` | Execute console command |
| `execute_python` | Execute Python script |

### Editor Service

| Tool | Description |
|------|-------------|
| `save_asset` | Save asset to disk |
| `compile_blueprint` | Compile blueprint |

## Usage

1. Build the project, MCP plugin will load automatically
2. Start MCP server in editor via console command:
   - `MCP.Start` - Start in stdio mode
   - `MCP.StartTCP` - Start in TCP mode (port 20260)
3. AI Agent connects via TCP protocol and sends MCP requests

### Client Script

The `MCPScript/mcp-server.js` bridges Claude Code (20261) and Unreal Editor (20260).

```bash
# TCP mode
node mcp-server.js --tcp

# Stdio mode (default)
node mcp-server.js
```

## Dependencies

- Core, CoreUObject, Engine
- Json, JsonUtilities
- Sockets, SocketSubsystem
- UnrealEd, AssetTools, AssetRegistry
- ContentBrowser, EditorStyle, Slate, SlateCore