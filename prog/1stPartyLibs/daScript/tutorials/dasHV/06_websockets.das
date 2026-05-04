// Tutorial HV-06: WebSockets — Client and Server
//
// This tutorial covers:
//   - Creating a WebSocket server with HvWebServer (onWsOpen, onWsClose, onWsMessage)
//   - Creating a WebSocket client with HvWebSocketClient (onOpen, onClose, onMessage)
//   - Sending text messages (client → server, server → client)
//   - Broadcasting messages to all connected clients
//   - Tracking connected clients with a channel table
//   - Mixing HTTP routes with WebSocket endpoints on the same server
//   - Binary messages and ws_opcode
//
// Prerequisites: Tutorial HV-03 (HTTP server basics)
//
// The tutorial starts a WebSocket-capable server, connects clients to it,
// and demonstrates bidirectional message exchange.
//
// Run: daslang.exe tutorials/dasHV/06_websockets.das

options gen2
options persistent_heap
options gc

require dashv/dashv_boost public
require daslib/jobque_boost
require daslib/fio

let SERVER_PORT = 18085

// ──────────────────────────────────────────────────────────────────────────
// Section 1 — WebSocket Server
// ──────────────────────────────────────────────────────────────────────────
//
// HvWebServer already supports WebSocket connections. Override:
//   onWsOpen(channel, url)    — called when a client connects
//   onWsClose(channel)        — called when a client disconnects
//   onWsMessage(channel, msg) — called when a text message arrives
//
// The server can also register HTTP routes alongside WebSocket handlers.

class ChatServer : HvWebServer {
    // Track connected clients: channel → nickname
    clients : table<WebSocketChannel?; string>
    next_id : int = 0

    def override onInit {
        // HTTP health-check route — works alongside WebSocket
        GET("/ping") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            return resp |> TEXT_PLAIN("pong")
        }

        // HTTP route to check how many WS clients are connected
        GET("/clients") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            return resp |> TEXT_PLAIN("{length(self.clients)}")
        }
    }

    def override onWsOpen(channel : WebSocketChannel?; url : string#) {
        next_id++
        let nickname = "user_{next_id}"
        clients |> insert_clone(channel, nickname)
        print("[server] {nickname} connected (url={url}, total={length(clients)})\n")
        // Welcome the new client
        send(channel, "welcome {nickname}", ws_opcode.WS_OPCODE_TEXT, true)
        // Notify others
        broadcast("{nickname} joined", channel)
    }

    def override onWsClose(channel : WebSocketChannel?) {
        let nickname = clients?[channel] ?? "unknown"
        print("[server] {nickname} disconnected\n")
        clients |> erase(channel)
        broadcast("{nickname} left", null)
    }

    def override onWsMessage(channel : WebSocketChannel?; msg : string#) {
        let nickname = clients?[channel] ?? "unknown"
        print("[server] {nickname}: {msg}\n")
        // Echo with prefix to the sender
        send(channel, "echo: {msg}", ws_opcode.WS_OPCODE_TEXT, true)
        // Broadcast to everyone else
        broadcast("{nickname}: {msg}", channel)
    }

    // Send a message to all connected clients except `exclude`
    def broadcast(msg : string; exclude : WebSocketChannel?) {
        for (ch in keys(clients)) {
            if (ch != exclude) {
                send(ch, msg, ws_opcode.WS_OPCODE_TEXT, true)
            }
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Section 2 — WebSocket Client
// ──────────────────────────────────────────────────────────────────────────
//
// Subclass HvWebSocketClient and override:
//   onOpen()        — connection established
//   onClose()       — connection lost
//   onMessage(msg)  — text message received
//
// Call init(url) to connect, send(text) to send, process_event_que() to
// pump the event queue, and is_connected() to check state.

class ChatClient : HvWebSocketClient {
    name : string
    received : array<string>

    def override onOpen {
        print("[{name}] connected\n")
    }

    def override onClose {
        print("[{name}] disconnected\n")
    }

    def override onMessage(msg : string#) {
        print("[{name}] received: {msg}\n")
        received |> push_clone(string(msg))
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Section 3 — Server lifecycle helper
// ──────────────────────────────────────────────────────────────────────────
//
// The server runs on a dedicated thread. We use with_job_status for
// synchronization and with_atomic32 for the stop signal.

def with_ws_server(port : int; blk : block<(base_url : string) : void>) {
    with_job_status(1) $(started) {
        with_job_status(1) $(finished) {
            with_atomic32() $(stop_flag) {
                new_thread() @() {
                    var server = new ChatServer()
                    server->init(port)
                    server->start()
                    started |> notify_and_release
                    while (stop_flag |> get == 0) {
                        server->tick()
                        sleep(10u)
                    }
                    server->stop()
                    unsafe {
                        delete server
                    }
                    finished |> notify_and_release
                }
                started |> join
                let base_url = "ws://127.0.0.1:{port}"
                invoke(blk, base_url)
                stop_flag |> set(1)
                finished |> join
            }
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Helpers
// ──────────────────────────────────────────────────────────────────────────

// Pump client event queues and let the server tick
def pump(var clients : array<ChatClient?>) {
    for (c in clients) {
        c->process_event_que()
    }
    sleep(50u)
    for (c in clients) {
        c->process_event_que()
    }
}

// Wait for a client to have at least `count` messages
def wait_for_messages(var client : ChatClient?; count : int; max_ms : int = 2000) {
    var elapsed = 0
    while (length(client.received) < count && elapsed < max_ms) {
        client->process_event_que()
        sleep(20u)
        elapsed += 20
    }
}

[export]
def main() {
    with_ws_server(SERVER_PORT) <| $(base_url) {

        // ── Section 2a: Single client connection ──
        print("=== Section 2a: Single client ===\n")

        var alice = new ChatClient()
        alice.name = "alice"
        alice->init("{base_url}/chat")

        // Wait for connection and welcome message
        wait_for_messages(alice, 1)
        print("  alice received {length(alice.received)} message(s)\n")
        if (length(alice.received) > 0) {
            print("  welcome: {alice.received[0]}\n")
        }

        // ── Section 2b: Sending a message ──
        print("\n=== Section 2b: Sending messages ===\n")

        alice->send("hello server!")
        wait_for_messages(alice, 2)
        if (length(alice.received) > 1) {
            print("  echo: {alice.received[1]}\n")
        }

        // ── Section 2c: Multiple clients ──
        print("\n=== Section 2c: Multiple clients ===\n")

        var bob = new ChatClient()
        bob.name = "bob"
        bob->init("{base_url}/chat")

        // Wait for bob's welcome + alice's notification of bob joining
        wait_for_messages(bob, 1)
        sleep(100u)
        alice->process_event_que()

        print("  bob connected, alice has {length(alice.received)} messages\n")

        // ── Section 2d: Broadcasting ──
        print("\n=== Section 2d: Broadcasting ===\n")

        bob->send("hi everyone!")
        // Bob gets echo, alice gets broadcast
        wait_for_messages(bob, 2)
        wait_for_messages(alice, length(alice.received) + 1)

        print("  bob messages: {length(bob.received)}\n")
        print("  alice messages: {length(alice.received)}\n")

        // Show all messages received
        print("\n  alice's message log:\n")
        for (msg in alice.received) {
            print("    - {msg}\n")
        }
        print("\n  bob's message log:\n")
        for (msg in bob.received) {
            print("    - {msg}\n")
        }

        // ── Section 2e: HTTP still works alongside WebSocket ──
        print("\n=== Section 2e: HTTP alongside WebSocket ===\n")

        GET("http://127.0.0.1:{SERVER_PORT}/clients") <| $(resp) {
            if (resp != null) {
                print("  connected clients: {resp.body}\n")
            }
        }

        GET("http://127.0.0.1:{SERVER_PORT}/ping") <| $(resp) {
            if (resp != null) {
                print("  ping: {resp.body}\n")
            }
        }

        // ── Section 2f: Graceful disconnect ──
        print("\n=== Section 2f: Graceful close ===\n")

        // close() sends a WebSocket close frame, triggering onClose on both sides.
        // After close(), is_connected() returns false and the server's onWsClose fires.
        alice->close()
        bob->close()

        // Pump until onClose fires
        var elapsed = 0
        while ((alice->is_connected() || bob->is_connected()) && elapsed < 2000) {
            alice->process_event_que()
            bob->process_event_que()
            sleep(20u)
            elapsed += 20
        }
        print("  alice connected: {alice->is_connected()}\n")
        print("  bob connected: {bob->is_connected()}\n")

        unsafe {
            delete alice
            delete bob
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Quick reference
// ──────────────────────────────────────────────────────────────────────────
//
//  Server (HvWebServer overrides):
//    onWsOpen(channel, url)       Called when a WebSocket client connects
//    onWsClose(channel)           Called when a WebSocket client disconnects
//    onWsMessage(channel, msg)    Called when a text message arrives
//    onTick()                     Called each server tick iteration
//    onInit()                     Called during init() — register routes here
//
//  Server send:
//    send(channel, msg, opcode, fin)     Send text/binary to one client
//    send(channel, buf, len, opcode, fin) Send raw buffer
//
//  Client (HvWebSocketClient overrides):
//    onOpen()                     Called when connection is established
//    onClose()                    Called when connection is lost
//    onMessage(msg)               Called when a text message is received
//
//  Client API:
//    init(url) : int              Connect to WebSocket server
//    send(text)                   Send a text message
//    close() : int                Graceful disconnect (sends close frame)
//    is_connected() : bool        Check connection state
//    process_event_que()          Pump the event queue (call regularly)
//
//  ws_opcode values:
//    WS_OPCODE_TEXT               Text frame (UTF-8)
//    WS_OPCODE_BINARY             Binary frame
//    WS_OPCODE_CLOSE              Close frame
//    WS_OPCODE_PING               Ping frame
//    WS_OPCODE_PONG               Pong frame
//    WS_OPCODE_CONTINUE           Continuation frame
