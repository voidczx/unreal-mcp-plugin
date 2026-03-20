#!/usr/bin/env node
/**
 * MCP Server Bridge for Unreal Editor
 * Bridges Claude Code MCP client to Unreal Editor MCP plugin via TCP
 *
 * Usage:
 *   node mcp-server.js              # stdio mode (default)
 *   node mcp-server.js --tcp        # TCP mode (connect to UE)
 *   node mcp-server.js --tcp --port 20262  # Custom port for Claude Code
 */

const net = require('net');
const readline = require('readline');

const DEFAULT_UE_PORT = 20260;
const DEFAULT_UE_HOST = 'localhost';
const DEFAULT_MCP_PORT = 20261;  // Port for Claude Code to connect

let ueHost = process.env.UE_HOST || DEFAULT_UE_HOST;
let uePort = parseInt(process.env.UE_PORT || String(DEFAULT_UE_PORT), 10);
let mcpPort = parseInt(process.env.MCP_PORT || String(DEFAULT_MCP_PORT), 10);

// Parse command line arguments
const args = process.argv.slice(2);
const isTcpMode = args.includes('--tcp');
const portArgIndex = args.indexOf('--port');
if (portArgIndex !== -1 && args[portArgIndex + 1]) {
    mcpPort = parseInt(args[portArgIndex + 1], 10);
}

// Request ID counter
let requestId = 1;

// TCP client connection to Unreal Editor
let ueSocket = null;

/**
 * Connect to Unreal Editor MCP plugin via TCP
 */
function connectToUnrealEditor() {
    return new Promise((resolve, reject) => {
        ueSocket = net.createConnection({ host: ueHost, port: uePort }, () => {
            console.error(`Connected to Unreal Editor at ${ueHost}:${uePort}`);
            resolve();
        });

        ueSocket.on('error', (err) => {
            console.error(`Failed to connect to Unreal Editor: ${err.message}`);
            reject(err);
        });

        // Handle incoming data from Unreal Editor
        let ueBuffer = Buffer.alloc(0);
        ueSocket.on('data', (data) => {
            // Append new data to buffer
            ueBuffer = Buffer.concat([ueBuffer, data]);

            // Process complete messages (4-byte length prefix + JSON)
            while (ueBuffer.length >= 4) {
                const length = ueBuffer.readUInt32BE(0);
                if (ueBuffer.length >= 4 + length) {
                    const message = ueBuffer.slice(4, 4 + length).toString('utf8');
                    ueBuffer = ueBuffer.slice(4 + length);
                    handleResponse(message);
                } else {
                    break;
                }
            }
        });

        ueSocket.on('close', () => {
            console.error('Connection to Unreal Editor closed');
            ueSocket = null;
        });
    });
}

/**
 * Send request to Unreal Editor MCP plugin via TCP
 */
function sendToUnrealEditor(jsonRpcMessage) {
    return new Promise((resolve, reject) => {
        if (!ueSocket || ueSocket.destroyed) {
            reject(new Error('Not connected to Unreal Editor'));
            return;
        }

        const jsonStr = JSON.stringify(jsonRpcMessage);
        const length = Buffer.byteLength(jsonStr, 'utf8');

        // Create buffer: 4-byte length header + message body
        const buffer = Buffer.alloc(4 + length);
        buffer.writeUInt32BE(length, 0);
        buffer.write(jsonStr, 4, length, 'utf8');

        // Store pending request
        const requestId = jsonRpcMessage.id;
        pendingRequests.set(requestId, { resolve, reject });

        ueSocket.write(buffer, (err) => {
            if (err) {
                pendingRequests.delete(requestId);
                reject(err);
            }
        });
    });
}

// Pending requests waiting for response
const pendingRequests = new Map();

/**
 * Handle response from Unreal Editor
 */
function handleResponse(message) {
    try {
        const response = JSON.parse(message);
        const id = response.id;

        if (pendingRequests.has(id)) {
            const { resolve } = pendingRequests.get(id);
            pendingRequests.delete(id);
            resolve(response);
        }
    } catch (e) {
        console.error('Failed to parse response:', e);
    }
}

/**
 * Handle MCP initialize request
 */
async function handleInitialize(params) {
    return {
        jsonrpc: "2.0",
        id: params.id,
        result: {
            protocolVersion: "2024-11-05",
            capabilities: {
                tools: {}
            },
            serverInfo: {
                name: "unreal-editor-mcp",
                version: "1.0.0"
            }
        }
    };
}

/**
 * Handle tools/list request
 */
async function handleToolsList() {
    try {
        const response = await sendToUnrealEditor({
            jsonrpc: "2.0",
            id: String(requestId++),
            method: 'tools/list',
            params: {}
        });
        return {
            jsonrpc: "2.0",
            id: String(requestId - 1),
            result: response.result
        };
    } catch (e) {
        // Return default tools if Unreal Editor is not connected
        return {
            jsonrpc: "2.0",
            id: String(requestId - 1),
            result: {
                tools: [
                    { name: "list_assets", description: "List all assets in a directory path" },
                    { name: "get_asset", description: "Get asset metadata by path" },
                    { name: "get_dependencies", description: "Get asset dependencies" },
                    { name: "get_referencers", description: "Get assets that reference this asset" },
                    { name: "create_asset", description: "Create a new asset" },
                    { name: "duplicate_asset", description: "Duplicate an existing asset" },
                    { name: "delete_asset", description: "Delete an asset" },
                    { name: "rename_asset", description: "Rename an asset" },
                    { name: "execute_command", description: "Execute an editor console command" },
                    { name: "execute_python", description: "Execute a Python script" },
                    { name: "save_asset", description: "Save an asset" },
                    { name: "compile_blueprint", description: "Compile a blueprint" }
                ]
            }
        };
    }
}

/**
 * Handle tools/call request
 */
async function handleToolsCall(params) {
    const { name, arguments: toolArgs } = params;

    try {
        const response = await sendToUnrealEditor({
            jsonrpc: "2.0",
            id: params.id,
            method: 'tools/call',
            params: {
                name: name,
                arguments: toolArgs
            }
        });
        return response;
    } catch (e) {
        return {
            jsonrpc: "2.0",
            id: params.id,
            error: {
                code: -32603,
                message: e.message || 'Internal error'
            }
        };
    }
}

/**
 * Process incoming JSON-RPC message
 */
async function processMessage(message) {
    try {
        const request = JSON.parse(message);
        const { jsonrpc, id, method, params } = request;

        if (jsonrpc !== "2.0") {
            return JSON.stringify({ error: { code: -32600, message: "Invalid Request" } });
        }

        let response;
        switch (method) {
            case "initialize":
                response = await handleInitialize({ id, ...params });
                break;
            case "tools/list":
                response = await handleToolsList();
                break;
            case "tools/call":
                response = await handleToolsCall({ id, ...params });
                break;
            case "ping":
                response = { jsonrpc: "2.0", id, result: {} };
                break;
            default:
                response = { jsonrpc: "2.0", id, error: { code: -32601, message: "Method not found" } };
        }

        return JSON.stringify(response);
    } catch (e) {
        return JSON.stringify({ error: { code: -32700, message: "Parse error" } });
    }
}

/**
 * Stdio mode - read from stdin, write to stdout
 */
async function runStdioMode() {
    const rl = readline.createInterface({
        input: process.stdin,
        output: process.stdout,
        terminal: false
    });

    rl.on('line', async (line) => {
        if (line.trim() === '') return;

        // Handle JSON-RPC messages (each line is a complete message in stdio mode)
        const response = await processMessage(line);
        if (response) {
            console.log(response);
        }
    });

    process.stdin.resume();
}

/**
 * TCP mode - start TCP server for Claude Code to connect
 * Also connects to Unreal Editor via TCP
 */
async function runTcpMode() {
    // First connect to Unreal Editor
    try {
        await connectToUnrealEditor();
    } catch (e) {
        console.error(`Warning: Could not connect to Unreal Editor at ${ueHost}:${uePort}`);
        console.error('The server will run but tool calls will return default responses');
    }

    // Start TCP server for Claude Code to connect
    const server = net.createServer(async (socket) => {
        console.error(`Client connected: ${socket.remoteAddress}:${socket.remotePort}`);

        let clientBuffer = Buffer.alloc(0);
        socket.on('data', async (data) => {
            // Append new data to buffer
            clientBuffer = Buffer.concat([clientBuffer, data]);

            // Process complete messages (4-byte length prefix + JSON)
            while (clientBuffer.length >= 4) {
                const length = clientBuffer.readUInt32BE(0);
                if (clientBuffer.length >= 4 + length) {
                    const message = clientBuffer.slice(4, 4 + length).toString('utf8');
                    clientBuffer = clientBuffer.slice(4 + length);

                    const response = await processMessage(message);

                    // Send response with length prefix
                    const responseBuffer = Buffer.alloc(4 + Buffer.byteLength(response, 'utf8'));
                    responseBuffer.writeUInt32BE(Buffer.byteLength(response, 'utf8'), 0);
                    responseBuffer.write(response, 4, 'utf8');
                    socket.write(responseBuffer);
                } else {
                    break;
                }
            }
        });

        socket.on('close', () => {
            console.error(`Client disconnected: ${socket.remoteAddress}:${socket.remotePort}`);
        });

        socket.on('error', (err) => {
            console.error(`Socket error: ${err.message}`);
        });
    });

    server.listen(mcpPort, () => {
        console.error(`MCP Server Bridge running on port ${mcpPort}`);
        console.error(`Claude Code should connect to localhost:${mcpPort}`);
        if (ueSocket && !ueSocket.destroyed) {
            console.error(`Connected to Unreal Editor at ${ueHost}:${uePort}`);
        } else {
            console.error(`Not connected to Unreal Editor (${ueHost}:${uePort})`);
        }
    });
}

// Main entry point
if (isTcpMode) {
    runTcpMode();
} else {
    runStdioMode();
}