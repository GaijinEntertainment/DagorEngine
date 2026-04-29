options gen2

require daslib/decs_boost
require live/decs_live
require live_host

var reload_count : int = 0

[before_reload]
def save_reload_count() {
    var data : array<uint8>
    data |> push(uint8(reload_count & int(0xFF)))
    live_store_bytes("reload_count", data)
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
        print("init: reload #{reload_count}\n")
    } else {
        reload_count = 0
        restart()
        // Create test entities
        for (i in range(5)) {
            create_entity() @(eid, cmp) {
                cmp |> set("index", i)
                cmp |> set("name", "entity_{i}")
                cmp |> set("value", float(i) * 10.0)
            }
        }
        commit()
        print("init: created 5 entities\n")
    }
}

[export]
def update() {
    if (reload_count == 0) {
        // First run: verify entities exist, then reload
        var count = 0
        query() $(index : int; name : string; value : float) {
            count++
        }
        if (count != 5) {
            print("FAIL: expected 5 entities, got {count}\n")
            request_exit()
            return
        }
        print("PASS: {count} entities before reload\n")
        request_reload(false)
    } elif (reload_count == 1) {
        // After first reload: verify DECS state survived
        var count = 0
        var sum = 0
        var names_ok = true
        query() $(index : int; name : string; value : float) {
            count++
            sum += index
            if (name != "entity_{index}") {
                names_ok = false
            }
            if (value != float(index) * 10.0) {
                names_ok = false
            }
        }
        if (count != 5) {
            print("FAIL: expected 5 entities after reload, got {count}\n")
            request_exit()
            return
        }
        if (sum != 10) {  // 0+1+2+3+4
            print("FAIL: index sum wrong, expected 10, got {sum}\n")
            request_exit()
            return
        }
        if (!names_ok) {
            print("FAIL: entity names or values corrupted after reload\n")
            request_exit()
            return
        }
        print("PASS: all 5 entities survived reload with correct data\n")

        // Mutate: add more entities, then reload again
        for (i in range(3)) {
            create_entity() @(eid, cmp) {
                cmp |> set("index", 100 + i)
                cmp |> set("name", "new_{i}")
                cmp |> set("value", float(100 + i))
            }
        }
        commit()
        print("PASS: added 3 more entities, requesting second reload\n")
        request_reload(false)
    } elif (reload_count == 2) {
        // After second reload: verify both original and new entities
        var count = 0
        var original = 0
        var added = 0
        query() $(index : int; name : string) {
            count++
            if (index < 100) {
                original++
            } else {
                added++
            }
        }
        if (original != 5) {
            print("FAIL: expected 5 original entities, got {original}\n")
            request_exit()
            return
        }
        if (added != 3) {
            print("FAIL: expected 3 added entities, got {added}\n")
            request_exit()
            return
        }
        print("PASS: {original} original + {added} added entities survived second reload\n")
        print("ALL TESTS PASSED\n")
        request_exit()
    }
}

[export]
def shutdown() {
    print("shutdown: reload_count={reload_count}\n")
}
