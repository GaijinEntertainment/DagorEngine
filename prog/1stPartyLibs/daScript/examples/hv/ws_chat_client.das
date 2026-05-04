// WebSocket Chat Client (terminal)
//
// Connects to the chat server and lets you type messages in the terminal.
// Messages from other users appear inline.
//
// Usage:
//   1. Start the server:  daslang.exe examples/hv/ws_chat_server.das
//   2. Run this client:   daslang.exe examples/hv/ws_chat_client.das
//   3. Type messages and press Enter. Type /nick <name> to change your name.
//   4. Type /quit to exit.
//
// You can run multiple instances of this client alongside browser tabs
// connected to http://localhost:9090.

options gen2
options persistent_heap
options gc

require dashv/dashv_boost public
require strings
require daslib/fio

let SERVER_URL = "ws://127.0.0.1:9090/chat"

class TerminalChatClient : HvWebSocketClient {
    def override onOpen {
        print("Connected to {SERVER_URL}\n")
        print("Type a message and press Enter. Commands: /nick <name>, /quit\n\n")
    }
    def override onClose {
        print("\nDisconnected from server.\n")
    }
    def override onMessage(msg : string#) {
        print("{msg}\n")
    }
}

[export]
def main() {
    var client = new TerminalChatClient()
    let rc = client->init(SERVER_URL)
    if (rc != 0) {
        print("Failed to connect to {SERVER_URL}\n")
        return
    }
    // Main loop: read stdin + pump WebSocket events
    while (client->is_connected()) {
        client->process_event_que()
        // Non-blocking read from stdin
        let line = fgets(fstdin())
        if (length(line) > 0) {
            let text = strip(line)
            if (text == "/quit") {
                client->close()
                break
            }
            if (length(text) > 0) {
                client->send(text)
            }
        }
        sleep(10u)
    }
    // Drain remaining events
    client->process_event_que()
    unsafe {
        delete client
    }
}
