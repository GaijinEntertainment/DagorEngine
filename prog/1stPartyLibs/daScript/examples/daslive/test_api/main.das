options gen2

require daslib/json
require daslib/json_boost
require live/live_commands
require live/live_api
require live_host

var spawn_count : int = 0

[live_command]
def spawn(input : JsonValue?) : JsonValue? {
    spawn_count++
    return JV(spawn_count)
}

[live_command]
def get_status(input : JsonValue?) : JsonValue? {
    return JV("alive")
}

[export]
def init() {
    if (!key_exists(live_command_registry, "spawn")) {
        print("FAIL: spawn not registered\n")
        request_exit()
        return
    }
    print("PASS: {length(live_command_registry)} commands registered\n")
    print("live_api test ready on port {live_api_port} (will run for 10 seconds)\n")
}

[export]
def update() {
    if (get_uptime() > 10.0f) {
        request_exit()
    }
}

[export]
def shutdown() {
    print("shutdown, spawn_count={spawn_count}\n")
}
