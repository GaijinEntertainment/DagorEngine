options gen2
options persistent_heap
options gc

require dashv/dashv_boost
require daslib/json
require daslib/json_boost
require daslib/jobque_boost
require live/live_commands
require live/live_api
require live/live_gc
require strings
require live_host

// Integration test for the live-host REST API, self-driven from inside the
// hosted program. Run with:
//   daslang-live.exe --live-port=9090 examples/daslive/test_api_http/main.das
//
// The API server is serviced by the host's frame loop (the debug agent's onTick),
// so a synchronous GET from update() would deadlock the very loop that has to
// answer it. The client therefore runs on its own thread; update() keeps the
// frames flowing and polls for the thread's verdict.

var tests_passed : int = 0
var tests_failed : int = 0

def check(name : string; condition : bool) {
    if (condition) {
        tests_passed++
        print("  PASS: {name}\n")
    } else {
        tests_failed++
        print("  FAIL: {name}\n")
    }
}

// --- Test commands ---

var cmd_counter : int = 0

[live_command, unused_argument(input)]
def ping(input : JsonValue?) : JsonValue? {
    cmd_counter++
    return JV("\{\"pong\": {cmd_counter}}")
}

// --- Tests (run on the client thread) ---

def base_url() : string {
    return "http://127.0.0.1:{live_api_port}"
}

def test_status() {
    print("--- GET /status ---\n")
    GET("{base_url()}/status") $(resp) {
        check("status returns a response", resp != null)
        if (resp == null) return
        let body = string(resp.body)
        check("status returns 200", int(resp.status_code) == int(http_status.OK))
        check("status contains fps", find(body, "fps") >= 0)
        check("status contains uptime", find(body, "uptime") >= 0)
        check("status contains paused", find(body, "paused") >= 0)
        check("status contains dt", find(body, "dt") >= 0)
    }
}

def test_pause_unpause() {
    print("--- POST /pause and /unpause ---\n")
    POST("{base_url()}/pause", "") $(resp) {
        check("pause returns a response", resp != null)
        if (resp == null) return
        check("pause returns 200", int(resp.status_code) == int(http_status.OK))
        peek(resp.body) $(body) {
            check("pause returns paused:true", find(body, "true") >= 0)
        }
    }
    // Verify via status
    GET("{base_url()}/status") $(resp) {
        var paused = false
        if (resp != null) {
            var parse_error = ""
            var js = read_json(string(resp.body), parse_error)
            if (js != null && js.value is _object) {
                let tab & = unsafe(js.value as _object)
                var pv = tab?["paused"] ?? null
                if (pv != null && pv.value is _bool) {
                    paused = pv.value as _bool
                }
            }
        }
        check("status shows paused", paused)
    }
    POST("{base_url()}/unpause", "") $(resp) {
        check("unpause returns 200", resp != null && int(resp.status_code) == int(http_status.OK))
    }
}

// The ping command returns JV("{\"pong\": N}") - a JSON STRING holding JSON - so the wire
// body is a quoted string; parse the outer string, then the object inside it. -1 = no pong.
def pong_of(body : string) : int {
    var err = ""
    var outer = read_json(body, err)
    if (outer == null || !(outer.value is _string)) return -1
    var inner = read_json(outer.value as _string, err)
    if (inner == null || !(inner.value is _object)) return -1
    let tab & = unsafe(inner.value as _object)
    var pv = tab?["pong"] ?? null
    return pv ?? -1   // integral JSON literals parse as _longint, not _number - let json_boost widen
}

def test_command() {
    print("--- POST /command ---\n")
    var first = -1
    POST("{base_url()}/command", "\{\"name\": \"ping\"}") $(resp) {
        check("command returns a response", resp != null)
        if (resp == null) return
        check("command returns 200", int(resp.status_code) == int(http_status.OK))
        first = pong_of(string(resp.body))
        check("command ping response carries a pong counter", first >= 0)
    }
    POST("{base_url()}/command", "\{\"name\": \"ping\"}") $(resp) {
        let second = resp != null ? pong_of(string(resp.body)) : -1
        check("second ping increments the counter by one", first >= 0 && second == first + 1)
    }
}

def test_command_errors() {
    print("--- POST /command errors ---\n")
    POST("{base_url()}/command", "\{\"name\": \"nonexistent\"}") $(resp) {
        check("unknown command returns a response", resp != null)
        if (resp == null) return
        check("unknown command returns 200", int(resp.status_code) == int(http_status.OK))
        peek(resp.body) $(body) {
            check("unknown command names the miss", find(body, "unknown command") >= 0)
        }
    }
}

// --- Lifecycle ---

let CLIENT_IDLE = 0
let CLIENT_RUNNING = 1
let CLIENT_DONE = 2

// Written by the client thread, read by update() — hence atomic. The verdict is
// the printed summary (the host has no exit-code channel).
var g_client_state : Atomic32?

def run_client() {
    // pause_unpause leaves the host unpaused; the [live_command] ping runs on the
    // host loop, so cmd_counter is host-owned — the thread only reads bodies.
    test_status()
    test_pause_unpause()
    test_command()
    test_command_errors()
    print("\n=== Results: {tests_passed} passed, {tests_failed} failed ===\n")
    if (tests_failed == 0) {
        print("ALL TESTS PASSED\n")
    }
}

[export]
def init() {
    print("test_api_http: init\n")
    g_client_state = atomic32_create()
    if (!is_live_mode()) {
        print("test_api_http: no live API server outside daslang-live — run under the host\n")
        g_client_state |> set(CLIENT_DONE)
    }
}

[export]
def update() {
    if (g_client_state |> get() == CLIENT_IDLE) {
        g_client_state |> set(CLIENT_RUNNING)
        // The thread runs in a cloned context whose globals start blank; the
        // lambda's captured LOCALS travel, so hand the handle over that way.
        var client_state = g_client_state
        new_thread() @() {
            run_client()
            client_state |> set(CLIENT_DONE)
        }
    } elif (g_client_state |> get() == CLIENT_DONE) {
        request_exit()
    }
}

[export]
def shutdown() {
    print("test_api_http: shutdown\n")
    unsafe(atomic32_remove(g_client_state))
}

[export]
def main() {
    init()
    while (!exit_requested()) {
        update()
        maybe_collect_gc()
    }
    shutdown()
}
