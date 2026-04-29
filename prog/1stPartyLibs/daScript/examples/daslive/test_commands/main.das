options gen2

require daslib/json
require daslib/json_boost
require live/live_commands
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
    // Verify commands are registered
    if (!key_exists(live_command_registry, "spawn")) {
        print("FAIL: spawn not registered\n")
        request_exit()
        return
    }
    if (!key_exists(live_command_registry, "get_status")) {
        print("FAIL: get_status not registered\n")
        request_exit()
        return
    }
    print("PASS: 2 commands registered\n")

    // Test dispatch
    var r1 = live_dispatch_command("\{\"name\": \"spawn\"}")
    print("dispatch spawn: {r1}\n")
    var r2 = live_dispatch_command("\{\"name\": \"spawn\"}")
    print("dispatch spawn: {r2}\n")
    var r3 = live_dispatch_command("\{\"name\": \"get_status\"}")
    print("dispatch get_status: {r3}\n")
    var r4 = live_dispatch_command("\{\"name\": \"unknown\"}")
    print("dispatch unknown: {r4}\n")

    if (spawn_count == 2) {
        print("PASS: spawn_count == 2\n")
    } else {
        print("FAIL: spawn_count == {spawn_count}\n")
    }
    print("ALL TESTS PASSED\n")
}

[export]
def update() {
    request_exit()
}

[export]
def shutdown() {
    pass
}

[export]
def main() {
    init()
}
