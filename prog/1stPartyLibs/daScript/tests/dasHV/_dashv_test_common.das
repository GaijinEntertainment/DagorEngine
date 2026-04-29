// Shared helpers for dasHV tests
//
// Provides:
//   - Port constants (one per test file to avoid conflicts)
//   - with_test_server() lifecycle helper
//   - File I/O helpers for form upload tests

options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false

module _dashv_test_common shared public

require dashv/dashv_boost public
require daslib/jobque_boost public
require daslib/fio public

// ──────────────────────────────────────────────────────────────────────────
// Port assignments — each test file uses a unique port
// ──────────────────────────────────────────────────────────────────────────

let PORT_CLIENT_BASIC   = 19001
let PORT_REQUEST_BUILDER = 19002
let PORT_SERVER_ROUTES  = 19003
let PORT_SERVER_ADVANCED = 19004
let PORT_COOKIES        = 19005
let PORT_FORMS          = 19006
let PORT_WEBSOCKETS     = 19007
let PORT_SSE            = 19008

// ──────────────────────────────────────────────────────────────────────────
// Generic server lifecycle
// ──────────────────────────────────────────────────────────────────────────
//
// Starts a HvWebServer subclass on a dedicated thread, waits for it to be
// ready, runs the user's block, then shuts down.

def with_test_server(server_type : auto(ServerType); port : int; blk : block<(base_url : string) : void>) {
    with_job_status(1) $(started) {
        with_job_status(1) $(finished) {
            with_atomic32() $(stop_flag) {
                new_thread() @() {
                    var server = new ServerType()
                    server->init(port)
                    server->start()
                    started |> notify_and_release
                    while (stop_flag |> get == 0) {
                        server->tick()
                        sleep(1u)
                    }
                    server->stop()
                    finished |> notify_and_release
                }
                started |> join
                let base_url = "http://127.0.0.1:{port}"
                invoke(blk, base_url)
                stop_flag |> set(1)
                finished |> join
            }
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────
// File helpers for form/upload tests
// ──────────────────────────────────────────────────────────────────────────

def write_test_file(path, content : string) {
    fopen(path, "w") <| $(f) {
        if (f != null) {
            fwrite(f, content)
        }
    }
}

def read_test_file(path : string) : string {
    var result = ""
    fopen(path, "r") <| $(f) {
        if (f != null) {
            result = fread(f)
        }
    }
    return result
}
