const net = require('net');

const client = new net.Socket();
let buffer = Buffer.alloc(0);

console.log('[TEST] Starting MCP client test...');

client.on('connect', () => {
    console.log('[TEST] Connected to MCP server on port 20262');
});

client.on('ready', () => {
    console.log('[TEST] Socket ready, sending initialize request...');

    // Step 1: Send initialize request
    const initRequest = JSON.stringify({
        jsonrpc: '2.0',
        id: '1',
        method: 'initialize',
        params: {}
    });

    const length = Buffer.byteLength(initRequest, 'utf8');
    const packet = Buffer.alloc(4 + length);
    packet.writeUInt32BE(length, 0);
    packet.write(initRequest, 4, length, 'utf8');
    client.write(packet);
    console.log('[TEST] Sent initialize request');
});

client.on('data', (data) => {
    buffer = Buffer.concat([buffer, data]);
    console.log('[TEST] Received', data.length, 'bytes, total buffer:', buffer.length);

    // Read complete messages
    while (buffer.length >= 4) {
        const length = buffer.readUInt32BE(0);
        console.log('[TEST] Message length:', length);
        if (buffer.length >= 4 + length) {
            const message = buffer.slice(4, 4 + length).toString('utf8');
            buffer = buffer.slice(4 + length);

            console.log('[TEST] Response:', message);

            // Parse response
            try {
                const response = JSON.parse(message);
                if (response.result && response.result.serverInfo) {
                    console.log('[TEST] ✓ Initialize success, server:', response.result.serverInfo.name);
                }
                if (response.result && response.result.tools) {
                    console.log('[TEST] ✓ Tools list received, count:', response.result.tools.length);
                    console.log('[TEST] Tools:', response.result.tools.map(t => t.name).join(', '));
                }
            } catch (e) {
                console.error('[TEST] Parse error:', e);
            }
        } else {
            console.log('[TEST] Waiting for more data...');
            break;
        }
    }
});

client.on('error', (err) => {
    console.error('[TEST] Error:', err.message);
    process.exit(1);
});

client.on('close', () => {
    console.log('[TEST] Connection closed');
    process.exit(0);
});

client.on('timeout', () => {
    console.log('[TEST] Socket timeout');
});

// Connect with explicit host
console.log('[TEST] Connecting to 127.0.0.1:20262...');
client.connect(20262, '127.0.0.1');

// Send tools/list after 2 seconds
setTimeout(() => {
    if (!client.destroyed && client.readyState === 'open') {
        console.log('[TEST] Sending tools/list request...');
        const request = JSON.stringify({
            jsonrpc: '2.0',
            id: '2',
            method: 'tools/list',
            params: {}
        });

        const length = Buffer.byteLength(request, 'utf8');
        const packet = Buffer.alloc(4 + length);
        packet.writeUInt32BE(length, 0);
        packet.write(request, 4, length, 'utf8');
        client.write(packet);
    }
}, 2000);

// Close after 6 seconds
setTimeout(() => {
    console.log('[TEST] Test completed');
    client.destroy();
    process.exit(0);
}, 6000);