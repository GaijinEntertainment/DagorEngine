// Tests for WebSocket client and server (Tutorial HV-06)
//
// Covers: HvWebSocketClient, HvWebServer WebSocket callbacks,
//         connect/send/receive, broadcast, multiple clients,
//         HTTP+WebSocket coexistence, ws_opcode

options gen2
options persistent_heap
options gc
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require dastest/testing_boost public
require _dashv_test_common
require strings

// ──────────────────────────────────────────────────────────────────────────
// WebSocket server for tests
// ──────────────────────────────────────────────────────────────────────────

class WsTestServer : HvWebServer {
    clients : table<WebSocketChannel?; string>
    next_id : int = 0

    def override onInit {
        GET("/ping") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            return resp |> TEXT_PLAIN("pong")
        }
        GET("/ws-clients") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
            return resp |> TEXT_PLAIN("{length(self.clients)}")
        }
    }

    def override onWsOpen(channel : WebSocketChannel?; url : string#) {
        next_id++
        let nickname = "user_{next_id}"
        // Notify existing clients before inserting the new one
        for (ch in keys(clients)) {
            send(ch, "{nickname} joined", ws_opcode.WS_OPCODE_TEXT, true)
        }
        clients |> insert_clone(channel, nickname)
        send(channel, "welcome {nickname}", ws_opcode.WS_OPCODE_TEXT, true)
    }

    def override onWsClose(channel : WebSocketChannel?) {
        let nickname = clients?[channel] ?? "unknown"
        clients |> erase(channel)
        // Notify remaining clients
        for (ch in keys(clients)) {
            send(ch, "{nickname} left", ws_opcode.WS_OPCODE_TEXT, true)
        }
    }

    def override onWsMessage(channel : WebSocketChannel?; msg : string#) {
        let nickname = clients?[channel] ?? "unknown"
        // Echo back to sender
        send(channel, "echo: {msg}", ws_opcode.WS_OPCODE_TEXT, true)
        // Broadcast to others
        for (ch in keys(clients)) {
            if (ch != channel) {
                send(ch, "{nickname}: {msg}", ws_opcode.WS_OPCODE_TEXT, true)
            }
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Test WebSocket client
// ──────────────────────────────────────────────────────────────────────────

class TestWsClient : HvWebSocketClient {
    received : array<string>
    connected : bool = false
    closed : bool = false

    def override onOpen {
        connected = true
    }

    def override onClose {
        connected = false
        closed = true
    }

    def override onMessage(msg : string#) {
        received |> push_clone(string(msg))
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Helper: wait for a client to accumulate N messages
// ──────────────────────────────────────────────────────────────────────────

def wait_messages(var client : TestWsClient?; count : int; max_ms : int = 3000) {
    var elapsed = 0
    while (length(client.received) < count && elapsed < max_ms) {
        client->process_event_que()
        sleep(1u)
        elapsed += 1
    }
}

// Wait until client.closed becomes true (onClose has fired)
def wait_closed(var client : TestWsClient?; max_ms : int = 3000) {
    var elapsed = 0
    while (!client.closed && elapsed < max_ms) {
        client->process_event_que()
        sleep(1u)
        elapsed += 1
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Server lifecycle helper for WS tests
// ──────────────────────────────────────────────────────────────────────────

def with_ws_test_server(port : int; blk : block<(ws_url : string; http_url : string) : void>) {
    with_job_status(1) $(started) {
        with_job_status(1) $(finished) {
            with_atomic32() $(stop_flag) {
                new_thread() @() {
                    var server = new WsTestServer()
                    server->init(port)
                    server->start()
                    started |> notify_and_release
                    while (stop_flag |> get == 0) {
                        server->tick()
                        sleep(1u)
                    }
                    server->stop()
                    unsafe {
                        delete server
                    }
                    finished |> notify_and_release
                }
                started |> join
                let ws_url = "ws://127.0.0.1:{port}"
                let http_url = "http://127.0.0.1:{port}"
                invoke(blk, ws_url, http_url)
                stop_flag |> set(1)
                finished |> join
            }
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Tests
// ──────────────────────────────────────────────────────────────────────────

[test]
def test_websockets(t : T?) {
    with_ws_test_server(PORT_WEBSOCKETS) <| $(ws_url, http_url) {

        t |> run("connect and receive welcome") @(t : T?) {
            var client = new TestWsClient()
            client->init("{ws_url}/ws")
            wait_messages(client, 1)
            t |> success(client.connected, "client is connected")
            t |> equal(length(client.received), 1)
            t |> success(starts_with(client.received[0], "welcome "), "welcome message received")
            client->close()
            wait_closed(client)
            unsafe {
                delete client
            }
        }

        t |> run("send and receive echo") @(t : T?) {
            var client = new TestWsClient()
            client->init("{ws_url}/ws")
            wait_messages(client, 1)  // welcome
            client->send("hello")
            wait_messages(client, 2)  // welcome + echo
            t |> equal(length(client.received), 2)
            t |> equal(client.received[1], "echo: hello")
            client->close()
            wait_closed(client)
            unsafe {
                delete client
            }
        }

        t |> run("multiple messages in sequence") @(t : T?) {
            var client = new TestWsClient()
            client->init("{ws_url}/ws")
            wait_messages(client, 1)  // welcome
            client->send("msg1")
            client->send("msg2")
            client->send("msg3")
            wait_messages(client, 4)  // welcome + 3 echoes
            t |> equal(length(client.received), 4)
            t |> equal(client.received[1], "echo: msg1")
            t |> equal(client.received[2], "echo: msg2")
            t |> equal(client.received[3], "echo: msg3")
            client->close()
            wait_closed(client)
            unsafe {
                delete client
            }
        }

        t |> run("two clients receive broadcasts") @(t : T?) {
            var alice = new TestWsClient()
            alice->init("{ws_url}/ws")
            wait_messages(alice, 1)  // welcome

            var bob = new TestWsClient()
            bob->init("{ws_url}/ws")
            wait_messages(bob, 1)  // welcome for bob

            // let alice receive the join notification
            wait_messages(alice, 2)

            // bob sends a message — alice should get the broadcast
            let alice_before = length(alice.received)
            bob->send("hi from bob")
            wait_messages(bob, 2)  // welcome + echo
            wait_messages(alice, alice_before + 1)

            // bob got echo
            t |> success(find(bob.received[length(bob.received) - 1], "echo:") >= 0, "bob got echo")
            // alice got broadcast
            let alice_last = alice.received[length(alice.received) - 1]
            t |> success(find(alice_last, "hi from bob") >= 0, "alice got broadcast")

            alice->close()
            bob->close()
            wait_closed(alice)
            wait_closed(bob)
            unsafe {
                delete alice
                delete bob
            }
        }

        t |> run("HTTP works alongside WebSocket") @(t : T?) {
            // First connect a WS client
            var client = new TestWsClient()
            client->init("{ws_url}/ws")
            wait_messages(client, 1)

            // Then make HTTP requests
            GET("{http_url}/ping") <| $(resp) {
                t |> equal(string(resp.body), "pong")
            }

            GET("{http_url}/ws-clients") <| $(resp) {
                // At least 1 WS client connected
                let count = to_int(string(resp.body))
                t |> success(count >= 1, "at least 1 WS client reported via HTTP")
            }

            client->close()
            wait_closed(client)
            unsafe {
                delete client
            }
        }

        t |> run("disconnect notifies remaining clients") @(t : T?) {
            var alice = new TestWsClient()
            alice->init("{ws_url}/ws")
            wait_messages(alice, 1)  // welcome

            var bob = new TestWsClient()
            bob->init("{ws_url}/ws")
            wait_messages(bob, 1)  // welcome
            wait_messages(alice, 2)  // welcome + bob's join notification

            let alice_before = length(alice.received)

            // Gracefully disconnect bob
            bob->close()
            wait_closed(bob)

            // Alice should get "left" notification
            wait_messages(alice, alice_before + 1)
            let alice_last = alice.received[length(alice.received) - 1]
            t |> success(find(alice_last, "left") >= 0, "alice notified of disconnect")

            alice->close()
            wait_closed(alice)
            unsafe {
                delete alice
                delete bob
            }
        }

        t |> run("is_connected reflects state") @(t : T?) {
            var client = new TestWsClient()
            client->init("{ws_url}/ws")
            wait_messages(client, 1)
            t |> success(client->is_connected(), "connected after init")
            client->close()
            wait_closed(client)
            t |> success(!client->is_connected(), "disconnected after close")
            unsafe {
                delete client
            }
        }
    }
}
