// WebSocket Chat Server
//
// A chat server that:
//   1. Serves chat.html at http://localhost:9090/
//   2. Accepts WebSocket connections at ws://localhost:9090/chat
//   3. Broadcasts messages to all connected clients
//   4. Shows join/leave notifications
//
// Usage:
//   daslang.exe examples/hv/ws_chat_server.das
//
// Then open http://localhost:9090 in your browser,
// or connect with the companion ws_chat_client.das from a terminal.

options gen2
options persistent_heap
options gc

require dashv/dashv_boost public
require daslib/fio
require strings

let PORT = 9090

// ── Chat server ──────────────────────────────────────────────────────────

class ChatServer : HvWebServer {
    clients : table<WebSocketChannel?; string>
    next_id : int = 0

    def override onInit {
        // Serve files from examples/hv/, with chat.html as the home page
        let static_dir = "{get_das_root()}/examples/hv"
        set_document_root(static_dir)
        set_home_page("chat.html")
        // Health check
        GET("/ping") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            return resp |> TEXT_PLAIN("pong")
        }
        // Connected client count (JSON)
        GET("/status") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            return resp |> JSON("\{\"clients\": {length(self.clients)}\}")
        }
    }

    def override onWsOpen(channel : WebSocketChannel?; url : string#) {
        next_id++
        let nickname = "guest_{next_id}"
        clients |> insert_clone(channel, nickname)
        send(channel, "[server] Welcome! You are {nickname}. Type /nick <name> to change your name.", ws_opcode.WS_OPCODE_TEXT, true)
        broadcast("[server] {nickname} joined the chat ({length(clients)} online)", channel)
        print("[+] {nickname} connected ({length(clients)} total)\n")
    }

    def override onWsClose(channel : WebSocketChannel?) {
        let nickname = clients?[channel] ?? "unknown"
        clients |> erase(channel)
        broadcast("[server] {nickname} left the chat ({length(clients)} online)", null)
        print("[-] {nickname} disconnected ({length(clients)} total)\n")
    }

    def override onWsMessage(channel : WebSocketChannel?; msg : string#) {
        let nickname = clients?[channel] ?? "unknown"
        let text = string(msg)
        // Handle /nick command
        if (starts_with(text, "/nick ")) {
            let new_name = slice(text, 6)
            if (length(new_name) > 0 && length(new_name) <= 20) {
                let old_name = nickname
                clients |> erase(channel)
                clients |> insert_clone(channel, new_name)
                send(channel, "[server] You are now {new_name}", ws_opcode.WS_OPCODE_TEXT, true)
                broadcast("[server] {old_name} is now {new_name}", channel)
                print("[*] {old_name} -> {new_name}\n")
            } else {
                send(channel, "[server] Nickname must be 1-20 characters", ws_opcode.WS_OPCODE_TEXT, true)
            }
            return
        }
        // Regular message
        send(channel, "[you] {text}", ws_opcode.WS_OPCODE_TEXT, true)
        broadcast("[{nickname}] {text}", channel)
        print("[{nickname}] {text}\n")
    }

    def broadcast(msg : string; exclude : WebSocketChannel?) {
        for (ch in keys(clients)) {
            if (ch != exclude) {
                send(ch, msg, ws_opcode.WS_OPCODE_TEXT, true)
            }
        }
    }
}

// ── Main ─────────────────────────────────────────────────────────────────

[export]
def main() {
    print("Starting chat server on port {PORT}...\n")
    print("  Open http://localhost:{PORT} in your browser\n")
    print("  Or run: daslang.exe examples/hv/ws_chat_client.das\n")
    print("  Press Ctrl+C to stop\n\n")
    var server = new ChatServer()
    server->init(PORT)
    server->start()
    while (true) {
        server->tick()
        sleep(10u)
    }
    server->stop()
    unsafe {
        delete server
    }
}
