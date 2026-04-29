options gen2

require live_host

var frame_count : int = 0
var reload_count : int = 0

[before_reload]
def save_state() {
    print("save_state: storing reload_count={reload_count}\n")
    var data : array<uint8>
    data |> push(uint8(reload_count & int(0xFF)))
    live_store_bytes("reload_count", data)
}

[after_reload]
def restore_state() {
    var data : array<uint8>
    if (live_load_bytes("data_check", data)) {
        print("restore_state: loaded data_check, size={length(data)}\n")
    }
    print("restore_state: done\n")
}

[after_reload]
def restore_reload_count() {
    var data : array<uint8>
    if (live_load_bytes("reload_count", data)) {
        reload_count = int(data[0])
    }
}

[export]
def init() {
    if (is_reload()) {
        // reload_count already restored by [after_reload] before init() runs
        reload_count++
        print("init: reload #{reload_count}, restored from store\n")
    } else {
        reload_count = 0
        print("init: first run\n")
    }
    frame_count = 0
}

[export]
def update() {
    frame_count++
    if (frame_count == 10 && reload_count == 0) {
        // First run: store some data and request reload
        print("update: frame {frame_count}, storing test data and requesting reload\n")
        var check : array<uint8>
        check |> push(uint8(0xAB))
        check |> push(uint8(0xCD))
        live_store_bytes("data_check", check)
        request_reload(false)
    } elif (frame_count == 10 && reload_count == 1) {
        // Second run (after reload): verify data survived
        var data : array<uint8>
        if (live_load_bytes("data_check", data)) {
            assert(length(data) == 2)
            assert(data[0] == uint8(0xAB))
            assert(data[1] == uint8(0xCD))
            print("update: persistent store verified! data survived reload\n")
        } else {
            print("ERROR: data_check not found after reload!\n")
        }
        print("update: test passed, requesting exit\n")
        request_exit()
    }
}

[export]
def shutdown() {
    print("shutdown: reload_count={reload_count}, frame_count={frame_count}\n")
}

// Dual-mode: under daslang.exe, just run init/shutdown (no reload support)
[export]
def main() {
    print("main: running in simple mode (no reload)\n")
    init()
    for (i in range(20)) {
        update()
    }
    shutdown()
}
