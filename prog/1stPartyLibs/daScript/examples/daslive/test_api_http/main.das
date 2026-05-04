options gen2
options persistent_heap
options gc

require dashv/dashv_boost
require daslib/json
require daslib/json_boost
require live/live_commands
require live/live_api
require strings
require live_host

// Integration test for the REST API.
// Run with: daslang-live.exe examples/daslive/test_api_http/main.das
// Or:       daslang.exe examples/daslive/test_api_http/main.das

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

[live_command]
def ping(input : JsonValue?) : JsonValue? {
    cmd_counter++
    return JV("\{\"pong\": {cmd_counter}}")
}

// --- Tests ---

def test_status() {
    print("--- GET /status ---\n")
    GET("http://localhost:{live_api_port}/status") <| $(resp) {
        let body = string(resp.body)
        check("status returns 200", int(resp.status_code) == int(http_status.OK))
        check("status contains fps", find(body, "fps") >= 0)
        check("status contains uptime", find(body, "uptime") >= 0)
        check("status contains paused", find(body, "paused") >= 0)
        check("status contains dt", find(body, "dt") >= 0)
    }
}

def test_reload() {
    print("--- POST /reload ---\n")
    POST("http://localhost:{live_api_port}/reload", "") <| $(resp) {
        let body = string(resp.body)
        check("reload returns 200", int(resp.status_code) == int(http_status.OK))
        check("reload returns ok", find(body, "true") >= 0)
    }
}

def test_reload_full() {
    print("--- POST /reload/full ---\n")
    POST("http://localhost:{live_api_port}/reload/full", "") <| $(resp) {
        let body = string(resp.body)
        check("reload/full returns 200", int(resp.status_code) == int(http_status.OK))
        check("reload/full returns ok", find(body, "true") >= 0)
    }
}

def test_pause_unpause() {
    print("--- POST /pause and /unpause ---\n")
    POST("http://localhost:{live_api_port}/pause", "") <| $(resp) {
        check("pause returns 200", int(resp.status_code) == int(http_status.OK))
        check("pause returns paused:true", find(string(resp.body), "true") >= 0)
    }
    // Verify via status
    GET("http://localhost:{live_api_port}/status") <| $(resp) {
        var parse_error = ""
        var js = read_json(string(resp.body), parse_error)
        if (js != null && js.value is _object) {
            let tab & = unsafe(js.value as _object)
            var pv = tab?["paused"] ?? null
            if (pv != null && pv.value is _bool) {
                check("status shows paused", pv.value as _bool)
            } else {
                check("status shows paused", false)
            }
        }
    }
    // Unpause
    POST("http://localhost:{live_api_port}/unpause", "") <| $(resp) {
        check("unpause returns 200", int(resp.status_code) == int(http_status.OK))
    }
    set_paused(false)
}

def test_command() {
    print("--- POST /command ---\n")
    cmd_counter = 0
    POST("http://localhost:{live_api_port}/command", "\{\"name\": \"ping\"}") <| $(resp) {
        check("command returns 200", int(resp.status_code) == int(http_status.OK))
        let body = string(resp.body)
        check("command ping response has pong", find(body, "pong") >= 0)
    }
    // Call again
    POST("http://localhost:{live_api_port}/command", "\{\"name\": \"ping\"}") <| $(resp) {
        let body = string(resp.body)
        check("second ping increments counter", find(body, "2") >= 0)
    }
}

def test_command_errors() {
    print("--- POST /command errors ---\n")
    // Unknown command
    POST("http://localhost:{live_api_port}/command", "\{\"name\": \"nonexistent\"}") <| $(resp) {
        check("unknown command returns 200", int(resp.status_code) == int(http_status.OK))
        check("unknown command has error", find(string(resp.body), "error") >= 0)
    }
}

// --- Lifecycle ---

var test_phase : int = 0

[export]
def init() {
    print("test_api_http: init\n")
    test_phase = 0
}

[export]
def update() {
    test_phase++
    // Wait a couple frames for the API server to start
    if (test_phase == 3) {
        test_status()
        test_reload()
        test_reload_full()
        test_pause_unpause()
        test_command()
        test_command_errors()
        print("\n=== Results: {tests_passed} passed, {tests_failed} failed ===\n")
        if (tests_failed == 0) {
            print("ALL TESTS PASSED\n")
        }
        request_exit()
    }
}

[export]
def shutdown() {
    print("test_api_http: shutdown\n")
}

[export]
def main() {
    init()
    while (!exit_requested()) {
        update()
    }
    shutdown()
}
