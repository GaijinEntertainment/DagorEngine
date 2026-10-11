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
    def override onMessageFrame(msg : string#; byte_count : int; opcode : ws_opcode) {
        if (opcode == ws_opcode.WS_OPCODE_TEXT) onMessage(msg)
    }
}

let CONNECT_TIMEOUT_MS = 3000
let POLL_STEP_MS = 10

[export]
def main() : int {
    var client = new TerminalChatClient()
    let init_ok = client->init(SERVER_URL) == 0
    // init() only starts the handshake — pump until connected or the window elapses;
    // otherwise a missing server looks like a clean exit.
    var waited = 0
    while (init_ok && !client->is_connected() && waited < CONNECT_TIMEOUT_MS) {
        client->process_event_que()
        sleep(uint(POLL_STEP_MS))
        waited += POLL_STEP_MS
    }
    if (!init_ok || !client->is_connected()) {
        print("Failed to connect to {SERVER_URL} — is the server running? (daslang examples/hv/ws_chat_server.das)\n")
        client->cleanup()
        unsafe {
            delete client
        }
        return 1
    }
    // Main loop: read stdin + pump WebSocket events
    while (client->is_connected()) {
        client->process_event_que()
        // Non-blocking read from stdin
        let line = fgets(fstdin())
        if (!empty(line)) {
            let text = strip(line)
            if (text == "/quit") {
                client->close()
                break
            }
            if (!empty(text)) {
                client->send(text)
            }
        }
        sleep(10u)
    }
    // Drain remaining events
    client->process_event_que()
    client->cleanup()
    unsafe {
        delete client
    }
    return 0
}
